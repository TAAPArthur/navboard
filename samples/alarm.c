#include "../navboard.h"
#include <stdio.h>

/*
 * Sample interface for an alarm clock.
 * When submit is pressed, the current alarm schedule will be printed in a
 * cron compatible format
 */
void genCronTab();
Key keys_alarm[] = {
    {.label = "min", .min = 0, .max = 60},
    {0},
    {.label = "hour", .min = 0, .max = 24},
    {0},
    {.label = "Sun", .flags = LATCH},
    {.label = "Mon", .flags = LATCH},
    {.label = "Tue", .flags = LATCH},
    {.label = "Wed", .flags = LATCH},
    {.label = "Thu", .flags = LATCH},
    {.label = "Fri", .flags = LATCH},
    {.label = "Sat", .flags = LATCH},
    {0},
    {.label = "Submit", .onPress = genCronTab},
};

void genCronTab() {
    int minute = keys_alarm[0].value;
    int hour = keys_alarm[2].value;
    printf("%d %d * * ", minute, hour);
    for (int i = 0; i < 7; i++) {
        if (keys_alarm[4 + i].pressed) {
            printf("%d,", i);
        }
    }
    printf("\n");

}
REGISTER(alarm, keys_alarm);
