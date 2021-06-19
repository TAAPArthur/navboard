#define SCUTEST_DEFINE_MAIN
#include <assert.h>
#include <stdlib.h>
#include <scutest/tester.h>
#include <signal.h>
#include <unistd.h>
#include <stdio.h>


#include "../navboard.h"
#include "../xutil.h"
#include "../functions.h"

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
                if(key->keySym)
                    assert(key->keyCode);
                assert(key->weight);
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
    cleanupKeygroup(&keyGroup);
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
    cleanupKeygroup(&keyGroup);
}



SCUTEST(test_load_value) {
    Key keys[] = {
        {.min=0, .max=100, .loadValue = readValueFromCmd, .arg.s="echo 10"},
        {.min=0, .max=100, .loadValue = readValueFromCmd, .arg.s="echo 20" },
    };

    Board board = CREATE_BOARD("name", keys, LEN(keys));
    initBoard(&board);
    assert(keys[0].value == 10);
    assert(keys[1].value == 20);
    cleanupBoard(&board);
}

SCUTEST_SET_ENV(init, closeConnection);

SCUTEST(test_latch) {
    Key keys[] = { {.label="A", .flags=LATCH} };

    Board board = CREATE_BOARD("name", keys, LEN(keys));
    initBoard(&board);
    computeRects(board.keyGroup);
    setupWindowsForBoard(&board);
    assert(!keys[0].pressed);

    triggerCell(board.keyGroup, keys, PRESS);
    assert(keys[0].pressed);
    triggerCell(board.keyGroup, keys, RELEASE);
    assert(keys[0].pressed);
    triggerCell(board.keyGroup, keys, PRESS);
    triggerCell(board.keyGroup, keys, RELEASE);
    assert(!keys[0].pressed);

    cleanupBoard(&board);
}
SCUTEST(test_slider_release_motion_press) {
    Key keys[] = {
        {.max=100}, {.max=100}
    };
    boards[0] = CREATE_BOARD("name", keys, LEN(keys));
    Board* board =  boards;
    initBoard(board);
    board->keyGroup[0].windowWidth = 1000;
    board->keyGroup[0].windowHeight = 1000;
    computeRects(board->keyGroup);
    setupWindowsForBoard(board);


    int win = *(xcb_window_t*)board->keyGroup[0].drawable;

    triggerCellAtPosition(0, PRESS, win, 0, 0);
    triggerCellAtPosition(0, RELEASE, win, 0, 0);
    triggerCellAtPosition(0, DRAG, win, board->keyGroup[0].windowWidth, 0);
    assert(keys[0].value == 0);
    triggerCellAtPosition(0, PRESS, win, 0, 0);
    triggerCellAtPosition(0, DRAG, win, board->keyGroup[0].windowWidth, 0);
    assert(keys[0].value == 100);

    cleanupBoard(board);
}

SCUTEST(test_activate_boards_click_and_cycle) {

    for(int i=0;i<numBoards * 2; i++) {
        activateBoardByName(boards[i%numBoards].name);
        assert(getActiveBoard()->keyGroup[0].drawable);
        int win = *(xcb_window_t*)getActiveBoard()->keyGroup[0].drawable;

        computeRects(getActiveBoard()->keyGroup);
        triggerCellAtPosition(0, PRESS, win, 0, 0);
        triggerCellAtPosition(0, RELEASE, win, 0, 0);
    }
}

SCUTEST(test_swap_boards) {
    static Key keys[][1] = {
        {{.label="A", .onPress=activateBoard, .arg="B"}},
        {{.label="B", .onPress=activateBoard, .arg="A"}},
    };
    boards[numBoards] = CREATE_BOARD("A", keys[0], 1);
    initBoard(&boards[numBoards++]);
    boards[numBoards] = CREATE_BOARD("B", keys[1], 1);
    initBoard(&boards[numBoards++]);
    activateBoardByName("B");
    for(int i = 0; i < 10; i++) {
        computeRects(getActiveBoard()->keyGroup);
        Board* board = getActiveBoard();
        int win = *(xcb_window_t*)board->keyGroup->drawable;
        triggerCellAtPosition(0, PRESS, win, 0, 0);
        triggerCellAtPosition(0, RELEASE, win, 0, 0);
        assert(board != getActiveBoard());
    }
}
