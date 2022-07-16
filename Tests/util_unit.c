#include <assert.h>
#include <scutest/scutest.h>
#include <stdio.h>
#include <string.h>

#include "../navboard.h"
#include "../util.h"

SCUTEST(test_init_spawn) {
    assert(spawn("exit 0") == 0);
    assert(spawn("exit 1") == 1);
}

SCUTEST(test_read_cmd) {
    char buffer[255];
    assert(readCmd("exit 0", buffer, sizeof(buffer)) == 0);
    assert(buffer[0] == 0);
    assert(readCmd("printf hi", buffer, sizeof(buffer)) == 0);
    assert(strcmp(buffer, "hi") == 0);
}
