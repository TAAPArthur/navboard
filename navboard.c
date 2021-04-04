#include <X11/X.h>
#include <X11/Xlib.h>
#include <assert.h>
#include <stdio.h>
#include <string.h>
#include <unistd.h>
#include <xcb/xcb.h>
#include <xcb/xcb_ewmh.h>
#include <xcb/xcb_icccm.h>
#include <xcb/xtest.h>

#include "navboard.h"
#include "xutil.h"
#include "util.h"

Board boards[MAX_BOARDS];
int numBoards;
static int activeIndex;

Board* getActiveBoard() {
    return &boards[activeIndex];
}
int setActiveBoard(const char* name) {
    for(int i = 0; i < numBoards; i++) {
        if(strcmp(name, boards[i].name) ==0){
            activeIndex = i;
            return 1;
        }
    }
    return 0;
}

void setDefaults(Key* key) {
    if(key->weight == 0)
        key->weight = 1;
    for(int i = 0; defaults[i].keySym; i++) {
        if((key->keySym == defaults[i].keySym) || (key->keySym && defaults[i].keySym == -1)) {
            if(!key->label)
                key->label = defaults[i].label;
            key->flags |= defaults[i].flags;
            if(!key->onPress)
                key->onPress = defaults[i].onPress;
            if(!key->onRelease)
                key->onRelease = defaults[i].onRelease;
            break;
        }
    }
}

char isRowSeperator(Key* key) {
    return !key->keySym && !key->label;

}

int getNumRows(KeyGroup* keyGroup) {
    int rows = 1;
    for(int i = 0; i < keyGroup->numKeys; i++) {
        rows+=isRowSeperator(&keyGroup->keys[i]);
    }
    return rows;
}

int initKeys(Key* keys, int n, int level) {
    int i, j;
    for(i = 0, j = 0; i < n; i++) {
        if(isRowSeperator(&keys[i]))
            continue;
        setDefaults(&keys[i]);
        xcb_keysym_t* sym = NULL;
        keys[i].index = j++;
        if(keys[i].keySym) {
            keys[i].keyCode = getKeyCode(keys[i].keySym, &sym);
            if(level && sym && sym[level])
                keys[i].keySym = sym[level];
            if(!keys[i].label) {
                keys[i].c = getKeyChar(keys[i].keySym);
                if(!keys[i].c)
                    keys[i].label = XKeysymToString(keys[i].keySym);
            }
        }
    }
    return i;
}

void initKeyGroup(KeyGroup* keyGroup) {
    keyGroup->numRows = getNumRows(keyGroup);
    /*
    if(!layouts[i].shiftKeys) {
        Key* shiftKeys = malloc(numKeys * sizeof(Key));
        memcpy(shiftKeys, layouts[i].keys, numKeys * sizeof(Key));
        layouts[i].shiftKeys = shiftKeys;
        initKeys(keyboard_mapping, layouts[i].shiftKeys, numKeys, 1);
    }
    else {
        initKeys(keyboard_mapping, layouts[i].shiftKeys, numKeys, 0);
    }
    */
    initKeys(keyGroup->keys, keyGroup->numKeys, 0);
}
void initBoard(Board* board) {
    for(int n = 0; n < board->groupSize; n++) {
        initKeyGroup(&board->keyGroup[n]);
    }
}

void initBoards() {
    for(int i = 0; i < numBoards; i++) {
        initBoard(boards + i);
    }
}

KeyGroup* getKeyGroupForWindow(xcb_window_t win) {
    for(int i = 0; i < getActiveBoard()->groupSize; i++) {
        if(getActiveBoard()->keyGroup[i].win == win)
            return &getActiveBoard()->keyGroup[i];
    }
    return NULL;
}

Key* findKey(KeyGroup* keyGroup, int x, int y) {
    assert(keyGroup);
    assert(keyGroup->numKeys);
    assert(keyGroup->rects);
    for(int i = 0, n = 0; i < keyGroup->numKeys; i++) {
        if(!isRowSeperator(&keyGroup->keys[i])) {
            if(x >= keyGroup->rects[n].x && x <= keyGroup->rects[n].x + keyGroup->rects[n].width &&
                y >= keyGroup->rects[n].y && y <= keyGroup->rects[n].y + keyGroup->rects[n].height) {
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
    int numKeys= keyGroup->numKeys;
    keyGroup->rects = realloc(keyGroup->rects, sizeof(xcb_rectangle_t) * keyGroup->numKeys);
    int y = 0;
    int i, n;
    for(i = 0, n = 0; i < numKeys; i++) {
        if(isRowSeperator(&keyGroup->keys[i]))
            continue;
        int numCols = 0;
        for(int j = i; j < numKeys && !isRowSeperator(&keyGroup->keys[j]); j++)
            numCols += keyGroup->keys[j].weight;
        assert(numCols);
        int rem = width % numCols;
        for(int x = 0; i < numKeys && !isRowSeperator(&keyGroup->keys[i]); i++) {
            keyGroup->rects[n] = (xcb_rectangle_t) { x, y, keyGroup->keys[i].weight* width / numCols, heightPerRow };
            if(rem) {
                keyGroup->rects[n].width += 1;
                rem--;
            }
            x += keyGroup->rects[n].width;
            n++;
        }
        y += heightPerRow;
    }
    keyGroup->numRects = n;
}

static void redrawCells(KeyGroup* keyGroup) {
    assert(keyGroup);
    for(int i = 0, n = 0; i < keyGroup->numKeys; i++) {
        if(!isRowSeperator(&keyGroup->keys[i])){
            const char* label = keyGroup->keys[i].label;
            drawText(keyGroup->win, label ? strlen(label) : 1, keyGroup->rects[n].x + keyGroup->rects[n].width / 2,
                keyGroup->rects[n].y +  keyGroup->rects[n].height / 2,
                label ? label : &keyGroup->keys[i].c);
            n++;
        }
    }
    outlineRect(keyGroup->win, keyGroup->numRects, keyGroup->rects);
}

void configureNotify(xcb_configure_notify_event_t* event) {
    KeyGroup* keyGroup = getKeyGroupForWindow(event->window);
    if(keyGroup) {
        keyGroup->windowHeight = event->height;
        keyGroup->windowWidth = event->width;

        computeRects(keyGroup);
        redrawCells(keyGroup);
    }
}

void exposeEvent(xcb_expose_event_t* event) {
    if(event->count == 0)
        redrawCells(getKeyGroupForWindow(event->window));
}

char hasLatchFlag(Key* key) {
    return key->flags & LATCH;
}

char isModifier(Key* key) {
    return key->flags & MOD;
}

void triggerCell(KeyGroup*keyGroup, Key*key, char press) {
    if(!key) {
        return;
    }
    if(hasLatchFlag(key) && !press)
        return;
    if(press && key->onPress)
        key->onPress(keyGroup, key);
    else if(!press && key->onRelease) {
        key->onRelease(keyGroup, key);
    }

    key->pressed = hasLatchFlag(key) ? !key->pressed: press;

    updateBackground(keyGroup->win, key->pressed, &keyGroup->rects[key->index]);
}

void pressAllModifiers(KeyGroup*keyGroup, int press) {
    for(int i = 0; i < keyGroup->numKeys; i++) {
        if(keyGroup->keys[i].pressed && isModifier(&keyGroup->keys[i])) {
            sendKeyEvent(press, keyGroup->keys[i].keyCode);
            if(!press && (keyGroup->keys[i].flags & LATCH)&& !(keyGroup->keys[i].flags & LOCK)) {
                keyGroup->keys[i].pressed = 0;
                updateBackground(keyGroup->win, 0, &keyGroup->rects[keyGroup->keys[i].index]);
            }
        }
    }
}

void sendKeyPress(Key*key){
    sendKeyEvent(1, key->keyCode);
}

void sendKeyRelease(Key*key){
    sendKeyEvent(0, key->keyCode);
}

void sendKeyReleaseWithModifiers(KeyGroup*keyGroup, Key*key){
    sendKeyRelease(key);
    pressAllModifiers(keyGroup, 0);
}

void sendKeyPressWithModifiers(KeyGroup*keyGroup, Key*key){
    pressAllModifiers(keyGroup, 1);
    sendKeyPress(key);
}

void buttonEvent(xcb_button_press_event_t* event) {
    printf("Button event %d\n", event->detail);
    char press = event->response_type == XCB_BUTTON_PRESS;
    KeyGroup*keyGroup = getKeyGroupForWindow(event->event);
    Key* key = findKey(keyGroup, event->event_x, event->event_y);
    triggerCell(keyGroup, key, press);
    redrawCells(keyGroup);
}


void setupWindowsForBoard(Board*board) {
    for(int i = 0; i < board->groupSize; i++) {
        KeyGroup* keyGroup=&board->keyGroup[i];
        keyGroup->win=createWindow();
        setWindowProperties(keyGroup->win);
        updateDockProperties(keyGroup->win, keyGroup->dockType, keyGroup->thicknessPercent, keyGroup->start, keyGroup->end);
        mapWindow(keyGroup->win);
    }
}

void init() {
    initConnection();
    initBoards();
    addExtraEvent(getXFD(), processAllQueuedEvents);
}

    int dumpKeyCodes() ;
int __attribute__((weak)) main(int argc, const char* args[]) {
    if(argc >1)
        if(!setActiveBoard(args[1]))
            exit(1);
    init();
    setupWindowsForBoard(getActiveBoard());
    xFlush();
    while(1) {
        processEvents(-1);
    }
}
