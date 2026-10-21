#pragma once
// Host canvas: the same calls as the firmware's, backed by the real u8g2 in a 128x64 buffer.
#include <stddef.h>
#include <stdint.h>
#include <stdbool.h>

typedef enum {
    ColorWhite = 0x00,
    ColorBlack = 0x01,
    ColorXOR = 0x02,
} Color;
typedef enum {
    FontPrimary,
    FontSecondary,
} Font;

typedef struct Canvas Canvas;

Canvas* canvas_alloc(void);
/** Row-major copy of the framebuffer: 128*64 bytes, 1 = lit (black on the LCD). */
void canvas_dump(Canvas* canvas, uint8_t* pixels);

void canvas_clear(Canvas* canvas);
void canvas_set_color(Canvas* canvas, Color color);
void canvas_set_font(Canvas* canvas, Font font);
void canvas_set_custom_u8g2_font(Canvas* canvas, const uint8_t* font);
void canvas_draw_str(Canvas* canvas, int32_t x, int32_t y, const char* str);
uint16_t canvas_string_width(Canvas* canvas, const char* str);
void canvas_draw_dot(Canvas* canvas, int32_t x, int32_t y);
void canvas_draw_box(Canvas* canvas, int32_t x, int32_t y, size_t w, size_t h);
void canvas_draw_frame(Canvas* canvas, int32_t x, int32_t y, size_t w, size_t h);
void canvas_draw_line(Canvas* canvas, int32_t x1, int32_t y1, int32_t x2, int32_t y2);
