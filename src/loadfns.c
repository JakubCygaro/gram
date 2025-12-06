#include "loadfns.h"
#include "gram.h"
#include <ctype.h>
#include <dlfcn.h>
#include <lauxlib.h>
#include <lua.h>
#include <lualib.h>
#include <raylib.h>
#include <raymath.h>
#include <stdlib.h>
#include <string.h>

#define STRINGIFY(T) #T
#define streq(A, B) strcmp(A, B) == 0

typedef struct {
    const char* name;
    const GramColor color;
} string_color_pair_t;

static const string_color_pair_t PredefinedColors[] = {
    (string_color_pair_t) { .name = "red", .color = GRAM_RED },
    (string_color_pair_t) { .name = "blue", .color = GRAM_BLUE },
    (string_color_pair_t) { .name = "green", .color = GRAM_GREEN },
    (string_color_pair_t) { .name = "pink", .color = GRAM_PINK },
    (string_color_pair_t) { .name = "yellow", .color = GRAM_YELLOW },
    (string_color_pair_t) { .name = "white", .color = GRAM_WHITE },
    (string_color_pair_t) { .name = "orange", .color = GRAM_ORANGE },
};

static lua_State* L = NULL;
static size_t Dim = 0;

char* stolower(const char* str)
{
    size_t str_len = strlen(str);
    char* lstr = calloc(str_len + 1, sizeof(char));
    for (size_t i = 0; i < str_len; i++) {
        lstr[i] = tolower(str[i]);
    }
    return lstr;
}

static int find_predefined_color(const char* str, GramColor* c)
{
    for (size_t i = 0; i < sizeof(PredefinedColors) / sizeof(string_color_pair_t); i++) {
        if (streq(str, PredefinedColors[i].name)) {
            *c = PredefinedColors[i].color;
            return 1;
        }
    }
    return 0;
}

void load_from_so(const char* p, GramExtFns* fns)
{
    if (fns->lib)
        dlclose(fns->lib);
    fns->lib = dlopen(p, RTLD_NOW);
    if (!fns->lib) {
        TraceLog(LOG_ERROR, "Could not open `%s`\nError: %s\n", p, dlerror());
        fns->lib = NULL;
        return;
    }
    _LOAD_FN(fns->gram_update, fns->lib, gram_update);
    _LOAD_ERR(fns->gram_update, gram_update, p);

    _LOAD_FN(fns->gram_get_draw_type, fns->lib, gram_get_draw_type);
    _LOAD_ERR(fns->gram_get_draw_type, gram_get_draw_type, p);

    _LOAD_FN(fns->gram_get_dimensions, fns->lib, gram_get_dimensions);
    _LOAD_ERR(fns->gram_get_dimensions, gram_get_dimensions, p);

    _LOAD_FN(fns->gram_get_time, fns->lib, gram_get_time);
    _LOAD_ERR(fns->gram_get_time, gram_get_time, p);

    _LOAD_FN(fns->gram_get_color_scheme, fns->lib, gram_get_color_scheme);
    _LOAD_ERR(fns->gram_get_color_scheme, gram_get_color_scheme, p);

    _LOAD_FN(fns->gram_init, fns->lib, gram_init);
    _LOAD_ERR(fns->gram_init, gram_init, p);

    _LOAD_FN(fns->gram_fini, fns->lib, gram_fini);
    _LOAD_ERR(fns->gram_fini, gram_fini, p);
}
static size_t l_gram_get_time()
{
    lua_getglobal(L, STRINGIFY(time));
    if (!lua_isinteger(L, -1)) {
        TraceLog(LOG_ERROR, STRINGIFY(time) " has to be a positive non-zero integer");
        return 1;
    }
    size_t t = lua_tointeger(L, -1);
    lua_settop(L, 0);
    if (t <= 0) {
        TraceLog(LOG_ERROR, STRINGIFY(time) " has to be a positive non-zero integer");
        return 1;
    }
    return t;
}

static void l_gram_update(size_t t, float* row)
{
    lua_getglobal(L, STRINGIFY(gram_update));
    if (!lua_isfunction(L, -1)) {
        TraceLog(LOG_ERROR, "Could not find" STRINGIFY(gram_update) " function in lua script");
        lua_settop(L, 0);
        return;
    }
    lua_pushnumber(L, t);
    if (Dim <= 0) {
        lua_settop(L, 0);
        return;
    }
    if (lua_pcall(L, 1, 1, 0) != LUA_OK) {
        TraceLog(LOG_ERROR, "Error while calling" STRINGIFY(gram_update) " function in lua script",
            lua_tostring(L, -1));
        lua_settop(L, 0);
        return;
    }
    if (lua_isnumber(L, -1) && Dim == 1) {
        row[0] = lua_tonumber(L, -1);
    } else if (lua_istable(L, -1)) {
        size_t ret_len = lua_rawlen(L, -1);
        for (size_t i = 0; (i < Dim) && (i < ret_len); i++) {
            int topty = lua_type(L, -1);
            int ty = lua_rawgeti(L, -1, i + 1);
            if (ty == LUA_TNUMBER) {
                row[i] = lua_tonumber(L, -1);
            }
            lua_pop(L, 1);
        }
    } else {
        TraceLog(LOG_ERROR, STRINGIFY(gram_update) " function returned a disallowed value");
    }
    lua_settop(L, 0);
}
static int l_gram_get_draw_type()
{
    lua_getglobal(L, STRINGIFY(draw));
    if (!lua_isstring(L, -1)) {
        TraceLog(LOG_ERROR, STRINGIFY(draw) " has to be a string");
        lua_settop(L, 0);
        return 0;
    }
    const char* str = lua_tostring(L, -1);
    lua_pop(L, 1);

    char* strl = stolower(str);

    int ret = GRAM_DRAW_LINE;
    if (streq(strl, "line")) {
        ret = GRAM_DRAW_LINE;
    } else if (streq(strl, "rect")) {
        ret = GRAM_DRAW_RECT;
    } else {
        TraceLog(LOG_ERROR, STRINGIFY(draw) " is of invalid value `%s`", str);
    }
    free(strl);
    lua_settop(L, 0);
    return ret;
}
static size_t l_gram_get_dimensions()
{
    lua_getglobal(L, STRINGIFY(dimensions));
    if (!lua_isinteger(L, -1)) {
        TraceLog(LOG_WARNING, STRINGIFY(dimensions) " not set, assuming default of 1");
        return 1;
    }
    Dim = lua_tointeger(L, -1);
    lua_settop(L, 0);
    if (Dim <= 0) {
        TraceLog(LOG_ERROR, STRINGIFY(dimensions) " has to be a positive non-zero integer");
        return 1;
    }
    return Dim;
}

static GramColorScheme* ColorScheme = NULL;

int is_color(const char* str, GramColor* c)
{
    char* lstr = stolower(str);
    if (find_predefined_color(lstr, c)) {
        free(lstr);
        return 1;
    }
    static const char pat[] = "#aabbcc";
    if (strlen(lstr) != (sizeof pat / sizeof(char)) - 1) {
        free(lstr);
        return 0;
    }
    long hex = strtol(lstr + 1, NULL, 16);
    c->b = (c->b | hex);
    hex = hex >> 8;
    c->g = (c->g | hex);
    hex = hex >> 8;
    c->r = (c->r | hex);
    c->a = 255;

    free(lstr);
    return 1;
}

static GramColorScheme* l_gram_get_color_scheme()
{
    if (ColorScheme) {
        free(ColorScheme);
        ColorScheme = NULL;
    }
    lua_getglobal(L, STRINGIFY(colors));
    if (!lua_istable(L, -1)) {
        TraceLog(LOG_WARNING, STRINGIFY(colors) " not set, assuming default color scheme");
        lua_settop(L, 0);
        return NULL;
    }
    ColorScheme = calloc(1, sizeof *ColorScheme);
    ColorScheme->colors = calloc(2, sizeof(GramColor));
    size_t len = lua_rawlen(L, -1);
    size_t i = 0;
    for (; i < len; i++) {
        int ty = lua_rawgeti(L, -1, i + 1);
        if (ty != LUA_TSTRING) {
            TraceLog(LOG_ERROR, STRINGIFY(colors) "[%ld] is not a string, will be ignored", i);
            continue;
        }
        const char* cstring = lua_tostring(L, -1);
        GramColor c = { 0 };
        if (is_color(cstring, &c)) {
            ColorScheme->colors = realloc(ColorScheme->colors, i + 1 * sizeof c);
            ColorScheme->colors_sz = i + 1;
            ColorScheme->colors[i] = c;
        } else {
            TraceLog(LOG_ERROR, STRINGIFY(colors) "[%ld] is not a valid color, will be ignored", i);
        }

        lua_pop(L, 1);
    }
    lua_settop(L, 0);
    return ColorScheme;
}
static void l_gram_init()
{
    if (!lua_getglobal(L, STRINGIFY(init))) {
        lua_settop(L, 0);
        return;
    }
    if (!lua_isfunction(L, -1)) {
        lua_settop(L, 0);
        return;
    }
    if (lua_pcall(L, 0, 0, 0) != LUA_OK) {
        TraceLog(LOG_ERROR, "Error while calling" STRINGIFY(init) " function in lua script",
            lua_tostring(L, -1));
        lua_settop(L, 0);
        return;
    }
}
static void l_gram_fini(){
    if (!lua_getglobal(L, STRINGIFY(fini))) {
        lua_settop(L, 0);
        return;
    }
    if (!lua_isfunction(L, -1)) {
        lua_settop(L, 0);
        return;
    }
    if (lua_pcall(L, 0, 0, 0) != LUA_OK) {
        TraceLog(LOG_ERROR, "Error while calling" STRINGIFY(fini) " function in lua script",
            lua_tostring(L, -1));
        lua_settop(L, 0);
        return;
    }
}
void load_from_lua(const char* src, lua_State* l, GramExtFns* fns)
{
    L = NULL;
    if (luaL_loadfile(l, src) || lua_pcall(l, 0, 0, 0)) {
        TraceLog(LOG_ERROR, "Cannot run configuration file: %s",
            lua_tostring(l, -1));
        return;
    }
    fns->gram_fini = &l_gram_fini;
    fns->gram_init = &l_gram_init;
    fns->gram_get_color_scheme = &l_gram_get_color_scheme;
    fns->gram_get_draw_type = &l_gram_get_draw_type;
    fns->gram_get_time = &l_gram_get_time;
    fns->gram_get_dimensions = &l_gram_get_dimensions;
    fns->gram_update = &l_gram_update;
    L = l;
}
