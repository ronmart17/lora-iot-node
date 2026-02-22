#ifndef OLED_DRIVER_H
#define OLED_DRIVER_H

#include <stdint.h>

/* Display dimensions */
#define OLED_WIDTH   128
#define OLED_HEIGHT  64

/* Font sizes */
typedef enum {
    OLED_FONT_SMALL = 0,   /* 6x8 pixels */
} oled_font_t;

/**
 * @brief Initialize the SSD1306 OLED display over I2C
 */
void oled_driver_init(void);

/**
 * @brief Clear the display buffer (all pixels off)
 */
void oled_driver_clear(void);

/**
 * @brief Print a string at the given pixel position
 *
 * @param x      X coordinate (0-127)
 * @param y      Y coordinate (0-63)
 * @param text   Null-terminated string
 * @param font   Font size selector
 */
void oled_driver_print(int x, int y, const char *text, oled_font_t font);

/**
 * @brief Draw a horizontal line
 *
 * @param x1  Start X coordinate
 * @param x2  End X coordinate
 * @param y   Y coordinate
 */
void oled_driver_draw_hline(int x1, int x2, int y);

/**
 * @brief Flush the internal buffer to the display
 */
void oled_driver_update(void);

#endif /* OLED_DRIVER_H */
