#include "../navboard.h"
#include "../functions.h"
Key keys_mobile[] = {
    {XK_1},
    {XK_2},
    {XK_3},
    {XK_4},
    {XK_5},
    {XK_6},
    {XK_7},
    {XK_8},
    {XK_9},
    {XK_0},
    {XK_BackSpace, .weight = 2},
    {0 }, /* New row */
    {XK_q},
    {XK_w},
    {XK_e},
    {XK_r},
    {XK_t},
    {XK_y},
    {XK_u},
    {XK_i},
    {XK_o},
    {XK_p},
    {0}, /* New row */
    {XK_Shift_L, .label="Shift Lock", .weight = 1, .flags = LOCK},
    {XK_a},
    {XK_s},
    {XK_d},
    {XK_f},
    {XK_g},
    {XK_h},
    {XK_j},
    {XK_k},
    {XK_l},
    {0}, /* New row */
    {XK_Shift_L, .weight = 1},
    {XK_z},
    {XK_x},
    {XK_c},
    {XK_v},
    {XK_b},
    {XK_n},
    {XK_m},
    {0}, /* New row */
    {.label="Sym", .onRelease=activateBoard, .arg="mobile_symb"},
    {XK_Tab, .weight = 1},
    {XK_comma, .weight = 1},
    {XK_space, .weight = 5},
    {XK_period, .weight = 1},
    {XK_Return, .weight = 1},
};

Key keys_mobile_symb[] = {
    {XK_grave},
    {XK_asciitilde},
    {XK_at},
    {XK_numbersign},
    {XK_dollar},
    {XK_asciicircum},
    {XK_underscore},
    {0 }, /* New row */
    {XK_parenleft},
    {XK_parenright},
    {XK_bracketright},
    {XK_bracketleft},
    {XK_bracketright},
    {XK_braceleft},
    {XK_braceright},
    {XK_backslash},
    {0 }, /* New row */
    {XK_asterisk},
    {XK_percent},
    {XK_slash},
    {XK_plus},
    {XK_minus},
    {XK_equal},
    {XK_ampersand},
    {XK_bar},
    {0 }, /* New row */
    {XK_semicolon},
    {XK_colon},
    {XK_period},
    {XK_quotedbl},
    {XK_apostrophe},
    {XK_question},
    {0 }, /* New row */
    {.label="Nor", .onRelease=activateBoard, .arg="mobile"},
    {XK_space, .weight = 5},
    {XK_BackSpace, .weight = 2},
    {0 }, /* New row */
};

REGISTER(mobile, keys_mobile);
REGISTER(mobile_symb, keys_mobile_symb);
