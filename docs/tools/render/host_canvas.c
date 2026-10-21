// Host implementation of the canvas_* calls the digivice code makes. Every call forwards to the
// same u8g2 function the firmware's canvas.c uses, so lines, boxes, XOR and glyphs match.
#include <gui/canvas.h>
#include <stdlib.h>
#include <u8g2.h>

struct Canvas {
    u8g2_t fb;
};

static uint8_t display_cb(u8x8_t* u8x8, uint8_t msg, uint8_t arg_int, void* arg_ptr) {
    static const u8x8_display_info_t info = {
        .tile_width = 16, .tile_height = 8, .pixel_width = 128, .pixel_height = 64};
    (void)arg_int;
    (void)arg_ptr;
    if(msg == U8X8_MSG_DISPLAY_SETUP_MEMORY) {
        u8x8_d_helper_display_setup_memory(u8x8, &info);
        return 1;
    }
    return msg == U8X8_MSG_DISPLAY_INIT ? 1 : 0;
}

static uint8_t nop_cb(u8x8_t* u8x8, uint8_t msg, uint8_t arg_int, void* arg_ptr) {
    (void)u8x8;
    (void)msg;
    (void)arg_int;
    (void)arg_ptr;
    return 1;
}

Canvas* canvas_alloc(void) {
    Canvas* canvas = calloc(1, sizeof(Canvas));
    uint8_t tile_rows;
    uint8_t* buf = u8g2_m_16_8_f(&tile_rows);
    u8g2_SetupDisplay(&canvas->fb, display_cb, u8x8_cad_001, nop_cb, nop_cb);
    u8g2_SetupBuffer(&canvas->fb, buf, tile_rows, u8g2_ll_hvline_vertical_top_lsb, U8G2_R0);
    canvas_clear(canvas);
    canvas_set_color(canvas, ColorBlack);
    canvas_set_font(canvas, FontSecondary);
    return canvas;
}

void canvas_dump(Canvas* canvas, uint8_t* pixels) {
    const uint8_t* buf = u8g2_GetBufferPtr(&canvas->fb);
    for(int y = 0; y < 64; y++)
        for(int x = 0; x < 128; x++) pixels[y * 128 + x] = (buf[(y / 8) * 128 + x] >> (y % 8)) & 1;
}

void canvas_clear(Canvas* canvas) {
    u8g2_ClearBuffer(&canvas->fb);
}

void canvas_set_color(Canvas* canvas, Color color) {
    u8g2_SetDrawColor(&canvas->fb, color);
}

void canvas_set_font(Canvas* canvas, Font font) {
    u8g2_SetFontMode(&canvas->fb, 1);
    u8g2_SetFont(
        &canvas->fb, font == FontPrimary ? u8g2_font_helvB08_tr : u8g2_font_haxrcorp4089_tr);
}

void canvas_set_custom_u8g2_font(Canvas* canvas, const uint8_t* font) {
    u8g2_SetFontMode(&canvas->fb, 1);
    u8g2_SetFont(&canvas->fb, font);
}

void canvas_draw_str(Canvas* canvas, int32_t x, int32_t y, const char* str) {
    u8g2_DrawUTF8(&canvas->fb, x, y, str);
}

uint16_t canvas_string_width(Canvas* canvas, const char* str) {
    return u8g2_GetUTF8Width(&canvas->fb, str);
}

void canvas_draw_dot(Canvas* canvas, int32_t x, int32_t y) {
    u8g2_DrawPixel(&canvas->fb, x, y);
}

void canvas_draw_box(Canvas* canvas, int32_t x, int32_t y, size_t w, size_t h) {
    u8g2_DrawBox(&canvas->fb, x, y, w, h);
}

void canvas_draw_frame(Canvas* canvas, int32_t x, int32_t y, size_t w, size_t h) {
    u8g2_DrawFrame(&canvas->fb, x, y, w, h);
}

void canvas_draw_line(Canvas* canvas, int32_t x1, int32_t y1, int32_t x2, int32_t y2) {
    u8g2_DrawLine(&canvas->fb, x1, y1, x2, y2);
}
