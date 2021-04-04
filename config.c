#include "navboard.h"
#include "config.h"
#include "xutil.h"

void(*xEventHandlers[MAX_X_EVENTS])() = {
    [0] = logError,
    [XCB_BUTTON_PRESS] = buttonEvent,
    [XCB_BUTTON_RELEASE] = buttonEvent,
    [XCB_EXPOSE] = exposeEvent,
    [XCB_CONFIGURE_NOTIFY] = configureNotify,
};

Key defaults[] = {
    {XK_Alt_L,      "Alt"  ,  .flags = MOD | LATCH },
    {XK_Alt_R,      "Alt"  ,  .flags = MOD | LATCH },
    {XK_Control_L,  "Ctrl" ,  .flags = MOD | LATCH },
    {XK_Control_R,  "Ctrl" ,  .flags = MOD | LATCH },
    {XK_Hyper_L,    "Hyper",  .flags = MOD | LATCH },
    {XK_Hyper_R,    "Hyper",  .flags = MOD | LATCH },
    {XK_Meta_L,     "Meta" ,  .flags = MOD | LATCH },
    {XK_Meta_R,     "Meta" ,  .flags = MOD | LATCH },
    {XK_Caps_Lock,  "Caps" ,  .flags = MOD | LATCH },
    {XK_Shift_L,    "Shift",  .flags = MOD | LATCH },
    {XK_Shift_R,    "Shift",  .flags = MOD | LATCH },
    {XK_Shift_Lock, "Shift",  .flags = MOD | LATCH | LOCK },
    {XK_Super_L,    "Super",  .flags = MOD | LATCH },
    {XK_Super_R,    "Super",  .flags = MOD | LATCH },
    {-1 , .onPress=sendKeyPressWithModifiers, .onRelease = sendKeyReleaseWithModifiers},
    {0}
};
