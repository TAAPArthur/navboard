#define _XOPEN_SOURCE 500
#include <xcb/xcb.h>
#include <xcb/xcb_ewmh.h>
#include <xcb/xcb_icccm.h>
#include <X11/X.h>
#include <unistd.h>
#include <assert.h>
#include <stdio.h>
#include <X11/Xlib.h>
#include <string.h>
#include <xcb/xtest.h>


#include "myvkbd.h"
#include "config.h"

xcb_connection_t* dis;
xcb_window_t root;
xcb_ewmh_connection_t* ewmh;
xcb_screen_t* screen;
xcb_gcontext_t  gc;
xcb_gcontext_t  gcPressed[2];
short windowHeight, windowWidth;
float heightPercent = .20;
xcb_window_t win;
DockType dockType = BOTTOM;
const xcb_setup_t* xSetup;

Layout* activeLayout = layouts;
int activeLevel = 0;
Key* getActiveKeys() {
    return (&activeLayout->keys)[activeLayout->level];
}

void sendKeyEvent(char press, Key* key) {
    printf("Sending key press %s %c %d %d\n", key->label, key->c, key->keyCode, press);
    xcb_test_fake_input(dis, press ? XCB_KEY_PRESS : XCB_KEY_RELEASE, key->keyCode, XCB_CURRENT_TIME, XCB_NONE, 0, 0, 0);
}

struct {
    xcb_keysym_t sym;
    const char* label;
    char flags;
} DEFAULTS_TABLE[] = {
    {XK_Alt_L, "Alt", MOD},
    {XK_Alt_R, "Alt", MOD},
    {XK_Control_L, "Ctrl", MOD},
    {XK_Control_R, "Ctrl", MOD},
    {XK_Hyper_L, "Hyper", MOD},
    {XK_Hyper_R, "Hyper", MOD},
    {XK_Meta_L, "Meta", MOD},
    {XK_Meta_R, "Meta", MOD},
    {XK_Shift_L, "Shift", SHIFT | MOD },
    {XK_Shift_R, "Shift", SHIFT | MOD },
    {XK_Shift_Lock, "Shift", SHIFT | MOD | LOCK },
    {XK_Super_L, "Super", MOD},
    {XK_Super_R, "Super", MOD},
};
void setDefaults(Key* key) {
    // set default values;
    if(!key->keySymShift)
        key->keySymShift = key->keySym;
    if(key->weight == 0)
        key->weight = 1;
    if(!key->label) {
        for(int i = 0; i < LEN(DEFAULTS_TABLE); i++) {
            if(key->keySym == DEFAULTS_TABLE[i].sym) {
                key->label = DEFAULTS_TABLE[i].label;
                key->flags |= DEFAULTS_TABLE[i].flags;
            }
        }
    }
}


int getNumRows(int* n) {
    int rows = 1;
    int i;
    for(i = 0; ; i++) {
        if(getActiveKeys()[i].keySym == 0)
            if(getActiveKeys()[i + 1].keySym == 0)
                break;
            else
                rows++;
    }
    if(n)
        *n = i + 1;
    return rows;
}
void initConnection() {
    dis = xcb_connect(NULL, NULL);
    ewmh = (xcb_ewmh_connection_t*)malloc(sizeof(xcb_ewmh_connection_t));
    xcb_intern_atom_cookie_t* cookie = xcb_ewmh_init_atoms(dis, ewmh);
    xcb_ewmh_init_atoms_replies(ewmh, cookie, NULL);
    screen = ewmh->screens[0];
    root = screen->root;
    win = xcb_generate_id(dis);
    int mask = XCB_EVENT_MASK_KEYMAP_STATE | XCB_EVENT_MASK_BUTTON_PRESS | XCB_EVENT_MASK_BUTTON_RELEASE |
        XCB_EVENT_MASK_EXPOSURE |
        XCB_EVENT_MASK_STRUCTURE_NOTIFY;
    uint32_t windowValues[] = {0x14313d, mask};
    xcb_create_window(dis, XCB_COPY_FROM_PARENT, win, root, 0, screen->height_in_pixels * (1 - heightPercent),
        screen->width_in_pixels,
        screen->height_in_pixels * heightPercent, 0, XCB_WINDOW_CLASS_INPUT_OUTPUT,
        screen->root_visual,
        XCB_CW_BACK_PIXEL | XCB_CW_EVENT_MASK, windowValues);
    gc = xcb_generate_id(dis);
    gcPressed[0] = xcb_generate_id(dis);
    gcPressed[1] = xcb_generate_id(dis);
    uint32_t value[]  = { 0xfefefe, 0x14313d };
    xcb_create_gc(dis, gc, root, XCB_GC_FOREGROUND | XCB_GC_BACKGROUND, value);
    uint32_t value2[]  = { 0x14313d };
    uint32_t value3[]  = { 0x005577};
    xcb_create_gc(dis, gcPressed[0], root, XCB_GC_FOREGROUND | XCB_GC_BACKGROUND, value2);
    xcb_create_gc(dis, gcPressed[1], root, XCB_GC_FOREGROUND | XCB_GC_BACKGROUND, value3);
    xSetup      = xcb_get_setup(dis);
}

int getKeyCode(xcb_get_keyboard_mapping_reply_t* keyboard_mapping, KeySym targetSym, xcb_keysym_t** foundSym) {
    int          nkeycodes = keyboard_mapping->length / keyboard_mapping->keysyms_per_keycode;
    xcb_keysym_t* keysyms  = (xcb_keysym_t*)(keyboard_mapping +
            1);  // `xcb_keycode_t` is just a `typedef u8`, and `xcb_keysym_t` is just a `typedef u32`
    for(int keycode_idx = 0; keycode_idx < nkeycodes; ++keycode_idx) {
        for(int keysym_idx = 0; keysym_idx < keyboard_mapping->keysyms_per_keycode; ++keysym_idx) {
            xcb_keysym_t*  sym = &keysyms[keysym_idx + keycode_idx * keyboard_mapping->keysyms_per_keycode];
            if(*sym == targetSym) {
                *foundSym = sym;
                return xSetup->min_keycode + keycode_idx;
            }
        }
    }
    return 0;
}

char getKeyChar(xcb_keysym_t sym) {
    if(XK_space <= sym && sym <= XK_asciitilde) {
        return sym  - XK_space + ' ';
    }
    return 0;
}
int initKeys(xcb_get_keyboard_mapping_reply_t* keyboard_mapping, Key* keys, int n, int level) {
    int i, j;
    for(i = 0, j = 0; i < n; i++) {
        if(!keys[i].keySym)
            continue;
        setDefaults(&keys[i]);
        xcb_keysym_t* sym = NULL;
        keys[i].index = j++;
        keys[i].keyCode = getKeyCode(keyboard_mapping, keys[i].keySym, &sym);
        //printf("%d %ld %p\n", keys[i].keyCode, keys[i].keySym, sym);
        if(level && sym && sym[level])
            keys[i].keySym = sym[level];
        if(!keys[i].label) {
            keys[i].c = getKeyChar(keys[i].keySym);
            if(!keys[i].c)
                keys[i].label = XKeysymToString(keys[i].keySym);
        }
        //sendKeyEvent(0, &keys[i]);
    }
    return i;
}
void initLayouts() {
    xcb_get_keyboard_mapping_reply_t* keyboard_mapping;
    keyboard_mapping = xcb_get_keyboard_mapping_reply(dis, xcb_get_keyboard_mapping(dis, xSetup->min_keycode,
                xSetup->max_keycode - xSetup->min_keycode + 1), NULL);
    for(int i = 0; i < LEN(layouts); i++) {
        int numKeys;
        getNumRows(&numKeys);
        numKeys++;
        if(!layouts[i].shiftKeys) {
            Key* shiftKeys = malloc(numKeys * sizeof(Key));
            memcpy(shiftKeys, layouts[i].keys, numKeys * sizeof(Key));
            layouts[i].shiftKeys = shiftKeys;
            initKeys(keyboard_mapping, layouts[i].shiftKeys, numKeys, 1);
        }
        else {
            initKeys(keyboard_mapping, layouts[i].shiftKeys, numKeys, 0);
        }
        initKeys(keyboard_mapping, layouts[i].keys, numKeys, 0);
    }
    free(keyboard_mapping);
}

void updateDockProperties() {
    int dockProperties[4] = {0};
    dockProperties[dockType] = screen->height_in_pixels * heightPercent;
    xcb_ewmh_set_wm_strut(ewmh, win, dockProperties[0], dockProperties[1], dockProperties[2], dockProperties[3]);
}

xcb_rectangle_t* rects;
int numKeys;

Key* findKey(int x, int y) {
    for(int i = 0, n = 0; i < numKeys; i++) {
        if(getActiveKeys()[i].keySym) {
            if(x > rects[n].x && x < rects[n].x + rects[n].width &&
                y > rects[n].y && y < rects[n].y + rects[n].height) {
                return &getActiveKeys()[i];
            }
            n++;
        }
    }
    return NULL;
}

int computeRects() {
    int rows = getNumRows(&numKeys);
    int heightPerRow = windowHeight / rows;
    int y = 0;
    int i, n;
    for(i = 0, n = 0; i < numKeys; i++) {
        int numCols = 0;
        for(int j = i; j < numKeys && getActiveKeys()[j].keySym != 0; j++)
            numCols += getActiveKeys()[j].weight;
        int rem = windowWidth % numCols;
        for(int x = 0; i < numKeys && getActiveKeys()[i].keySym != 0; i++) {
            rects[n] = (xcb_rectangle_t) { x, y, getActiveKeys()[i].weight* windowWidth / numCols, heightPerRow };
            if(rem) {
                rects[n].width += 1;
                rem--;
            }
            x += rects[n].width;
            n++;
        }
        y += heightPerRow;
    }
    return n;
}
void updatekeys(void) {
    getNumRows(&numKeys);
    rects = realloc(rects, sizeof(xcb_rectangle_t) * numKeys);
    int numRects = computeRects();
    for(int i = 0, n = 0; i < numKeys; i++) {
        if(getActiveKeys()[i].keySym) {
            const char* label = getActiveKeys()[i].label;
            //printf("%s %c %d %d %d %d\n", label, getActiveKeys()[i].c, x, y, rects[n].width, heightPerRow);
            xcb_image_text_8(dis, label ? strlen(label) : 1, win, gc, rects[n].x + rects[n].width / 2,
                rects[n].y +  rects[n].height / 2,
                label ? label : &getActiveKeys()[i].c);
            n++;
        }
    }
    //xcb_rectangle_t winGeo = {0, 0, windowWidth, windowHeight};
    //xcb_poly_fill_rectangle(dis, win, gc, 1, &winGeo);
    xcb_poly_rectangle(dis, win, gc, numRects, rects);
}

void setup(void) {
    xcb_icccm_wm_hints_t hints = {};
    xcb_icccm_wm_hints_set_input(&hints, 0);
    xcb_icccm_set_wm_hints(dis, win, &hints);
    const char classInstance[] = "myvkbd\0myvkbd";
    xcb_icccm_set_wm_class(dis, win, LEN(classInstance), classInstance);
    xcb_ewmh_client_source_type_t source = XCB_EWMH_CLIENT_SOURCE_TYPE_NORMAL;
    xcb_ewmh_set_wm_window_type(ewmh, win, 1, &ewmh->_NET_WM_WINDOW_TYPE_DOCK);
    xcb_ewmh_request_change_wm_state(ewmh, 0, win, XCB_EWMH_WM_STATE_ADD, ewmh->_NET_WM_STATE_STICKY,
        ewmh->_NET_WM_STATE_ABOVE, source);
    updateDockProperties();
    xcb_flush(dis);
    updatekeys();
    xcb_map_window(dis, win);
    xcb_flush(dis);
}

void configureNotify(xcb_configure_notify_event_t* event) {
    if(event->window == win) {
        windowHeight = event->height;
        windowWidth = event->width;
        updatekeys();
    }
}
void expose(xcb_expose_event_t* event) {
    if(event->count == 0 && (event->window == win))
        updatekeys();
}
void updateBackground(int press, Key* k) {
    int n = k->index;
    xcb_poly_fill_rectangle(dis, win, gcPressed[press], 1, &rects[n]);
}
char isLockModifier(Key* key) {
    return key->flags & LOCK;
}
char hasShiftFlag(Key* key) {
    return key->flags & SHIFT;
}
char isModifier(Key* key) {
    return key->flags & MOD;
}
void buttonEvent(xcb_button_press_event_t* event) {
    char press = event->response_type == XCB_BUTTON_PRESS;
    Key* key = findKey(event->event_x, event->event_y);
    if(key) {
        if(isModifier(key)) {
            if(!press)
                return;
            press = !key->pressed;
        }
        printf("%s %c %d %d", key->label, key->c, press, hasShiftFlag(key));
        updateBackground(press, key);
        sendKeyEvent(press, key);
        key->pressed = press;
        if(!press && !isModifier(key)) {
            for(int i = 0; i < numKeys; i++) {
                if(getActiveKeys()[i].pressed && isModifier(&getActiveKeys()[i]) && (!isLockModifier(&getActiveKeys()[i]))) {
                    sendKeyEvent(0, &getActiveKeys()[i]);
                    getActiveKeys()[i].pressed = 0;
                    updateBackground(press, &getActiveKeys()[i]);
                }
            }
        }
        if(hasShiftFlag(key)) {
            printf("shit layout");
            activeLayout->level = key->pressed;
        }
    }
    updatekeys();
}
int main(void) {
    initConnection();
    initLayouts();
    setup();
    while(1) {
        xcb_generic_event_t* event = xcb_wait_for_event(dis);
        //printf("%d\n", event->response_type);
        switch(event->response_type) {
            case XCB_BUTTON_PRESS:
            case XCB_BUTTON_RELEASE:
                buttonEvent((void*)event);
                break;
            case XCB_EXPOSE:
                expose((void*)event);
                break;
            case XCB_CONFIGURE_NOTIFY:
                configureNotify((void*)event);
                break;
        }
        xcb_flush(dis);
        free(event);
    }
}

