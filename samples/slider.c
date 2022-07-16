#include "../functions.h"
#include "../navboard.h"

/**
 * Create a 2 rows each containing a slider. When the slider gets updated, they label and current value will be printed
 */
Key keys_slider[] = {
    {.label = "Slider1", .min = 0, .max = 100, .onDrag = spawnCmd, .arg.s = "echo $KEY_LABEL: $KEY_VALUE"},
    {0},
    {.label = "Slider2", .min = 0, .max = 100, .onDrag = spawnCmd, .arg.s = "echo $KEY_LABEL: $KEY_VALUE"},
};
REGISTER(sliders, keys_slider);
