#include <X11/keysym.h>
#include <assert.h>
#include <stdio.h>
#include <string.h>
#include <unistd.h>
#include <xcb/xcb.h>
#include <xcb/xcb_ewmh.h>
#include <xcb/xcb_icccm.h>
#include <xcb/xtest.h>

#define DTEXT_XCB_IMPLEMENTATION
#include "dtext_xcb.h"
#include "xutil.h"

static xcb_connection_t* dis;
static xcb_window_t root;
static uint32_t depth;
static xcb_screen_t* screen;
static xcb_ewmh_connection_t _ewmh;
static xcb_ewmh_connection_t* ewmh = &_ewmh;
static xcb_gcontext_t  gc;
static const xcb_setup_t* xSetup;
static xcb_get_keyboard_mapping_reply_t* keyboard_mapping;
static uint32_t rootDims[2];
static dt_font *font;


static int avgNumberLengthWithCurrentFont;
static xcb_keycode_t keyCodeShift;


struct xdrawable {
    xcb_window_t win;
    xcb_window_t drawable;
    uint32_t width;
    uint32_t height;
    dt_context *ctx;
};

void initConnection() {
    dis = xcb_connect(NULL, NULL);
    if (!dis)
        exit(1);
    xcb_intern_atom_cookie_t* cookie = xcb_ewmh_init_atoms(dis, ewmh);
    xcb_ewmh_init_atoms_replies(ewmh, cookie, NULL);
    screen = ewmh->screens[0];
    root = screen->root;
    depth = screen->root_depth;
    setRootDims(screen->width_in_pixels, screen->height_in_pixels);

    gc = xcb_generate_id(dis);
    uint32_t value[]  = { 0xfefefe, 0x14313d};
    xcb_create_gc(dis, gc, root, XCB_GC_FOREGROUND | XCB_GC_BACKGROUND , value);
    xSetup      = xcb_get_setup(dis);

    keyboard_mapping = xcb_get_keyboard_mapping_reply(dis, xcb_get_keyboard_mapping(dis, xSetup->min_keycode,
                xSetup->max_keycode - xSetup->min_keycode + 1), NULL);

    int mask = XCB_EVENT_MASK_STRUCTURE_NOTIFY;
    xcb_change_window_attributes(dis, root, XCB_CW_EVENT_MASK, &mask);
    keyCodeShift = getKeyCode(XK_Shift_L, NULL, NULL);
}

void setRootDims(uint16_t width, uint16_t height) {
    rootDims[0] = width;
    rootDims[1] = height;
}

void closeConnection() {
    setFont(NULL, 0);
    free(keyboard_mapping);
    xcb_ewmh_connection_wipe(ewmh);
    xcb_disconnect(dis);
}

void setFont(const char* fontName, int size) {
    if (font) {
        dt_free_font(dis, font);
        font = NULL;
    }

    if (fontName) {
        font = dt_load_font(dis, fontName, size);
        if (!font) {
            printf("Could not load font name %s\n", fontName);
            exit(1);
        }
        avgNumberLengthWithCurrentFont = dt_get_text_width(dis, font, "0123456789", 10) / 10;
    }
}

void destroyWindow(XDrawable* drawable) {
    dt_free_context(drawable->ctx);
    xcb_destroy_window(dis, drawable->win);
    if (drawable->drawable != drawable->win) {
        xcb_free_pixmap(dis, drawable->drawable);
    }
    free(drawable);
}

XDrawable* createWindow(uint32_t windowMasks) {
    xcb_window_t win = xcb_generate_id(dis);
    xcb_create_window(dis, XCB_COPY_FROM_PARENT, win, root, 0, 0, 10, 10,
        0, XCB_WINDOW_CLASS_INPUT_OUTPUT,
        screen->root_visual,
        XCB_CW_EVENT_MASK, &windowMasks);

    XDrawable* drawable = malloc(sizeof(XDrawable));


    drawable->ctx = dt_create_context(dis, win);
    drawable->win = win;
    drawable->drawable = win;
    return drawable;
}

void mapWindow(XDrawable* drawable) {
    xcb_map_window(dis, drawable->win);
}

void onResize(XDrawable* drawable, uint32_t width, uint32_t height) {
        if (drawable->drawable == drawable->win || drawable->width != width || drawable->win != height) {
            xcb_window_t pixmapId = xcb_generate_id (dis);
            xcb_create_pixmap(dis, depth, pixmapId, root, width, height);
            xcb_change_window_attributes(dis, drawable->win, XCB_CW_BACK_PIXMAP, &pixmapId);
            if (drawable->drawable != drawable->win) {
                xcb_free_pixmap(dis, drawable->drawable);
            }
            drawable->drawable = pixmapId;
            drawable->width = width;
            drawable->height = height;
            dt_free_context(drawable->ctx);
            drawable->ctx = dt_create_context(dis, pixmapId);
        }
}

void clear_drawable(XDrawable* drawable) {
    xcb_rectangle_t rect = {0,0,drawable->width, drawable->height};
    xcb_poly_fill_rectangle(dis, drawable->drawable, gc, 1, &rect);
}

void clear_window(XDrawable* drawable) {
    xcb_clear_area(dis, 0, drawable->win, 0, 0, drawable->width, drawable->height);
}

void updateDockProperties(XDrawable* drawable, DockProperties dockProperties) {
    DockType dockType = dockProperties.type;
    int start = dockProperties.start;
    int end = dockProperties.end == 0 ? rootDims[dockType < TOP] : dockProperties.end;
    int wmStructFields[4] = {0};
    int x,y;
    short width, height;
    if (dockType < TOP) {
        width = wmStructFields[dockType] = rootDims[0] * dockProperties.thicknessPercent / 100;
        height = end - start;
    } else {
        height = wmStructFields[dockType] = rootDims[1] * dockProperties.thicknessPercent / 100;
        width = end - start;
    }

    switch(dockType) {
        default:
        case TOP:
        case LEFT:
            x = y = 0;
            break;
        case RIGHT:
            y = 0;
            x = rootDims[0] -  width;
            break;
        case BOTTOM:
            y = rootDims[1] - height;
            x = 0;
            break;
    }

    uint32_t mask = XCB_CONFIG_WINDOW_X | XCB_CONFIG_WINDOW_Y | XCB_CONFIG_WINDOW_WIDTH | XCB_CONFIG_WINDOW_HEIGHT;
    uint32_t values[4] = {x, y, width, height};

    xcb_ewmh_set_wm_strut(ewmh, drawable->win, wmStructFields[0], wmStructFields[1], wmStructFields[2], wmStructFields[3]);
    xcb_configure_window(dis, drawable->win, mask, values);
}

void sendKeyEvent(char press, xcb_keycode_t keyCode) {
    assert(keyCode);
    xcb_test_fake_input(dis, press ? XCB_KEY_PRESS : XCB_KEY_RELEASE, keyCode, XCB_CURRENT_TIME, root, 0, 0, 0);
}

void sendShiftKeyEvent(char press) {
    sendKeyEvent(press, keyCodeShift);
}

char getKeyChar(xcb_keysym_t sym) {
    if (XK_space <= sym && sym <= XK_asciitilde) {
        return sym  - XK_space + ' ';
    }
    return 0;
}

int dumpKeyCodes() {
    int          nkeycodes = keyboard_mapping->length / keyboard_mapping->keysyms_per_keycode;
    printf("NumKeyCodes %d %d\n", nkeycodes, keyboard_mapping->keysyms_per_keycode);
    xcb_keysym_t* keysyms  = (xcb_keysym_t*)(keyboard_mapping +
            1);  // `xcb_keycode_t` is just a `typedef u8`, and `xcb_keysym_t` is just a `typedef u32`
    for (int keycode_idx = 0; keycode_idx < nkeycodes; ++keycode_idx) {
        for (int keysym_idx = 0; keysym_idx < keyboard_mapping->keysyms_per_keycode; ++keysym_idx) {
            xcb_keysym_t  sym = keysyms[keysym_idx + keycode_idx * keyboard_mapping->keysyms_per_keycode];
            printf("KeyCode: %d Sym: %d %d\n", xSetup->min_keycode + keycode_idx, sym, sym == XK_Shift_R);
        }
    }
    return 0;
}

xcb_keycode_t getKeyCode(xcb_keysym_t targetSym, xcb_keysym_t** foundSym, char* symIndex) {
    int          nkeycodes = keyboard_mapping->length / keyboard_mapping->keysyms_per_keycode;
    // `xcb_keycode_t` is just a `typedef u8`, and `xcb_keysym_t` is just a `typedef u32`
    xcb_keysym_t* keysyms  = (xcb_keysym_t*)(keyboard_mapping + 1);
    for (int keycode_idx = 0; keycode_idx < nkeycodes; ++keycode_idx) {
        for (int keysym_idx = 0; keysym_idx < keyboard_mapping->keysyms_per_keycode; ++keysym_idx) {
            xcb_keysym_t*  sym = &keysyms[keysym_idx + keycode_idx * keyboard_mapping->keysyms_per_keycode];
            if (*sym == targetSym) {
                if (foundSym)
                    *foundSym = sym;
                if (symIndex)
                    *symIndex = keysym_idx;
                return xSetup->min_keycode + keycode_idx;
            }
        }
    }
    printf("Could not find %d\n", targetSym);
    assert(0);
    return 0;
}

int matchesWindow(XDrawable* drawable, xcb_window_t win) {
    return drawable->win == win;
}

void drawText(XDrawable* drawable, int numChars, const char*str, Color foreground,  int x, int y, int width, int height) {
    int textWidth = dt_get_text_width(dis, font, str, numChars);
    dt_draw(drawable->ctx, font, foreground, x + width/2 - textWidth/2, y + height/2, str, numChars);
}

void outlineRect(XDrawable* drawable, Color color, int numRects, const xcb_rectangle_t* rects) {
    xcb_change_gc(dis, gc, XCB_GC_FOREGROUND, &color);
    xcb_poly_rectangle(dis, drawable->drawable, gc, numRects, rects);
}

void drawSlider(XDrawable* drawable, Color color, float percent, xcb_rectangle_t* rect, xcb_rectangle_t* rectUsed) {
    int width = avgNumberLengthWithCurrentFont * 3;
    *rectUsed = (xcb_rectangle_t) {rect->x + percent * (rect->width - width) , rect->y, width, rect->height};
    xcb_change_gc(dis, gc, XCB_GC_FOREGROUND, &color);
    xcb_poly_fill_rectangle(dis, drawable->drawable, gc, 1, rectUsed);
}
void updateBackground(XDrawable* drawable, Color color, xcb_rectangle_t* rects) {
    xcb_change_gc(dis, gc, XCB_GC_FOREGROUND, &color);
    xcb_poly_fill_rectangle(dis, drawable->drawable, gc, 1, rects);
}

void setWindowProperties(XDrawable* drawable) {
    xcb_icccm_wm_hints_t hints = {};
    xcb_icccm_wm_hints_set_input(&hints, 0);
    xcb_icccm_set_wm_hints(dis, drawable->win, &hints);
    const char classInstance[] = "navboard\0navboard";
    xcb_icccm_set_wm_class(dis, drawable->win, LEN(classInstance), classInstance);
    xcb_ewmh_client_source_type_t source = XCB_EWMH_CLIENT_SOURCE_TYPE_NORMAL;
    xcb_ewmh_set_wm_window_type(ewmh, drawable->win, 1, &ewmh->_NET_WM_WINDOW_TYPE_DOCK);
    xcb_ewmh_request_change_wm_state(ewmh, 0, drawable->win, XCB_EWMH_WM_STATE_ADD, ewmh->_NET_WM_STATE_STICKY,
        ewmh->_NET_WM_STATE_ABOVE, source);
}

int getXFD() {
    return xcb_get_file_descriptor(dis);
}

int xFlush() {
    return xcb_flush(dis);
}

void processEvent(xcb_generic_event_t* event) {
    int type = event->response_type & 127;
    if (xEventHandlers[type])
        xEventHandlers[type](event);
}

void processAllQueuedEvents() {
    xcb_generic_event_t* event = xcb_poll_for_event(dis);
    while (event) {
        do {
            processEvent(event);
            free(event);
        } while ((event = xcb_poll_for_queued_event(dis)));
        xFlush();
        event = xcb_poll_for_event(dis);
    }
}

void grabSelection() {
    const char* name = "OSK";
    xcb_intern_atom_reply_t* reply;
    reply = xcb_intern_atom_reply(dis, xcb_intern_atom(dis, 0, strlen(name), name), NULL);
    xcb_atom_t atom = reply->atom;
    free(reply);
    xcb_set_selection_owner(dis, root, atom, XCB_CURRENT_TIME);
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
