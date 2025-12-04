#include "loadfns.h"
#include <dlfcn.h>
#include <raylib.h>
#include <raymath.h>

void load_from_so(const char* p, GramExtFns* fns){
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

void load_from_lua(const char* src, lua_State* l, GramExtFns* fns){

}
