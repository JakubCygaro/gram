#ifndef LOADFNS_H
#define LOADFNS_H
#include "gram.h"
#include "stdlib.h"
#include <lua.h>

#define _DEFINE_FN(RET, NAME, ...) \
    RET (*NAME)(__VA_ARGS__)

#define _LOAD_FN(PTR, LIB, FN) \
    PTR = dlsym(LIB, #FN);

#define _LOAD_ERR(PTR, FN, FILE)                                                                                 \
    if (!PTR) {                                                                                                  \
        TraceLog(LOG_ERROR, "Could not find function `%s` in `%s`\n\t %s", #FN, FILE, dlerror()); \
    }

typedef struct {
    void* lib;
    _DEFINE_FN(void, gram_update, size_t, float*);
    _DEFINE_FN(int, gram_get_draw_type);
    _DEFINE_FN(size_t, gram_get_time, void);
    _DEFINE_FN(size_t, gram_get_dimensions, void);
    _DEFINE_FN(GramColorScheme*, gram_get_color_scheme, void);
    _DEFINE_FN(void, gram_init, void);
    _DEFINE_FN(void, gram_fini, void);
} GramExtFns;

void load_from_so(const char*, GramExtFns*);
void load_from_lua(const char* src, lua_State* l, GramExtFns* fns);

#endif
