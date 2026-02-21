#include "lora_driver.h"
#include "driver/spi_master.h"
#include "driver/gpio.h"
#include "esp_log.h"
#include "freertos/FreeRTOS.h"
#include "freertos/task.h"

static const char *TAG = "LORA_DRIVER";

/* SPI device handle */
static spi_device_handle_t s_spi = NULL;

/* ─── SPI Low Level ───────────────────────────────────────────── */

static void lora_write_reg(uint8_t reg, uint8_t value)
{
    spi_transaction_t t = {
        .length    = 16,
        .tx_data   = { reg | 0x80, value },  /* MSB=1 for write */
        .flags     = SPI_TRANS_USE_TXDATA,
    };
    spi_device_transmit(s_spi, &t);
}

static uint8_t lora_read_reg(uint8_t reg)
{
    spi_transaction_t t = {
        .length    = 16,
        .rxlength  = 16,
        .tx_data   = { reg & 0x7F, 0x00 },   /* MSB=0 for read  */
        .flags     = SPI_TRANS_USE_TXDATA | SPI_TRANS_USE_RXDATA,
    };
    spi_device_transmit(s_spi, &t);
    return t.rx_data[1];
}

/* ─── Reset ───────────────────────────────────────────────────── */

static void lora_reset(void)
{
    gpio_set_level(LORA_PIN_RST, 0);
    vTaskDelay(pdMS_TO_TICKS(10));
    gpio_set_level(LORA_PIN_RST, 1);
    vTaskDelay(pdMS_TO_TICKS(10));
}

/* ─── Public API ──────────────────────────────────────────────── */

bool lora_driver_init(void)
{
    /* Configure RST pin */
    gpio_config_t rst_conf = {
        .pin_bit_mask = (1ULL << LORA_PIN_RST),
        .mode         = GPIO_MODE_OUTPUT,
    };
    gpio_config(&rst_conf);

    /* Configure SPI bus */
    spi_bus_config_t bus_cfg = {
        .mosi_io_num   = LORA_PIN_MOSI,
        .miso_io_num   = LORA_PIN_MISO,
        .sclk_io_num   = LORA_PIN_SCK,
        .quadwp_io_num = -1,
        .quadhd_io_num = -1,
    };
    spi_bus_initialize(HSPI_HOST, &bus_cfg, SPI_DMA_CH_AUTO);

    /* Add LoRa as SPI device */
    spi_device_interface_config_t dev_cfg = {
        .clock_speed_hz = 1000000,       /* 1 MHz SPI clock        */
        .mode           = 0,             /* SPI mode 0             */
        .spics_io_num   = LORA_PIN_CS,
        .queue_size     = 4,
    };
    spi_bus_add_device(HSPI_HOST, &dev_cfg, &s_spi);

    /* Reset module */
    lora_reset();

    /* Check version register - SX1276 returns 0x12 */
    uint8_t version = lora_read_reg(0x42);
    if (version != 0x12) {
        ESP_LOGE(TAG, "LoRa module not found! Version: 0x%02X", version);
        return false;
    }

    /* Set sleep mode before configuration */
    lora_write_reg(0x01, 0x80);
    vTaskDelay(pdMS_TO_TICKS(10));

    /* Set LoRa mode */
    lora_write_reg(0x01, 0x81);

    /* Set frequency: 915 MHz */
    uint64_t frf = ((uint64_t)915000000 << 19) / 32000000;
    lora_write_reg(0x06, (frf >> 16) & 0xFF);
    lora_write_reg(0x07, (frf >>  8) & 0xFF);
    lora_write_reg(0x08, (frf)       & 0xFF);

    /* Set TX power to 20 dBm */
    lora_write_reg(0x09, 0xFF);

    /* Bandwidth 125kHz, CR 4/5, explicit header */
    lora_write_reg(0x1D, 0x72);

    /* SF7, CRC enabled */
    lora_write_reg(0x1E, 0x74);

    /* Sync word for private network */
    lora_write_reg(0x39, LORA_SYNC_WORD);

    /* Set to standby mode */
    lora_write_reg(0x01, 0x81);

    ESP_LOGI(TAG, "LoRa initialized at 915 MHz, SF7, 125kHz BW");
    return true;
}

bool lora_driver_send(const uint8_t *data, uint8_t length)
{
    /* Set standby mode */
    lora_write_reg(0x01, 0x81);

    /* Reset FIFO pointer */
    lora_write_reg(0x0D, 0x00);
    lora_write_reg(0x0E, 0x00);

    /* Write payload to FIFO */
    for (uint8_t i = 0; i < length; i++) {
        lora_write_reg(0x00, data[i]);
    }

    /* Set payload length */
    lora_write_reg(0x22, length);

    /* Set TX mode */
    lora_write_reg(0x01, 0x83);

    /* Wait for TX done (IRQ flag bit 3) */
    uint32_t timeout = 0;
    while (!(lora_read_reg(0x12) & 0x08)) {
        vTaskDelay(pdMS_TO_TICKS(1));
        if (++timeout > 5000) {
            ESP_LOGE(TAG, "TX timeout");
            return false;
        }
    }

    /* Clear IRQ flags */
    lora_write_reg(0x12, 0xFF);

    /* Back to standby */
    lora_write_reg(0x01, 0x81);

    ESP_LOGI(TAG, "Packet sent (%d bytes)", length);
    return true;
}

bool lora_driver_available(void)
{
    /* Check RX done flag (bit 6) */
    return (lora_read_reg(0x12) & 0x40) != 0;
}

uint8_t lora_driver_receive(uint8_t *buffer, uint8_t length)
{
    /* Set to standby to read FIFO */
    lora_write_reg(0x01, 0x81);

    uint8_t received = lora_read_reg(0x13);  /* Number of bytes received */
    if (received > length) received = length;

    /* Reset FIFO pointer to start of received packet */
    lora_write_reg(0x0D, lora_read_reg(0x10));

    /* Read bytes from FIFO */
    for (uint8_t i = 0; i < received; i++) {
        buffer[i] = lora_read_reg(0x00);
    }

    /* Clear IRQ flags */
    lora_write_reg(0x12, 0xFF);

    /* Back to continuous receive mode */
    lora_write_reg(0x01, 0x85);

    ESP_LOGI(TAG, "Packet received (%d bytes)", received);
    return received;
}

int lora_driver_rssi(void)
{
    /* RSSI = -157 + RegRssiValue (for 915 MHz) */
    return -157 + lora_read_reg(0x1A);
}

void lora_driver_sleep(void)
{
    lora_write_reg(0x01, 0x80);
    ESP_LOGI(TAG, "LoRa in sleep mode");
}

void lora_driver_wake(void)
{
    lora_write_reg(0x01, 0x85);   /* Continuous RX mode */
    vTaskDelay(pdMS_TO_TI