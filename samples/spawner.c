#include "../functions.h"
#include "../navboard.h"
#include <stdio.h>

Key keys_spawn[] = {
    {.label = "Program 1", .onPress = spawnCmd, .arg.s = "echo spawn $KEY_LABEL" },
    {.label = "Program 2", .onPress = spawnCmd, .arg.s = "echo spawn $KEY_LABEL" },
    {.label = "Program 3", .onPress = spawnCmd, .arg.s = "echo spawn $KEY_LABEL" },
    {.label = "Program 4", .onPress = spawnCmd, .arg.s = "echo spawn $KEY_LABEL" },
};

REGISTER(spawn, keys_spawn);
