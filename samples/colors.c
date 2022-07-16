#include "../navboard.h"
/**
 * Create a 3x3 grid demonstrating how colors can be set
 */
Key keys_colors[] = {
    {.label = "red foreground", .foreground = 0xFF0000},
    {.label = "green foreground", .foreground = 0x00FF00},
    {.label = "blue foreground", .foreground = 0x0000FF},
    {0},
    {.label = "red background", .background = { [0] = 0xFF0000} },
    {.label = "green background", .background = { [0] = 0x00FF00} },
    {.label = "blue background", .background = { [0] = 0x0000FF} },
    {0},
    {.label = "Red background when pressed", .background = { [1] = 0xFF0000} },
    {.label = "Green background when pressed", .background = { [1] = 0x00FF00} },
    {.label = "blue background when pressed", .background = { [1] = 0x0000FF} },
};
REGISTER(colors, keys_colors);
