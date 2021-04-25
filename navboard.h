#ifndef NAVBOARD_H
#define NAVBOARD_H
#include <X11/X.h>
#include <X11/keysym.h>
#include <X11/keysymdef.h>
#include <xcb/xcb.h>
#include "common.h"
#include "config.h"


#define SHIFT          (1<<0)
#define LOCK           (1<<1)
#define MOD            (1<<2)
#define LATCH          (1<<3)
#define KEY_DISABLED   (1<<4)

typedef enum {
    RELEASE, PRESS, DRAG
} TriggerType;

typedef union {
    const char* s;
    int i;
}Arg;

typedef struct {
    KeySym keySym;
    KeySym keySymShift;
    const char* label;
    const char* altLabel;

    int16_t weight;

    int16_t min;
    int16_t max;
    int16_t value;

    char flags;

    void(*onPress)();
    void(*onRelease)();
    void(*onDrag)();

    Color foreground;
    Color background[2];
    Arg arg;

    // should not be manually set
    char pressed;
    int index;
    KeyCode keyCode;
} Key;

typedef struct {
    Key* keys;
    int numKeys;
    int numRows;
    xcb_rectangle_t* rects;
    int numRects;
    XDrawable* drawable;
    short windowWidth;
    short windowHeight;
    int level;
    DockProperties dockProperties;
    uint32_t outlineColor;
} KeyGroup;

typedef struct Layout {
    int groupSize;
    KeyGroup keyGroup[1];
    const char*name;
    const char*fontName;
} Board;
extern Board boards[MAX_BOARDS];
extern int numBoards;

static inline char hasLatchFlag(const Key* key) {
    return key->flags & LATCH;
}

static inline char isModifier(const Key* key) {
    return key->flags & MOD;
}

int activateBoardByName(const char*name);

KeyGroup* getKeyGroupForWindow(xcb_window_t win);

void computeRects(KeyGroup*keyGroup);
void initBoard(Board* board);

Board* getActiveBoard();
void cleanupBoard(Board*board);
void cleanupKeygroup(KeyGroup* keyGroup);

void initKeyGroup(KeyGroup* keyGroup);

char isRowSeperator(Key* key);

Key* findKey(KeyGroup* keyGroup, int x, int y);

void setupWindowsForBoard(Board*board);
void triggerCell(KeyGroup*keyGroup, Key*key, TriggerType type) ;

extern Key defaults[];

void buttonEvent(xcb_button_press_event_t* event);
void configureNotify(xcb_configure_notify_event_t* event);
void exposeEvent(xcb_expose_event_t* event);

#define __CAT(x, y) x ## y
#define _CAT(x, y) __CAT(x, y)


#define CREATE_KEYGROUP(KEYS, NUM_KEYS, TYPE, START, END, THICKNESS, OUTLINE_COLOR) (KeyGroup)\
    {KEYS, NUM_KEYS, .dockProperties={.type=TYPE, .start=START, .end=END, .thicknessPercent = THICKNESS}, .outlineColor=OUTLINE_COLOR}

#define CREATE_DEFAULT_KEYGROUP(KEYS, NUM_KEYS) \
    CREATE_KEYGROUP(KEYS, NUM_KEYS, DEFAULT_DOCK_TYPE, 0, 0, DEFAULT_THICKNESS, DEFAULT_OUTLINE_COLOR)

#define CREATE_BOARD(NAME, KEYS, NUM_KEYS) (Board)\
{1, {CREATE_DEFAULT_KEYGROUP(KEYS, NUM_KEYS)}, .name=NAME, .fontName=DEFAULT_FONT }

#define REGISTER(NAME, KEYS) \
    REGISTER_BOARD(NAME, CREATE_BOARD(#NAME, KEYS, LEN(KEYS)))

#define REGISTER_BOARD(NAME, BOARD) \
__attribute__((constructor)) void __CAT(setup, NAME) () { boards[numBoards++] = BOARD;}

#endif
