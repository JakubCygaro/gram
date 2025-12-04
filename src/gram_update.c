#include <stdlib.h>
#include "ceny.h"
#include "gram.h"

#define DIM 1
#define TIME 100

void gram_update(size_t t, float* ys) {
    ys[0] = DCENY.hrok[t] - 100.0;
}

int gram_get_draw_type() {
    return GRAM_DRAW_LINE;
}

size_t gram_get_time(){
    return DCENY.data_count;
}
size_t gram_get_dimensions(){
    return DCENY.column_count;
}

// GramColorScheme gram_get_color_scheme_fn(){
//     return (GramColorScheme) {
//         .colors_sz =
//     };
// }
