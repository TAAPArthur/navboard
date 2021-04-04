
#include <X11/keysymdef.h>
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
#include "xutil.h"
#include "config.h"
xcb_connection_t* dis;
xcb_window_t root;
xcb_ewmh_connection_t* ewmh;
xcb_screen_t* screen;
xcb_gcontext_t  gc;
xcb_gcontext_t  gcPressed[2];
const xcb_setup_t* xSetup;
xcb_get_keyboard_mapping_reply_t* keyboard_mapping;

void initConnection() {
    dis = xcb_connect(NULL, NULL);
    ewmh = (xcb_ewmh_connection_t*)malloc(sizeof(xcb_ewmh_connection_t));
    xcb_intern_atom_cookie_t* cookie = xcb_ewmh_init_atoms(dis, ewmh);
    xcb_ewmh_init_atoms_replies(ewmh, cookie, NULL);
    screen = ewmh->screens[0];
    root = screen->root;

    gc = xcb_generate_id(dis);
    gcPressed[0] = xcb_generate_id(dis);
    gcPressed[1] = xcb_generate_id(dis);
    uint32_t value[]  = { 0xfefefe, 0x14313d };
    xcb_create_gc(dis, gc, root, XCB_GC_FOREGROUND | XCB_GC_BACKGROUND, value);
    uint32_t value2[]  = { 0x14313d, 0 };
    uint32_t value3[]  = { 0x005577, 0};
    xcb_create_gc(dis, gcPressed[0], root, XCB_GC_FOREGROUND | XCB_GC_BACKGROUND, value2);
    xcb_create_gc(dis, gcPressed[1], root, XCB_GC_FOREGROUND | XCB_GC_BACKGROUND, value3);
    xSetup      = xcb_get_setup(dis);
    keyboard_mapping = xcb_get_keyboard_mapping_reply(dis, xcb_get_keyboard_mapping(dis, xSetup->min_keycode,
                xSetup->max_keycode - xSetup->min_keycode + 1), NULL);
}

void closeConnection() {
}

xcb_window_t createWindow(){
    xcb_window_t win = xcb_generate_id(dis);
    uint32_t windowValues[] = {0x14313d, WINDOW_MASKS};
    xcb_create_window(dis, XCB_COPY_FROM_PARENT, win, root, 0, 0, 10, 10,
        0, XCB_WINDOW_CLASS_INPUT_OUTPUT,
        screen->root_visual,
        XCB_CW_BACK_PIXEL | XCB_CW_EVENT_MASK, windowValues);
    return win;
}

void mapWindow(xcb_window_t win){
    xcb_map_window(dis, win);
}

void updateDockProperties(xcb_window_t win, DockType dockType, int thicknessPercent, int start, int end) {
    int dockProperties[4] = {0};
    if(end == 0) {
        end = (&screen->width_in_pixels)[dockType < TOP];
    }
    int x,y;
    short width, height;
    if(dockType < TOP) {
        width = dockProperties[dockType] = screen->width_in_pixels * thicknessPercent / 100;
        height = end - start;
    } else {
        height = dockProperties[dockType] = screen->height_in_pixels * thicknessPercent / 100;
        width = end - start;
    }

    xcb_ewmh_set_wm_strut(ewmh, win, dockProperties[0], dockProperties[1], dockProperties[2], dockProperties[3]);
    switch(dockType) {
        case TOP:
        case LEFT:
            x = y =0;
            break;
        case RIGHT:
            y = 0;
            x = screen->width_in_pixels -  width;
            break;
        case BOTTOM:
            y = screen->height_in_pixels - height;
            x = 0;
            break;
    }

    uint32_t mask = XCB_CONFIG_WINDOW_X | XCB_CONFIG_WINDOW_Y | XCB_CONFIG_WINDOW_WIDTH | XCB_CONFIG_WINDOW_HEIGHT;
    uint32_t values[4] = {x, y, width, height};
    xcb_configure_window(dis, win, mask, values);

}

void sendKeyEvent(char press, KeyCode keyCode) {
    assert(keyCode);
    xcb_test_fake_input(dis, press ? XCB_KEY_PRESS : XCB_KEY_RELEASE, keyCode, XCB_CURRENT_TIME, root, 0, 0, 0);
}



char getKeyChar(xcb_keysym_t sym) {
    if(XK_space <= sym && sym <= XK_asciitilde) {
        return sym  - XK_space + ' ';
    }
    return 0;
}

int dumpKeyCodes() {
    int          nkeycodes = keyboard_mapping->length / keyboard_mapping->keysyms_per_keycode;
    printf("NumKeyCodes %d %d\n", nkeycodes, keyboard_mapping->keysyms_per_keycode);
    xcb_keysym_t* keysyms  = (xcb_keysym_t*)(keyboard_mapping +
            1);  // `xcb_keycode_t` is just a `typedef u8`, and `xcb_keysym_t` is just a `typedef u32`
    for(int keycode_idx = 0; keycode_idx < nkeycodes; ++keycode_idx) {
        for(int keysym_idx = 0; keysym_idx < keyboard_mapping->keysyms_per_keycode; ++keysym_idx) {
            xcb_keysym_t  sym = keysyms[keysym_idx + keycode_idx * keyboard_mapping->keysyms_per_keycode];
            printf("KeyCode: %d Sym: %d String: %s %d\n", xSetup->min_keycode + keycode_idx, sym, XKeysymToString(sym), sym == XK_Shift_R);
        }
    }
    return 0;
}

int getKeyCode(KeySym targetSym, xcb_keysym_t** foundSym) {
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
    printf("Could not find %d %s\n",targetSym, XKeysymToString(targetSym));
    assert(0);
    return 0;
}

void drawText(xcb_window_t win, int numChars, int x, int y, const char*str) {
    xcb_image_text_8(dis, numChars, win, gc, x, y, str);
}

void outlineRect(xcb_window_t win, int numRects, const xcb_rectangle_t* rects) {
    xcb_poly_rectangle(dis, win, gc, numRects, rects);
}
void setWindowProperties(xcb_window_t win) {
    xcb_icccm_wm_hints_t hints = {};
    xcb_icccm_wm_hints_set_input(&hints, 0);
    xcb_icccm_set_wm_hints(dis, win, &hints);
    const char classInstance[] = "navboard\0navboard";
    xcb_icccm_set_wm_class(dis, win, LEN(classInstance), classInstance);
    xcb_ewmh_client_source_type_t source = XCB_EWMH_CLIENT_SOURCE_TYPE_NORMAL;
    xcb_ewmh_set_wm_window_type(ewmh, win, 1, &ewmh->_NET_WM_WINDOW_TYPE_DOCK);
    xcb_ewmh_request_change_wm_state(ewmh, 0, win, XCB_EWMH_WM_STATE_ADD, ewmh->_NET_WM_STATE_STICKY,
        ewmh->_NET_WM_STATE_ABOVE, source);
}

void updateBackground(xcb_window_t win, int press, xcb_rectangle_t* rects) {
    xcb_poly_fill_rectangle(dis, win, gcPressed[press], 1, rects);
}
int getXFD() {
    return xcb_get_file_descriptor(dis);
}

int xFlush() {
    return xcb_flush(dis);
}



void processEvent(xcb_generic_event_t* event) {
    if(event->response_type < MAX_X_EVENTS && xEventHandlers[event->response_type])
        xEventHandlers[event->response_type](event);
}
void processAllQueuedEvents() {
    xcb_generic_event_t* event = xcb_poll_for_event(dis);
    do {
        processEvent(event);
        free(event);
    } while((event = xcb_poll_for_queued_event(dis)));
    xFlush();
}


#define _ADD_EVENT_TYPE_CASE(TYPE) case TYPE: return #TYPE
const char* opcodeToString(int opcode) {
    switch(opcode) {
            _ADD_EVENT_TYPE_CASE(XCB_CREATE_WINDOW);
            _ADD_EVENT_TYPE_CASE(XCB_CHANGE_WINDOW_ATTRIBUTES);
            _ADD_EVENT_TYPE_CASE(XCB_GET_WINDOW_ATTRIBUTES);
            _ADD_EVENT_TYPE_CASE(XCB_DESTROY_WINDOW);
            _ADD_EVENT_TYPE_CASE(XCB_MAP_WINDOW);
            _ADD_EVENT_TYPE_CASE(XCB_CONFIGURE_WINDOW);
            _ADD_EVENT_TYPE_CASE(XCB_CHANGE_PROPERTY);
        default:
            return "unknown code";
    }
}

void logError(xcb_generic_error_t* e) {
    printf("error occurred with seq %d resource %d. Error code: %d %s (%d %d)\n", e->sequence, e->resource_id, e->error_code,
        opcodeToString(e->major_code), e->major_code, e->minor_code);
}
