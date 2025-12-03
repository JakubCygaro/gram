#include <stdlib.h>
#include "gram.h"

#define DIM 3
#define TIME 100

void gram_update(size_t t, float* ys) {
    ys[0] = 10 +(float)((int)t * (int)t) * 2;
    ys[1] = 10 +(float)((int)t * (int)t) * 3;
    ys[2] = 10 +(float)((int)t * (int)t) * 4;
}

int gram_get_draw_type() {
    return GRAM_DRAW_LINE;
}

size_t gram_get_time(){
    return TIME;
}
size_t gram_get_dimensions(){
    return DIM;
}

// GramColorScheme gram_get_color_scheme_fn(){
//     return (GramColorScheme) {
//         .colors_sz =
//     };
// }
