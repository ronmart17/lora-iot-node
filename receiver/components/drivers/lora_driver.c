#include "lora_driver.h"
#include "driver/spi_master.h"
#include "driver/gpio.h"
#include "esp_log.h"
#include "freertos/FreeRTOS.h"
#include "freertos/task.h"
#include <string.h>

static const char *TAG = "LORA_SX1262";

static spi_device_handle_t s_spi = NULL;

/* ─── SX1262 Commands ─────────────────────────────────────────── */
#define CMD_SET_SLEEP            0x84
#define CMD_SET_STANDBY          0x80
#define CMD_SET_TX               0x83
#define CMD_SET_RX               0x82
#define CMD_SET_PKT_TYPE         0x8A
#define CMD_SET_RF_FREQ          0x86
#define CMD_SET_PA_CONFIG        0x95
#define CMD_SET_TX_PARAMS        0x8E
#define CMD_SET_BUF_BASE_ADDR   0x8F
#define CMD_SET_MOD_PARAMS       0x8B
#define CMD_SET_PKT_PARAMS       0x8C
#define CMD_SET_DIO_IRQ_PARAMS   0x08
#define CMD_GET_IRQ_STATUS       0x12
#define CMD_CLR_IRQ_STATUS       0x02
#define CMD_WRITE_BUFFER         0x0E
#define CMD_READ_BUFFER          0x1E
#define CMD_WRITE_REGISTER       0x0D
#define CMD_READ_REGISTER        0x1D
#define CMD_GET_RX_BUF_STATUS    0x13
#define CMD_GET_PKT_STATUS       0x14
#define CMD_SET_DIO2_AS_RF_SW    0x9D
#define CMD_SET_DIO3_AS_TCXO     0x97
#define CMD_CALIBRATE            0x89
#define CMD_CALIBRATE_IMAGE      0x98

/* IRQ bit masks */
#define IRQ_TX_DONE              (1 << 0)
#define IRQ_RX_DONE              (1 << 1)
#define IRQ_TIMEOUT              (1 << 9)

/* LoRa sync word register address */
#define REG_SYNC_WORD_MSB        0x0740

/* Last packet RSSI */
static int s_last_rssi = 0;

/* ─── SPI Low Level ───────────────────────────────────────────── */

static void wait_busy(void)
{
    int retries = 0;
    while (gpio_get_level(LORA_PIN_BUSY) == 1) {
        vTaskDelay(pdMS_TO_TICKS(1));
        if (++retries > 1000) {
            ESP_LOGE(TAG, "BUSY pin timeout!");
            break;
        }
    }
}

static void sx_cmd(const uint8_t *cmd, size_t cmd_len,
                   uint8_t *resp, size_t resp_len)
{
    wait_busy();

    size_t total = cmd_len + resp_len;
    uint8_t *tx = calloc(total, 1);
    uint8_t *rx = calloc(total, 1);
    if (!tx || !rx) { free(tx); free(rx); return; }

    memcpy(tx, cmd, cmd_len);

    spi_transaction_t t = {
        .length    = total * 8,
        .tx_buffer = tx,
        .rx_buffer = rx,
    };
    spi_device_transmit(s_spi, &t);

    if (resp && resp_len > 0) {
        memcpy(resp, rx + cmd_len, resp_len);
    }

    free(tx);
    free(rx);
}

static void sx_write_reg(uint16_t addr, uint8_t value)
{
    uint8_t cmd[] = { CMD_WRITE_REGISTER,
                      (uint8_t)(addr >> 8), (uint8_t)(addr),
                      value };
    sx_cmd(cmd, sizeof(cmd), NULL, 0);
}

/* ─── Reset ───────────────────────────────────────────────────── */

static void lora_reset(void)
{
    gpio_set_level(LORA_PIN_RST, 0);
    vTaskDelay(pdMS_TO_TICKS(20));
    gpio_set_level(LORA_PIN_RST, 1);
    vTaskDelay(pdMS_TO_TICKS(50));
    wait_busy();
}

/* ─── IRQ helpers ─────────────────────────────────────────────── */

static uint16_t sx_get_irq(void)
{
    uint8_t cmd[] = { CMD_GET_IRQ_STATUS, 0x00 };
    uint8_t resp[2] = {0};
    sx_cmd(cmd, sizeof(cmd), resp, 2);
    return (resp[0] << 8) | resp[1];
}

static void sx_clear_irq(uint16_t mask)
{
    uint8_t cmd[] = { CMD_CLR_IRQ_STATUS,
                      (uint8_t)(mask >> 8), (uint8_t)(mask) };
    sx_cmd(cmd, sizeof(cmd), NULL, 0);
}

/* ─── Public API ──────────────────────────────────────────────── */

bool lora_driver_init(void)
{
    /* ── GPIO setup ── */
    gpio_config_t out_conf = {
        .pin_bit_mask = (1ULL << LORA_PIN_RST),
        .mode         = GPIO_MODE_OUTPUT,
    };
    gpio_config(&out_conf);

    gpio_config_t in_conf = {
        .pin_bit_mask = (1ULL << LORA_PIN_BUSY) | (1ULL << LORA_PIN_IRQ),
        .mode         = GPIO_MODE_INPUT,
    };
    gpio_config(&in_conf);

    /* ── SPI bus ── */
    spi_bus_config_t bus = {
        .mosi_io_num   = LORA_PIN_MOSI,
        .miso_io_num   = LORA_PIN_MISO,
        .sclk_io_num   = LORA_PIN_SCK,
        .quadwp_io_num = -1,
        .quadhd_io_num = -1,
    };
    spi_bus_initialize(SPI2_HOST, &bus, SPI_DMA_CH_AUTO);

    spi_device_interface_config_t dev = {
        .clock_speed_hz = 4000000,
        .mode           = 0,
        .spics_io_num   = LORA_PIN_CS,
        .queue_size     = 4,
    };
    spi_bus_add_device(SPI2_HOST, &dev, &s_spi);

    /* ── Hardware reset ── */
    lora_reset();

    /* ── Standby RC ── */
    uint8_t standby[] = { CMD_SET_STANDBY, 0x00 };
    sx_cmd(standby, 2, NULL, 0);

    /* ── DIO2 as RF switch (required by Heltec V3) ── */
    uint8_t dio2[] = { CMD_SET_DIO2_AS_RF_SW, 0x01 };
    sx_cmd(dio2, 2, NULL, 0);

    /* ── DIO3 as TCXO 1.7 V, ~5 ms timeout ── */
    uint8_t dio3[] = { CMD_SET_DIO3_AS_TCXO, 0x02, 0x00, 0x00, 0x40 };
    sx_cmd(dio3, 5, NULL, 0);

    /* ── Calibrate all blocks ── */
    uint8_t cal[] = { CMD_CALIBRATE, 0x7F };
    sx_cmd(cal, 2, NULL, 0);
    vTaskDelay(pdMS_TO_TICKS(10));

    /* ── Packet type = LoRa ── */
    uint8_t ptype[] = { CMD_SET_PKT_TYPE, 0x01 };
    sx_cmd(ptype, 2, NULL, 0);

    /* ── Frequency 915 MHz ── */
    uint32_t frf = (uint32_t)((double)915000000 / 32000000.0 * (1 << 25));
    uint8_t freq[] = { CMD_SET_RF_FREQ,
                       (uint8_t)(frf >> 24), (uint8_t)(frf >> 16),
                       (uint8_t)(frf >>  8), (uint8_t)(frf) };
    sx_cmd(freq, 5, NULL, 0);

    /* ── Calibrate image for 902-928 MHz ── */
    uint8_t calimg[] = { CMD_CALIBRATE_IMAGE, 0xE1, 0xE9 };
    sx_cmd(calimg, 3, NULL, 0);

    /* ── PA config: dutyCycle=4, hpMax=7, devSel=0, paLut=1 ── */
    uint8_t pa[] = { CMD_SET_PA_CONFIG, 0x04, 0x07, 0x00, 0x01 };
    sx_cmd(pa, 5, NULL, 0);

    /* ── TX params: +20 dBm, ramp 200 us ── */
    uint8_t txp[] = { CMD_SET_TX_PARAMS, 0x16, 0x04 };
    sx_cmd(txp, 3, NULL, 0);

    /* ── Modulation: SF7, BW125, CR4/5, LDRO off ── */
    uint8_t mod[] = { CMD_SET_MOD_PARAMS, 0x07, 0x04, 0x01, 0x00 };
    sx_cmd(mod, 5, NULL, 0);

    /* ── Packet: preamble 8, explicit hdr, payload len, CRC on, std IQ ── */
    uint8_t pkt[] = { CMD_SET_PKT_PARAMS,
                      0x00, 0x08,           /* preamble = 8 */
                      0x00,                 /* explicit header */
                      LORA_PACKET_SIZE,     /* payload length */
                      0x01,                 /* CRC on */
                      0x00 };               /* standard IQ */
    sx_cmd(pkt, 7, NULL, 0);

    /* ── Sync word 0x1424 (private network) ── */
    sx_write_reg(REG_SYNC_WORD_MSB,     0x14);
    sx_write_reg(REG_SYNC_WORD_MSB + 1, 0x24);

    /* ── Buffer base addresses ── */
    uint8_t buf[] = { CMD_SET_BUF_BASE_ADDR, 0x00, 0x00 };
    sx_cmd(buf, 3, NULL, 0);

    /* ── IRQs: TX_DONE | RX_DONE | TIMEOUT → DIO1 ── */
    uint16_t irq = IRQ_TX_DONE | IRQ_RX_DONE | IRQ_TIMEOUT;
    uint8_t dio_irq[] = { CMD_SET_DIO_IRQ_PARAMS,
                          (uint8_t)(irq >> 8), (uint8_t)(irq),
                          (uint8_t)(irq >> 8), (uint8_t)(irq),
                          0x00, 0x00,
                          0x00, 0x00 };
    sx_cmd(dio_irq, 9, NULL, 0);

    sx_clear_irq(0xFFFF);

    ESP_LOGI(TAG, "SX1262 initialized - 915 MHz, SF7, BW125, +22 dBm");
    return true;
}

bool lora_driver_send(const uint8_t *data, uint8_t length)
{
    /* Standby */
    uint8_t stby[] = { CMD_SET_STANDBY, 0x00 };
    sx_cmd(stby, 2, NULL, 0);

    /* Update payload length in packet params */
    uint8_t pkt[] = { CMD_SET_PKT_PARAMS,
                      0x00, 0x08, 0x00, length, 0x01, 0x00 };
    sx_cmd(pkt, 7, NULL, 0);

    sx_clear_irq(0xFFFF);

    /* Write payload into TX buffer at offset 0 */
    uint8_t *wr = malloc(2 + length);
    if (!wr) return false;
    wr[0] = CMD_WRITE_BUFFER;
    wr[1] = 0x00;
    memcpy(wr + 2, data, length);
    sx_cmd(wr, 2 + length, NULL, 0);
    free(wr);

    /* Start TX (no timeout) */
    uint8_t tx[] = { CMD_SET_TX, 0x00, 0x00, 0x00 };
    sx_cmd(tx, 4, NULL, 0);

    /* Wait for TX_DONE */
    uint32_t t = 0;
    while (!(sx_get_irq() & IRQ_TX_DONE)) {
        vTaskDelay(pdMS_TO_TICKS(1));
        if (++t > 5000) {
            ESP_LOGE(TAG, "TX timeout");
            sx_cmd(stby, 2, NULL, 0);
            return false;
        }
    }

    sx_clear_irq(0xFFFF);
    sx_cmd(stby, 2, NULL, 0);

    ESP_LOGI(TAG, "Packet sent (%d bytes)", length);
    return true;
}

bool lora_driver_available(void)
{
    return (sx_get_irq() & IRQ_RX_DONE) != 0;
}

uint8_t lora_driver_receive(uint8_t *buffer, uint8_t length)
{
    /* Get RX buffer status */
    uint8_t cmd[] = { CMD_GET_RX_BUF_STATUS, 0x00 };
    uint8_t resp[2] = {0};
    sx_cmd(cmd, sizeof(cmd), resp, 2);

    uint8_t plen   = resp[0];
    uint8_t offset = resp[1];
    if (plen > length) plen = length;

    /* Read payload */
    uint8_t *rd = calloc(3 + plen, 1);
    uint8_t *rx = calloc(plen, 1);
    if (!rd || !rx) { free(rd); free(rx); return 0; }

    rd[0] = CMD_READ_BUFFER;
    rd[1] = offset;
    rd[2] = 0x00;   /* NOP */
    sx_cmd(rd, 3, rx, plen);
    memcpy(buffer, rx, plen);
    free(rd);
    free(rx);

    /* Get RSSI */
    uint8_t ps_cmd[] = { CMD_GET_PKT_STATUS, 0x00 };
    uint8_t ps[3] = {0};
    sx_cmd(ps_cmd, sizeof(ps_cmd), ps, 3);
    s_last_rssi = -(int)(ps[0] / 2);

    sx_clear_irq(0xFFFF);

    /* Back to continuous RX */
    uint8_t rx_cmd[] = { CMD_SET_RX, 0xFF, 0xFF, 0xFF };
    sx_cmd(rx_cmd, 4, NULL, 0);

    ESP_LOGI(TAG, "Received %d bytes (RSSI %d dBm)", plen, s_last_rssi);
    return plen;
}

int lora_driver_rssi(void)
{
    return s_last_rssi;
}

void lora_driver_sleep(void)
{
    uint8_t cmd[] = { CMD_SET_SLEEP, 0x04 };  /* warm start */
    sx_cmd(cmd, 2, NULL, 0);
    ESP_LOGI(TAG, "SX1262 sleeping");
}

void lora_driver_wake(void)
{
    uint8_t stby[] = { CMD_SET_STANDBY, 0x00 };
    sx_cmd(stby, 2, NULL, 0);
    vTaskDelay(pdMS_TO_TICKS(10));

    uint8_t rx[] = { CMD_SET_RX, 0xFF, 0xFF, 0xFF };
    sx_cmd(rx, 4, NULL, 0);
    ESP_LOGI(TAG, "SX1262 awake, listening...");
}
