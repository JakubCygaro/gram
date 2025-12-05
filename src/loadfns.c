#include "loadfns.h"
#include <ctype.h>
#include <dlfcn.h>
#include <lauxlib.h>
#include <lua.h>
#include <lualib.h>
#include <raylib.h>
#include <raymath.h>
#include <string.h>

static lua_State* L = NULL;
static size_t Dim = 0;

#define STRINGIFY(T) #T
#define streq(A, B) strcmp(A, B) == 0

char* stolower(const char* str)
{
    size_t str_len = strlen(str);
    char* lstr = calloc(str_len + 1, sizeof(char));
    for (size_t i = 0; i < str_len; i++) {
        lstr[i] = tolower(str[i]);
    }
    return lstr;
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
}
static size_t l_gram_get_time()
{
    lua_getglobal(L, STRINGIFY(time));
    if (!lua_isinteger(L, -1)) {
        TraceLog(LOG_ERROR, STRINGIFY(time) " has to be a positive non-zero integer");
        return 1;
    }
    size_t t = lua_tointeger(L, -1);
    lua_pop(L, 1);
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
        return;
    }
    lua_pushnumber(L, t);
    if (Dim <= 0)
        return;
    if (lua_pcall(L, 1, 1, 0) != LUA_OK) {
        TraceLog(LOG_ERROR, "Error while calling" STRINGIFY(gram_update) " function in lua script");
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
    lua_pop(L, 1);
    if (Dim <= 0) {
        TraceLog(LOG_ERROR, STRINGIFY(dimensions) " has to be a positive non-zero integer");
        return 1;
    }
    return Dim;
}
static GramColorScheme* l_gram_get_color_scheme() {

}

void load_from_lua(const char* src, lua_State* l, GramExtFns* fns)
{
    L = NULL;
    if (luaL_loadfile(l, src) || lua_pcall(l, 0, 0, 0)) {
        TraceLog(LOG_ERROR, "Cannot run configuration file: %s",
            lua_tostring(l, -1));
        return;
    }
    fns->gram_get_color_scheme = &l_gram_get_color_scheme;
    fns->gram_get_draw_type = &l_gram_get_draw_type;
    fns->gram_get_time = &l_gram_get_time;
    fns->gram_get_dimensions = &l_gram_get_dimensions;
    fns->gram_update = &l_gram_update;
    L = l;
}
