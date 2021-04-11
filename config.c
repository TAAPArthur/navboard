#include "config.h"
#include "navboard.h"
#include "util.h"
#include "xutil.h"

void(*xEventHandlers[LASTEvent])() = {
    [0] = logError,
    [XCB_BUTTON_PRESS] = buttonEvent,
    [XCB_BUTTON_RELEASE] = buttonEvent,
    [XCB_SELECTION_CLEAR] = quit,
    [XCB_EXPOSE] = exposeEvent,
    [XCB_CONFIGURE_NOTIFY] = configureNotify,
};

Key defaults[] = {
    {XK_Alt_L,      .label="Alt"  ,  .flags = MOD | LATCH },
    {XK_Alt_R,      .label="Alt"  ,  .flags = MOD | LATCH },
    {XK_Control_L,  .label="Ctrl" ,  .flags = MOD | LATCH },
    {XK_Control_R,  .label="Ctrl" ,  .flags = MOD | LATCH },
    {XK_Hyper_L,    .label="Hyper",  .flags = MOD | LATCH },
    {XK_Hyper_R,    .label="Hyper",  .flags = MOD | LATCH },
    {XK_Meta_L,     .label="Meta" ,  .flags = MOD | LATCH },
    {XK_Meta_R,     .label="Meta" ,  .flags = MOD | LATCH },
    {XK_Caps_Lock,  .label="Caps" ,  .flags = MOD | LATCH },
    {XK_Shift_L,    .label="Shift", .onPress = shiftKeys, .flags = MOD | LATCH },
    {XK_Shift_R,    .label="Shift", .onPress = shiftKeys,  .flags = MOD | LATCH },
    {XK_Shift_Lock, .label="Shift", .onPress = shiftKeys,  .flags = MOD | LATCH | LOCK },
    {XK_Super_L,    .label="Super",  .flags = MOD | LATCH },
    {XK_Super_R,    .label="Super",  .flags = MOD | LATCH },
    {XK_space,      .label=" ", .onPress=sendKeyPressWithModifiers, .onRelease = sendKeyReleaseWithModifiers},
    {-1 , .onPress=sendKeyPressWithModifiers, .onRelease = sendKeyReleaseWithModifiers},
    {0}
};
