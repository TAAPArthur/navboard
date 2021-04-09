#include "../navboard.h"
Key keys_numbers[] = {
    {XK_1},
    {XK_2},
    {XK_3},
    {0},
    {XK_4},
    {XK_5},
    {XK_6},
    {0},
    {XK_7},
    {XK_8},
    {XK_9},
    {0},
    {XK_0, .weight=2},
    {XK_BackSpace},
};
REGISTER(number, keys_numbers);
