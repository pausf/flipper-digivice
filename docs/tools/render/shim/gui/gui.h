#pragma once
#include "canvas.h"
#define RECORD_GUI "gui"
typedef struct Gui Gui;
typedef struct ViewPort ViewPort;
typedef enum {
    GuiLayerFullscreen,
} GuiLayer;
#include <input/input.h>

ViewPort* view_port_alloc(void);
void view_port_free(ViewPort* view_port);
void view_port_draw_callback_set(ViewPort* view_port, void (*cb)(Canvas*, void*), void* context);
void view_port_input_callback_set(ViewPort* view_port, void (*cb)(InputEvent*, void*), void* context);
void view_port_update(ViewPort* view_port);
void gui_add_view_port(Gui* gui, ViewPort* view_port, GuiLayer layer);
void gui_remove_view_port(Gui* gui, ViewPort* view_port);
