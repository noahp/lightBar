
//
// rgb_mgr.h
// Header file for rgb_mgr.c

#include <stdint.h>
#include <stdbool.h>

#if !defined(RGB_MGR_H)
#define RGB_MGR_H

#define RGB_MGR_LEDS_PER_PANEL 84
#define RGB_MGR_PANEL_COUNT 2

// color type
typedef union rgbData_u {
    uint32_t all;
    uint8_t bytes[4];
    struct {
        uint8_t g;
        uint8_t r;
        uint8_t b;
        uint8_t pad;
    } color;
}rgbData_t;

void rgb_mgr_init(uint8_t *pBuf, uint32_t bufSize);
void rgb_mgr_set_brightness(uint8_t newBrightness);
void rgb_mgr_set_color(rgbData_t *newColor);
uint8_t rgb_mgr_get_brightness(void);
void rgb_mgr_toggle_scroll(void);
void rgb_mgr_toggle_test(void);
void rgb_mgr_toggle_increment(void);
bool rgb_mgr_is_printable(uint8_t chr);
void rgb_mgr_set_new_str(uint8_t *newStr, uint32_t len);
void rgb_mgr_main_function(void);

#endif // RGB_MGR_H
