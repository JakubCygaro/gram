#include "gram.h"
#include <stdlib.h>

#define DIM 1
#define TIME 100

void gram_update(size_t t, float* ys)
{
    ys[0] = t;
}

int gram_get_draw_type()
{
    return GRAM_DRAW_RECT;
}

size_t gram_get_time()
{
    return TIME;
}
size_t gram_get_dimensions()
{
    return DIM;
}
