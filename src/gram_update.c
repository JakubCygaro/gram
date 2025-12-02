#include <stdlib.h>
#include "gram.h"
float gram_update(size_t t) {
    return 10 + (float)(((int)t - 50) * ((int)t - 50) * ((int)t - 20));
}

int gram_get_draw_type() {
    return GRAM_DRAW_LINE;
}

size_t gram_get_time(){
    return 100;
}
size_t gram_get_dimensions(){
    return 1;
}
