#pragma once

#include <gui/view.h>
#include "desktop_view_main.h"

typedef struct DesktopDigiviceView DesktopDigiviceView;

DesktopDigiviceView* desktop_digivice_view_alloc(void);
void desktop_digivice_view_free(DesktopDigiviceView* digivice_view);
View* desktop_digivice_view_get_view(DesktopDigiviceView* digivice_view);

/** Short Right is reported as DesktopAnimationEventInteractAnimation, as the dolphin did. */
void desktop_digivice_view_set_callback(
    DesktopDigiviceView* digivice_view,
    DesktopMainViewCallback callback,
    void* context);
