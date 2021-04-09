#ifndef MYVKBD_H
#define MYVKBD_H
#include <X11/X.h>
#include <X11/keysym.h>
#include <X11/keysymdef.h>
#include <xcb/xcb.h>
#include "config.h"

#define LEN(x)         (sizeof x / sizeof x[0])

#define SHIFT   (1<<0)
#define LOCK    (1<<1)
#define MOD     (1<<2)
#define LATCH   (1<<3)

typedef struct xdrawable XDrawable;

typedef uint32_t Color;
typedef struct {
    KeySym keySym;
    KeySym keySymShift;
    const char* label;
    const char* altLabel;
    unsigned int weight;
    char flags;

    void(*onPress)();
    void(*onRelease)();

    Color foreground;
    Color background[2];

    // should not be manually set
    char pressed;
    int index;
    KeyCode keyCode;
} Key;

typedef enum DockType {
    LEFT, RIGHT, TOP, BOTTOM
} DockType;

typedef struct {
    Key* keys;
    int numKeys;
    int numRows;
    xcb_rectangle_t* rects;
    int numRects;
    XDrawable* drawable;
    short windowWidth;
    short windowHeight;
    int thicknessPercent;
    DockType dockType;
    short start;
    short end;
    uint32_t outlineColor;
    int level;
} KeyGroup;

typedef struct Layout {
    int groupSize;
    KeyGroup keyGroup[1];
    const char*name;
    const char*fontName;
} Board;
extern Board boards[MAX_BOARDS];
extern int numBoards;

KeyGroup* getKeyGroupForWindow(xcb_window_t win);


void computeRects(KeyGroup*keyGroup);
void initBoard(Board* board);

void initKeyGroup(KeyGroup* keyGroup);

char isRowSeperator(Key* key);

Key* findKey(KeyGroup* keyGroup, int x, int y);

void sendKeyPress(Key*key);

void sendKeyPressWithModifiers(KeyGroup*keyGroup, Key*key);
void sendKeyReleaseWithModifiers(KeyGroup*keyGroup, Key*key);
void sendKeyRelease(Key*key);


void setupWindowsForBoard(Board*board);
void triggerCell(KeyGroup*keyGroup, Key*key, char press);

extern void(*xEventHandlers[])();
extern Key defaults[];

void buttonEvent(xcb_button_press_event_t* event);
void configureNotify(xcb_configure_notify_event_t* event);
void exposeEvent(xcb_expose_event_t* event);
void shiftKeys(KeyGroup*keyGroup, Key*key);

#define __CAT(x, y) x ## y
#define _CAT(x, y) __CAT(x, y)

#define CREATE_BOARD(NAME, BOARD, NUM_KEYS) (Board)\
{1, {(KeyGroup){BOARD, NUM_KEYS, .dockType=DEFAULT_DOCK_TYPE, .thicknessPercent = DEFAULT_THICKNESS}}, .name=NAME, .fontName=DEFAULT_FONT }

//##define REGISTER(B) REGISTER(B, B)
#define REGISTER(NAME, BOARD) \
__attribute__((constructor)) void __CAT(setup, NAME) () { boards[numBoards++] = CREATE_BOARD(#NAME, BOARD, LEN(BOARD));}


#endif
