/* See LICENSE file for copyright and license details. */
/**
 * To define the implementation, define  DTEXT_XCB_IMPLEMENTATION before including this file
 *
 * Also you may need to add
 * `-I /usr/include/freetype2` to CFLAGS And  `-lfreetype -lxcb -lxcb-render -lxcb-render-util` to LDFLAGS
 * to get this program to compile
 */
#ifndef DTEXT_H
#define DTEXT_H

#include <stdint.h>
#include <xcb/xcb.h>


typedef struct dt_context dt_context;
typedef struct dt_font dt_font;

/**
 * returns the height of the highest character of the font, relative to the baseline
 */
uint16_t dt_get_font_ascent(dt_font* font);
/**
 * returns the total height of the highest character in the font.
 */
uint16_t dt_get_font_height(dt_font* font);

/*
 * Allocates a new context and stores the result in ctx
 */
dt_context* dt_create_context(xcb_connection_t *dpy, xcb_window_t win);
/**
 * Frees an allocated context
 */
void dt_free_context(dt_context *ctx);

dt_font* dt_load_font(xcb_connection_t *dis, char const *name, int size);
void dt_free_font(xcb_connection_t *dis, dt_font *fnt);

/**
 * draws the string composed of the first `len` characters of `txt`,
 * drawn with font `fnt`, in color `color`, with the baseline starting at position
 * x, y.
 */
int dt_draw(dt_context *ctx, dt_font *fnt, uint32_t color,
                 uint32_t x, uint32_t y, char const *txt, size_t len);

/**
 * Returns the width in pixels that the specified text will take when drawn
 */
uint16_t dt_get_text_width(xcb_connection_t* dis, dt_font *fnt, char const *txt, size_t len) ;

/**
 * Splits txt into N lines by by total length
 *
 * Breaks up text into N lines such that strings are generally less than width pixels
 * @return N, the new number of strings
 */
int dt_word_wrap_n(xcb_connection_t* dis, dt_font *fnt, char *txt, int num_lines, uint32_t width);

/**
 * Replace all instances of '\n' with \0.
 *
 * Split txt into N seperate string by replacing the new line marker
 * with the null string.
 * @return N, the new number of strings
 */
int dt_split_lines(char *txt);

/**
 * Splits txt into N lines by '\n' and by total length
 *
 * Breaks up text into N lines such that each human line is its own string
 * and string are generally less than width pixels
 * @return N, the new number of strings
 */
int dt_word_wrap_line(xcb_connection_t*dis, dt_font *fnt, char *txt, uint32_t width);

/**
 * Calls dt_draw for each line such that each line is drawn right after each other
 */
int dt_draw_all_lines(dt_context *ctx, dt_font *fnt, uint32_t color,
        uint32_t x, uint32_t starting_y, uint32_t padding, char const *lines, int num_lines);

#ifdef DTEXT_XCB_IMPLEMENTATION

#ifndef DTEXT_DEFAULT_SYS_FONT_PATH
#define DTEXT_DEFAULT_SYS_FONT_PATH "/usr/share/fonts/"
#endif

#ifndef DTEXT_CHECK_SUBDIRS
#define DTEXT_CHECK_SUBDIRS 1
#endif
#if DTEXT_CHECK_SUBDIRS
#include <dirent.h>
#endif

#include <string.h>
#include <freetype2/ft2build.h>
#include <xcb/render.h>
#include <xcb/xcb.h>
#include <xcb/xcb_renderutil.h>
#include FT_ADVANCES_H
#include FT_FREETYPE_H

#define DTEXT_XCB_MAX(a, b) ((a) > (b) ? (a) : (b))
#define DTEXT_XCB_MIN(a, b) ((a) < (b) ? (a) : (b))

struct dt_context {
    xcb_connection_t* dis;
    xcb_render_pictformat_t win_format;
    xcb_render_picture_t pic;
    xcb_render_picture_t fill;
};

typedef struct {
    uint8_t c;  // char
    uint16_t adv; // advance
    int16_t asc; // ascender
    uint16_t h; // height
} dt_pair;

typedef struct {
    dt_pair *data;
    size_t len;
    size_t allocated;
} dt_row;


#define DT_HASH_SIZE 128
struct dt_font {
    uint16_t height;
    uint16_t ascent;

    FT_Library ft_lib;
    FT_Face *faces;
    size_t num_faces;

    xcb_render_glyphset_t gs;
    dt_row advance[DT_HASH_SIZE];
};

uint16_t dt_get_font_ascent(dt_font* font) {
    return font->ascent;
}

uint16_t dt_get_font_height(dt_font* font) {
    return font->height;
}

static xcb_render_pictformat_t get_argb32_format(xcb_connection_t* dis) {
    static xcb_render_pictformat_t argb32_format;
    if (!argb32_format) {
        xcb_render_query_pict_formats_reply_t* reply;
        reply = xcb_render_query_pict_formats_reply(dis, xcb_render_query_pict_formats(dis), NULL);
        xcb_render_pictforminfo_t* pictforminfo = xcb_render_util_find_standard_format(reply, XCB_PICT_STANDARD_ARGB_32);
        argb32_format = pictforminfo ? pictforminfo->id: 0 ;
        free(reply);
    }
    return argb32_format;
}

static dt_pair const * hash_get(dt_row map[DT_HASH_SIZE], uint8_t key) {
    dt_row row;
    size_t i;

    row = map[key % DT_HASH_SIZE];
    for (i = 0; i < row.len; ++i)
        if (row.data[i].c == key)
            return &row.data[i];

    return NULL;
}

static int dt_hash_set(dt_row map[DT_HASH_SIZE], dt_pair val) {
    dt_row row;
    dt_pair *d;
    size_t i;

    row = map[val.c % DT_HASH_SIZE];

    for (i = 0; i < row.len; ++i) {
        if (row.data[i].c == val.c) {
            row.data[i] = val;
            return 0;
        }
    }

    if (row.allocated == row.len) {
        d = row.data;
        if (!(d = realloc(d, (2 * row.len + 1) * sizeof(d[0]))))
            return -1;
        row.data = d;
        row.allocated = 2 * row.len + 1;
    }
    ++row.len;

    row.data[row.len - 1] = val;

    map[val.c % DT_HASH_SIZE] = row;
    return 0;
}

static int load_char(xcb_connection_t* dis, dt_font *fnt, char c) {
    int err;
    FT_UInt code;
    FT_GlyphSlot slot;
    xcb_render_glyph_t gid;
    xcb_render_glyphinfo_t g;
    char *img;
    size_t x, y, i;

    if (hash_get(fnt->advance, c))
        return 0;

    slot = 0;
    for (i = 0; i < fnt->num_faces; ++i) {
        code = FT_Get_Char_Index(fnt->faces[i], c);
        if (!code)
            continue;

        if ((err = FT_Load_Glyph(fnt->faces[i], code, FT_LOAD_RENDER)))
            continue;
        slot = fnt->faces[i]->glyph;
        break;
    }
    if (!slot) {
        if ((err = FT_Load_Char(fnt->faces[0], c, FT_LOAD_RENDER)))
            return err;
        slot = fnt->faces[0]->glyph;
    }

    gid = c;

    g.width  = slot->bitmap.width;
    g.height = slot->bitmap.rows;
    g.x = -slot->bitmap_left;
    g.y =   slot->bitmap_top;
    g.x_off = slot->advance.x >> 6;
    g.y_off = slot->advance.y >> 6;

    if (!(img = malloc(4 * g.width * g.height)))
        return -1;
    for (y = 0; y < g.height; ++y)
        for (x = 0; x < g.width; ++x)
            for (i = 0; i < 4; ++i)
                img[4 * (y * g.width + x) + i] =
                    slot->bitmap.buffer[y * g.width + x];

    xcb_render_add_glyphs(dis, fnt->gs, 1, &gid, &g,
                     4 * g.width * g.height, (uint8_t*) img);

    free(img);

    return dt_hash_set(fnt->advance, (dt_pair) {
        .c = c,
        .adv = slot->advance.x >> 6,
        .asc = - slot->metrics.horiBearingY >> 6,
        .h = slot->metrics.height >> 6
    });
}

static int dt_load_face(FT_Library* ft_lib, FT_Face *face, char const *file, int size) {
    int err = FT_New_Face(*ft_lib, file, 0, face);
    if (err)
        return err;

    if ((err = FT_Set_Char_Size(*face, size << 6, 0, 0, 0))) {
        FT_Done_Face(*face);
        return err;
    }

    return 0;
}

static int dt_find_and_load_dir(FT_Library* ft_lib, FT_Face *face, char const *name, int str_len, int size) {

    if (name[0] == '/') {
        return dt_load_face(ft_lib, face, name, size);
    }
    char buffer[255];
#if DTEXT_CHECK_SUBDIRS
    DIR* d = opendir(DTEXT_DEFAULT_SYS_FONT_PATH);
    if (!d)
        return -1;
    struct dirent * dir;
    while ((dir = readdir(d)) != NULL) {
        if (dir->d_name[0] == '.' && dir->d_name[1] == '.')
            continue;
        snprintf(buffer, sizeof(buffer), DTEXT_DEFAULT_SYS_FONT_PATH "/%s/%.*s", dir->d_name, str_len, name);
        if (dt_load_face(ft_lib, face, buffer, size) == 0) {
            return 0;
        }
    }
    closedir(d);
#endif
    snprintf(buffer, sizeof(buffer), DTEXT_DEFAULT_SYS_FONT_PATH "/%.*s", str_len, name);
    return -1;
}

dt_context* dt_create_context(xcb_connection_t *dis, xcb_window_t win) {
    dt_context *ctx;
    xcb_pixmap_t pix;

    if (!(ctx = malloc(sizeof(*ctx))))
        return NULL;

    xcb_screen_iterator_t iter = xcb_setup_roots_iterator (xcb_get_setup (dis));
    xcb_screen_t* screen = iter.data;

    ctx->dis = dis;
    ctx->pic = xcb_generate_id(dis);

    xcb_render_query_pict_formats_reply_t* reply;
    reply = xcb_render_query_pict_formats_reply(dis, xcb_render_query_pict_formats(dis), NULL);
    xcb_render_pictvisual_t*  pictvisual;
    pictvisual = xcb_render_util_find_visual_format(reply, screen->root_visual);
    ctx->win_format = pictvisual ? pictvisual->format: 0;
    free(reply);
    xcb_render_create_picture(dis, ctx->pic, win, ctx->win_format, 0, NULL);

    pix = xcb_generate_id(dis);
    xcb_create_pixmap(dis, 32, pix , screen->root, 1, 1);
    uint32_t values = XCB_RENDER_REPEAT_NORMAL;

    ctx->fill = xcb_generate_id(dis);
    xcb_render_create_picture(dis, ctx->fill, pix, get_argb32_format(dis), XCB_RENDER_CP_REPEAT, &values);
    xcb_free_pixmap(dis, pix);

    return ctx;
}

void dt_free_context(dt_context *ctx) {
    xcb_render_free_picture(ctx->dis, ctx->pic);
    xcb_render_free_picture(ctx->dis, ctx->fill);
    free(ctx);
}

static int dt_load_font_helper(dt_font *fnt, char const *name, int size) {
    while(1) {
        char* end = strchr(name, ';');
        int len = end ? end - name: strlen(name);
        if (dt_find_and_load_dir(&fnt->ft_lib, &fnt->faces[0], name, len, size) == 0) {
            return 0;
        }
        if(!end) {
            return -1;
        }
        name = end + 1;
    }
}
dt_font* dt_load_font(xcb_connection_t *dis, char const *name, int size) {
    dt_font *fnt;
    int16_t descent;

    if (!(fnt = calloc(1, sizeof(*fnt))))
        return NULL;
    fnt->num_faces = 1;

    if (FT_Init_FreeType(&fnt->ft_lib)) {
        free(fnt);
        return NULL;
    }

    if (!(fnt->faces = malloc(fnt->num_faces * sizeof(fnt->faces[0])))) {
        free(fnt);
        return NULL;
    }

    if(dt_load_font_helper(fnt, name, size)) {
        dt_free_font(dis, fnt);
        return NULL;
    }

    fnt->gs = xcb_generate_id(dis);
    xcb_render_create_glyph_set(dis, fnt->gs, get_argb32_format(dis));
    memset(fnt->advance, 0, sizeof(fnt->advance));

    fnt->ascent = descent = 0;
    for (size_t i = 0; i < fnt->num_faces; ++i) {
        fnt->ascent = DTEXT_XCB_MAX(fnt->ascent, fnt->faces[i]->ascender >> 6);
        descent = DTEXT_XCB_MIN(descent, fnt->faces[i]->descender >> 6);
    }
    fnt->height = fnt->ascent - descent;

    return fnt;
}

uint16_t dt_get_text_width(xcb_connection_t* dis, dt_font *fnt, char const *txt, size_t len) {
    uint32_t text_width = 0;
    for (int i = 0; i < len; ++i) {
        if ((load_char(dis, fnt, txt[i])))
            continue;
        text_width += hash_get(fnt->advance, txt[i])->adv;
    }
    return text_width;
}

void dt_free_font(xcb_connection_t *dis, dt_font *fnt) {
    if (fnt->gs) {
        xcb_render_free_glyph_set(dis, fnt->gs);
    }
    for (size_t i = 0; i < fnt->num_faces; ++i)
        if(fnt->faces[i]) {
            FT_Done_Face(fnt->faces[i]);
        }
    if(fnt->ft_lib) {
        FT_Done_FreeType(fnt->ft_lib);
    }
    free(fnt);
}

int dt_draw(dt_context *ctx, dt_font *fnt, uint32_t color,
        uint32_t x, uint32_t y, const char *txt, size_t len) {
    int err;
    xcb_render_color_t col;
    size_t i;
    uint8_t * c = (uint8_t*)&color;

    col.blue  = (c[0] << 8);
    col.green = (c[1] << 8);
    col.red   = (c[2] << 8);
    col.alpha = (c[3] << 8);
    xcb_rectangle_t rect ={0, 0, 1, 1};
    xcb_render_fill_rectangles(ctx->dis, XCB_RENDER_PICT_OP_SRC, ctx->fill, col, 1, &rect);

    for (i = 0; i < len; ++i)
        if ((err = load_char(ctx->dis, fnt, txt[i])))
            return err;

    xcb_render_util_composite_text_stream_t* stream = xcb_render_util_composite_text_stream(fnt->gs, len, 0);
    xcb_render_util_glyphs_8(stream, x, y, len, (uint8_t*) txt);
    xcb_render_util_composite_text(ctx->dis, XCB_RENDER_PICT_OP_OVER, ctx->fill, ctx->pic, get_argb32_format(ctx->dis), 0, 0, stream);
    xcb_render_util_composite_text_free(stream);

    return 0;
}


int split_lines(char *txt) {
    char* ptr = txt;
    int num_lines;
    for (num_lines = 0; ptr; num_lines++) {
        ptr = strstr(ptr, "\n");
        if (ptr) {
            *ptr = 0;
            ptr++;
        }
    }
    return num_lines;
}

int word_wrap_n(xcb_connection_t*dis , dt_font *fnt, char *txt, int num_lines, uint32_t width) {
    char*ptr = txt;
    for (int i = 0; i < num_lines; i++) {
        if (dt_get_text_width(dis, fnt, ptr, strlen(ptr)) > width) {
            char* whitespace = ptr;
            char* space = NULL;
            while (whitespace) {
                space = strstr(whitespace + 1, " ");
                if (!space)
                    break;
                int sublen = space - ptr;
                if (dt_get_text_width(dis, fnt, ptr, sublen) > width)
                    break;
                whitespace = space;
            }
            if (whitespace != ptr) {
                *whitespace = 0;
                num_lines++;
            }
            else if (space) {
                *space = 0;
                num_lines++;
            }
        }
        ptr = ptr + strlen(ptr) + 1;
    }
    return num_lines;
}

int dt_word_wrap_line(xcb_connection_t*dis, dt_font *fnt, char *txt, uint32_t width) {
    return word_wrap_n(dis, fnt, txt, split_lines(txt), width);
}

int dt_draw_all_lines(dt_context *ctx, dt_font *fnt, uint32_t color,
        uint32_t x, uint32_t starting_y, uint32_t padding, char const *lines, int num_lines) {

    int cell_height = padding + dt_get_font_ascent(fnt);
    const char*ptr = lines;
    for (int i = 0; i < num_lines; i++) {
        dt_draw(ctx, fnt, color, x, starting_y + cell_height * (i + 1), ptr, strlen(ptr));
        ptr = ptr + strlen(ptr) + 1;
    }
    return starting_y + cell_height * num_lines;
}

#endif
#endif
