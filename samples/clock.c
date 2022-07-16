#include "../functions.h"
#include "../navboard.h"
#include <stdio.h>

/*
 * A sample interface that could be used to set the current yime
 */
void setTime();
Key keys_clock[] = {
    // The current date values are read from the date/time command
    {.label = "year"  , .min = 0, .max = 99, .loadValue = readValueFromCmd, .arg.s = "date +%y"},
    {.label = "month" , .min = 0, .max = 12, .loadValue = readValueFromCmd, .arg.s = "date +%m"},
    {.label = "day"   , .min = 0, .max = 31, .loadValue = readValueFromCmd, .arg.s = "date +%d"},
    {0},
    {.label = "hour"  , .min = 0, .max = 24, .loadValue = readValueFromCmd, .arg.s = "date +%H"},
    {.label = "min"   , .min = 0, .max = 60, .loadValue = readValueFromCmd, .arg.s = "date +%M"},
    {.label = "second", .min = 0, .max = 60,.loadValue = readValueFromCmd, .arg.s = "date +%S"},
    {0},
    {.label = "Submit", .onPress = setTime},
};

void setTime() {
    for (int i = 0; i < LEN(keys_clock); i++) {
        if (!isRowSeperator(&keys_clock[i])) {
            printf("%s: %d\n", keys_clock[i].label, keys_clock[i].value);
        }
    }
}
REGISTER(clock, keys_clock);
