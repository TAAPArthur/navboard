#include <X11/X.h>
#include <X11/Xlib.h>
#include <assert.h>
#include <math.h>
#include <stdio.h>
#include <string.h>
#include <unistd.h>
#include <xcb/xcb.h>
#include <xcb/xcb_ewmh.h>
#include <xcb/xcb_icccm.h>
#include <xcb/xtest.h>

#include "navboard.h"
#include "util.h"
#include "xutil.h"

Board boards[MAX_BOARDS];
int numBoards = 0;
static int activeIndex;

typedef struct {
    KeyGroup* keyGroup;
    Key* key;
    int16_t current[2];
} KeyState;
KeyState keyStates[255];


int isSlider(Key* key) {
    return key->min || key->max;
}

static int16_t getXPos() {
    return keyStates[0].current[0];
}

Board* getActiveBoard() {
    return &boards[activeIndex];
}

int setActiveBoard(const char* name) {
    for (int i = 0; i < numBoards; i++) {
        if (strcmp(name, boards[i].name) == 0) {
            activeIndex = i;
            return 1;
        }
    }
    return 0;
}

#define SET_DEFAULT(K,V) if (K == 0) K = V
void setDefaults(Key* key) {
    SET_DEFAULT(key->weight, 1);
    SET_DEFAULT(key->foreground, DEFAULT_TEXT_COLOR);
    SET_DEFAULT(key->background[0], DEFAULT_CELL_COLOR);
    SET_DEFAULT(key->background[1], key->flags & KEY_DISABLED ? DEFAULT_CELL_COLOR_DISABLED : DEFAULT_CELL_COLOR_PRESSED);
    for (int i = 0; defaults[i].keySym; i++) {
        if ((key->keySym == defaults[i].keySym) || (key->keySym && defaults[i].keySym == -1)) {
            if (!key->label)
                key->label = defaults[i].label;
            key->flags |= defaults[i].flags;
            if (!key->onPress)
                key->onPress = defaults[i].onPress;
            if (!key->onRelease)
                key->onRelease = defaults[i].onRelease;
            break;
        }
    }
}

char isRowSeperator(Key* key) {
    return !key->keySym && !key->label && !isSlider(key);

}

int getNumRows(KeyGroup* keyGroup) {
    int rows = 1;
    for (int i = 0; i < keyGroup->numKeys - 1; i++) {
        rows += isRowSeperator(&keyGroup->keys[i]);
    }
    return rows;
}

static int initKeys(KeyGroup* keyGroup) {
    Key* keys = keyGroup->keys;
    int n = keyGroup->numKeys;
    int i, j;

    for (i = 0, j = 0; i < n; i++) {
        if (isRowSeperator(&keys[i]))
            continue;
        setDefaults(&keys[i]);

        if (keys[i].loadValue && !(keys[i].flags & KEY_DISABLED)) {
            keys[i].loadValue(keyGroup, keys + i);
        }
        xcb_keysym_t* sym = NULL;
        keys[i].index = j++;
        if (keys[i].keySym) {
            char index;
            keys[i].keyCode = getKeyCode(keys[i].keySym, &sym, &index);
            if (index == 1)
                keys[i].flags |= SHIFT;
            if (!keys[i].label) {
                if (!getKeyChar(keys[i].keySym))
                    keys[i].label = XKeysymToString(keys[i].keySym);
            }

            if (sym && !keys[i].keySymShift) {
                keys[i].keySymShift = sym[1];
                if (!keys[i].altLabel)
                    keys[i].altLabel = getKeyChar(sym[1]) ? NULL: keys[i].label;
            }
        }
    }
    return i;
}

void initKeyGroup(KeyGroup* keyGroup) {
    assert(keyGroup);
    keyGroup->numRows = getNumRows(keyGroup);
    initKeys(keyGroup);
}
void initBoard(Board* board) {
    for (int n = 0; n < board->groupSize; n++) {
        initKeyGroup(&board->keyGroup[n]);
    }
}

void initBoards() {
    for (int i = 0; i < numBoards; i++) {
        initBoard(boards + i);
    }
}

KeyGroup* getKeyGroupForWindow(xcb_window_t win) {
    for (int i = 0; i < getActiveBoard()->groupSize; i++) {
        if (matchesWindow(getActiveBoard()->keyGroup[i].drawable, win))
            return &getActiveBoard()->keyGroup[i];
    }
    return NULL;
}

Key* findKey(KeyGroup* keyGroup, int x, int y) {
    assert(keyGroup);
    assert(keyGroup->numKeys);
    assert(keyGroup->rects);
    for (int i = 0, n = 0; i < keyGroup->numKeys; i++) {
        if (!isRowSeperator(&keyGroup->keys[i])) {
            if (keyGroup->rects[n].x <= x && x <= keyGroup->rects[n].x + keyGroup->rects[n].width &&
                keyGroup->rects[n].y <= y && y <= keyGroup->rects[n].y + keyGroup->rects[n].height) {
                return &keyGroup->keys[i];
            }
            n++;
        }
    }
    return NULL;
}

void computeRects(KeyGroup*keyGroup) {
    assert(keyGroup->numRows);
    int rows = keyGroup->numRows;
    int heightPerRow = (keyGroup->windowHeight) / rows;
    int width = keyGroup->windowWidth ;
    int numKeys = keyGroup->numKeys;
    keyGroup->rects = realloc(keyGroup->rects, sizeof(xcb_rectangle_t) * keyGroup->numKeys);
    int y = 0;
    int i, n;
    for (i = 0, n = 0; i < numKeys; i++) {
        if (isRowSeperator(&keyGroup->keys[i]))
            continue;
        int numCols = 0;
        for (int j = i; j < numKeys && !isRowSeperator(&keyGroup->keys[j]); j++)
            numCols += keyGroup->keys[j].weight;
        assert(numCols);
        int rem = numCols - width % numCols;
        for (int x = 0; i < numKeys && !isRowSeperator(&keyGroup->keys[i]); i++) {
            keyGroup->rects[n] = (xcb_rectangle_t) { x, y, keyGroup->keys[i].weight* width / numCols, heightPerRow };
            if (!rem) {
                keyGroup->rects[n].width += 1;
            } else
                rem--;
            x += keyGroup->rects[n].width;
            n++;
        }
        y += heightPerRow;
    }
    keyGroup->numRects = n;
}

static void redrawCells(KeyGroup* keyGroup) {
    assert(keyGroup);
    clear_drawable(keyGroup->drawable);

    char intBuffer[7];
    xcb_rectangle_t temp;
    for (int i = 0, n = 0; i < keyGroup->numKeys; i++) {
        if (!isRowSeperator(&keyGroup->keys[i])) {
            Key*key =&keyGroup->keys[i];
            updateBackground(keyGroup->drawable, key->background[key->pressed], &keyGroup->rects[key->index]);
            const char* rawLabel = (&key->label)[keyGroup->level];
            const char c = getKeyChar((&key->keySym)[keyGroup->level]);
            const char* label = rawLabel ? rawLabel : &c;
            int labelSize = rawLabel ? strlen(rawLabel) : 1;
            xcb_rectangle_t* rect = keyGroup->rects + n;
            if (isSlider(key)) {
                float percent = (key->value - key->min)/ (float)(key->max - key->min);
                drawSlider(keyGroup->drawable, key->background[1], percent, rect, &temp);
                rect = &temp;
                sprintf(intBuffer,"%d", key->value);
                label = intBuffer;
                labelSize = strlen(intBuffer);
            }
            drawText(keyGroup->drawable, labelSize, label,
                    key->foreground,
                    rect->x,
                    rect->y,
                    rect->width,
                    rect->height
                );

            n++;
        }
    }
    outlineRect(keyGroup->drawable, keyGroup->outlineColor, keyGroup->numRects, keyGroup->rects);

    clear_window(keyGroup->drawable);
}

void configureNotify(xcb_configure_notify_event_t* event) {
    KeyGroup* keyGroup = getKeyGroupForWindow(event->window);
    if (keyGroup) {
        keyGroup->windowHeight = event->height;
        keyGroup->windowWidth = event->width;
        onResize(keyGroup->drawable, event->width, event->height);
        computeRects(keyGroup);
        redrawCells(keyGroup);
    } else {
        setRootDims( event->width, event->height);
        for (int i = 0; i < getActiveBoard()->groupSize; i++) {
            KeyGroup* keyGroup =&getActiveBoard()->keyGroup[i];
            updateDockProperties(keyGroup->drawable, keyGroup->dockProperties);
        }
        xFlush();
    }
}

void exposeEvent(xcb_expose_event_t* event) {
    if (event->count == 0)
        redrawCells(getKeyGroupForWindow(event->window));
}


#define MAX(A, B) (A > B ? A:B)
#define MIN(A, B) (A < B ? A:B)
void dragSlider(KeyGroup*keyGroup, Key* key) {
    float percent = (getXPos() - keyGroup->rects[key->index].x)/ (float)keyGroup->rects[key->index].width;
    float delta = MAX(0, MIN(1, percent ));
    key->value = delta * (key->max - key->min) + key->min;
}

void triggerCell(KeyGroup*keyGroup, Key*key, TriggerType type) {
    if (!key || key->flags & KEY_DISABLED) {
        return;
    }
    if (hasLatchFlag(key) && type != PRESS )
        return;
    if (!isSlider(key)) {
        if (type == PRESS || type == RELEASE ) {
            key->pressed = hasLatchFlag(key) ? !key->pressed: type == PRESS;
        }
    } else if (type == DRAG) {
        dragSlider(keyGroup, key);
    }
    if (type == PRESS && key->onPress)
        key->onPress(keyGroup, key);
    else if (type == RELEASE && key->onRelease)
        key->onRelease(keyGroup, key);
    else if (type == DRAG && key->onDrag)
        key->onDrag(keyGroup, key);

}

static TriggerType getType(xcb_button_press_event_t* event) {
    switch(event->response_type) {
        case XCB_BUTTON_PRESS:
            return PRESS;
        case XCB_BUTTON_RELEASE:
            return RELEASE;
        case XCB_MOTION_NOTIFY:
            return DRAG;
    }
    return -1;
}

void triggerCellAtPosition(int id, TriggerType type, xcb_window_t win, int x, int y) {
    KeyGroup*keyGroup;
    Key* key;
    Board * board = getActiveBoard();
    if (type == PRESS) {
        keyGroup = getKeyGroupForWindow(win);
        if (!keyGroup)
            return;
        key = findKey(keyGroup, x, y);
        keyStates[id] = (KeyState) {keyGroup, key, {x, y} };
    } else {
        keyGroup = keyStates[id].keyGroup;
        key = keyStates[id].key;
        keyStates[id].current[0] = x;
        keyStates[id].current[1] = y;

        if (!key)
            return;
        if (type == RELEASE) {
            keyStates[id].keyGroup = NULL;
            keyStates[id].key = NULL;
        }
    }
    triggerCell(keyGroup, key, type);
    if (board == getActiveBoard()) {
        redrawCells(keyGroup);
    } else {
        keyStates[id].keyGroup = NULL;
        keyStates[id].key = NULL;
    }
}

void buttonEvent(xcb_button_press_event_t* event) {
    triggerCellAtPosition(0, getType(event), event->event, event->event_x, event->event_y);
}

void cleanupKeygroup(KeyGroup* keyGroup) {
    if (keyGroup->drawable)
        destroyWindow(keyGroup->drawable);
    if (keyGroup->rects) {
        free(keyGroup->rects);
        keyGroup->rects = NULL;
    }
}
void cleanupBoard(Board*board) {
    for (int i = 0; i < board->groupSize; i++) {
        cleanupKeygroup(board->keyGroup + i);
    }
}
void setupWindowsForBoard(Board*board) {
    setFont(board->fontName, board->fontSize);
    for (int i = 0; i < board->groupSize; i++) {
        KeyGroup* keyGroup =&board->keyGroup[i];
        keyGroup->drawable = createWindow(WINDOW_MASKS);
        setWindowProperties(keyGroup->drawable);
        updateDockProperties(keyGroup->drawable, keyGroup->dockProperties);
        mapWindow(keyGroup->drawable);
    }
}

int activateBoardByName(const char*name) {
    Board*board = getActiveBoard();
    if (setActiveBoard(name)) {
        cleanupBoard(board);
        setupWindowsForBoard(getActiveBoard());
        return 1;
    }
    return 0;
}

void activateBoard(KeyGroup*keyGroup, Key*key);

static void createBoardOfBoards() {
    int numColumns = 3;
    int numRows = (numBoards + numColumns - 1 ) / numColumns;
    int numKeys = numBoards + numRows - 1;
    Key* keys = calloc(numKeys, sizeof(Key));
    for (int i = 0, n = 0; i < numKeys; i++) {
        for (int c = 0; c < numColumns && i < numKeys; c++, i++) {
            keys[i].label = boards[n++].name;
            keys[i].onPress = activateBoard;
        }
    }
    boards[numBoards++] = CREATE_BOARD("board_of_boards", keys, numKeys);
}

void init() {
    initConnection();
    initBoards();
    addExtraEvent(getXFD(), processAllQueuedEvents);
}

void listBoards() {
    for (int i = 0; i < numBoards; i++)
        printf("%s\n", boards[i].name);
}

void usage() {
    printf("navboard [-l] [name]\n Where name is one of:\n");
    listBoards();
}

int __attribute__((weak)) main(int argc, const char* args[]) {
    createBoardOfBoards();
    int i;
    for (i = 1; i < argc; i++) {
        if (args[i][0] == '-')
            switch(args[i][1]) {
                case 'l':
                    listBoards();
                    exit(0);
                    break;
                case 'h':
                    usage();
                    exit(0);
                default:
                    break;
            }
        else
            break;
    }
    const char*activeBoardName = args[i] ? args[i] : getenv("NAVBOARD_DEFAULT") ? getenv("NAVBOARD_DEFAULT"): NULL;

    if (activeBoardName)
        if (!setActiveBoard(activeBoardName)) {
            printf("Unknown board %s; Valid names are:\n", activeBoardName);
            listBoards();
            exit(1);
        }
    init();
    grabSelection();
    setupWindowsForBoard(getActiveBoard());
    xFlush();
    while (1) {
        processEvents(-1);
    }
}
