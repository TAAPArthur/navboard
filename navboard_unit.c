#define SCUTEST_DEFINE_MAIN
#include <assert.h>
#include <stdlib.h>
#include <scutest/tester.h>
#include <signal.h>
#include <unistd.h>


#include "navboard.h"
#include "xutil.h"
#include <stdio.h>

SCUTEST(init_connection) {
    initConnection();
}


void init();
SCUTEST(init_boards) {
    assert(numBoards);
    init();
    for(int i = 0; i < numBoards; i++) {
        assert(boards[i].groupSize);
        for(int j = 0; j < boards[i].groupSize; j++) {
            assert(boards[i].keyGroup[j].numKeys);
            assert(boards[i].keyGroup[j].numRows);
            assert(boards[i].keyGroup[j].dockProperties.thicknessPercent);
            for(int n = 0; n < boards[i].keyGroup[j].numKeys; n++) {
                Key*key = boards[i].keyGroup[j].keys + n;
                if(isRowSeperator(key))
                    continue;
                assert(key->label || key->keySym);
                if(key->keySym)
                    assert(key->keyCode);
                assert(key->weight);
                assert(key->onPress || key->onRelease || key->flags & MOD);
            }
        }
    }
}

SCUTEST(compute_rect, .iter=3) {
    Key keys[] = {
        {.label="A"}, {.label="B"}, {.label="C"},
        {0},
        {.label="D"}, {.label="E"}, {.label="F"},
    };
    KeyGroup keyGroup = { keys, .numKeys=LEN(keys), .windowWidth = 3 + _i, .windowHeight = 100};
    initKeyGroup(&keyGroup);
    computeRects(&keyGroup);
    assert(keyGroup.numKeys == LEN(keys));

    int width = 0, height = 0;
    int rowHeight = 0;
    for(int i = 0, n = 0; i < keyGroup.numKeys; i++) {
        if(isRowSeperator(&keyGroup.keys[i])) {
            assert(keyGroup.windowWidth == width);
            height += rowHeight;
            width = 0;
            continue;
        }
        if(rowHeight)
            assert(rowHeight == keyGroup.rects[n].height);
        else
            rowHeight = keyGroup.rects[n].height;
        width += keyGroup.rects[n].width;
        n++;
    }
    assert(keyGroup.windowWidth == width);
    assert(keyGroup.windowHeight == height + rowHeight);
}

SCUTEST(find_keys) {
    Key keys[] = {
        {.label="A"}, {.label="B"}, {.label="C"},
        {0},
        {.label="D"}, {.label="E"}, {.label="F"},
    };
    KeyGroup keyGroup = { keys, .numKeys=LEN(keys), .windowWidth = 90, .windowHeight = 90};
    initKeyGroup(&keyGroup);
    computeRects(&keyGroup);
    assert(findKey(&keyGroup,   0,   0));
    assert(&keys[0] == findKey(&keyGroup,  0,  0));
    assert(&keys[0] == findKey(&keyGroup, 20,  0));
    assert(&keys[1] == findKey(&keyGroup, 50,  0));
    assert(&keys[2] == findKey(&keyGroup, 70,  0));
    assert(&keys[2] == findKey(&keyGroup, 90,  0));
    assert(&keys[4] == findKey(&keyGroup,  0, 50));
    assert(&keys[5] == findKey(&keyGroup, 33, 50));
    assert(&keys[6] == findKey(&keyGroup, 66, 50));
    assert(&keys[6] == findKey(&keyGroup, 90, 90));
}

SCUTEST(test_latch) {

    init();
    Key keys[] = { {.label="A", .flags=LATCH} };

    Board board = CREATE_BOARD("name", keys, LEN(keys));
    initBoard(&board);
    computeRects(board.keyGroup);
    setupWindowsForBoard(&board);
    assert(!keys[0].pressed);

    triggerCell(board.keyGroup, keys, 1);
    assert(keys[0].pressed);
    triggerCell(board.keyGroup, keys, 0);
    assert(keys[0].pressed);
    triggerCell(board.keyGroup, keys, 1);
    triggerCell(board.keyGroup, keys, 0);
    assert(!keys[0].pressed);
}
