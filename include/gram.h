#ifndef GRAM_H
#define GRAM_H

#define GRAM_DRAW_RECT (1 << 0)
#define GRAM_DRAW_LINE (1 << 1)
#define GRAM_DRAW_COL (1 << 2)

typedef struct gram_color {
    int r, g, b, a;
} GramColor;

typedef struct gram_color_scheme {
    unsigned long colors_sz;
    GramColor* colors;
} GramColorScheme;

#define GRAM_RED (GramColor) { .r = 255, .g = 0, .b = 0, .a = 255 }
#define GRAM_GREEN (GramColor) { .r = 0, .g = 255, .b = 0, .a = 255 }
#define GRAM_BLUE (GramColor) { .r = 0, .g = 0, .b = 255, .a = 255 }
#define GRAM_YELLOW (GramColor) { .r = 255, .g = 255, .b = 0, .a = 255 }
#define GRAM_WHITE (GramColor) { .r = 255, .g = 255, .b = 255, .a = 255 }
#define GRAM_PINK (GramColor) { .r = 255, .g = 20, .b = 147, .a = 255 }
#define GRAM_ORANGE (GramColor) { .r = 255, .g = 165, .b = 0, .a = 255 }

#endif
