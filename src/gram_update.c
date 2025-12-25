#include "gram.h"
#include <stdlib.h>

#define DIM 1
#define TIME 100
#define STEP 1
#define START_AT 0

/// this is the update function called TIME times with:
/// `t = ([0..TIME] * STEP) + START_AT`
/// `ret` is a pointer to an array of length equal to DIM
void gram_update(float t, float* ret){
    *ret = t;
}
/// this function returns the value indicating what style the graph should be drawn in
int gram_get_draw_type(){
    return GRAM_DRAW_RECT;
}
size_t gram_get_time(void){
    return TIME;
}
int gram_get_start_at(void){
    return START_AT;
}
size_t gram_get_dimensions(void){
    return DIM;
}
float gram_get_step(void){
    return STEP;
}
GramColorScheme* gram_get_color_scheme(void){
    return NULL; // default scheme
}
/// called when the script is loaded
void gram_init(void){}
/// called when the script is done (or before being reloaded)
void gram_fini(void){}
