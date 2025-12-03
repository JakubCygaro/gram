#include "gram.h"
#include <dlfcn.h>
#include <math.h>
#include <raylib.h>
#include <raymath.h>
#include <stdio.h>
#include <stdlib.h>
#define WIDHT 1000
#define HEIGHT 800
#define TIME 100
#define DIM 1
#define COL_MARGIN_PERCENT 0.20f
#define EXTERNAL_MARGIN_PERCENT 0.1f

#define GRAM_LOAD_FN(FN)                                                                                     \
    FN = dlsym(gram_update_lib, FN##_fn_name);                                                               \
    if (!FN) {                                                                                               \
        fprintf(stderr, "Could not find function `%s` in `%s`\nError: %s\n", #FN, gram_fns_file, dlerror()); \
    }
#define GRAM_DEFINE_FN(RET, NAME, ...)     \
    typedef RET (*NAME##_fn)(__VA_ARGS__); \
    static NAME##_fn NAME = NULL;          \
    static const char* NAME##_fn_name = #NAME

GRAM_DEFINE_FN(void, gram_update, size_t, float*);
GRAM_DEFINE_FN(int, gram_get_draw_type);
GRAM_DEFINE_FN(size_t, gram_get_time);
GRAM_DEFINE_FN(size_t, gram_get_dimensions);

const char* gram_fns_file_default = "./libgram_update.so";
const float PLOT_H = HEIGHT * (1 - EXTERNAL_MARGIN_PERCENT);
const float PLOT_W = WIDHT * (1 - EXTERNAL_MARGIN_PERCENT);
const float PLOT_EXTERNAL_MARGIN_W = WIDHT * EXTERNAL_MARGIN_PERCENT / 2.;
const float PLOT_EXTERNAL_MARGIN_H = HEIGHT * EXTERNAL_MARGIN_PERCENT / 2.;

static size_t s_time = TIME;
static size_t s_dim = DIM;
static void* gram_update_lib = NULL;
static char* gram_fns_file = "";
static float** s_data = NULL;
static float s_min = 0;
static float s_max = 0;
static int s_draw_type = GRAM_DRAW_RECT;
static float s_full = 0;
static float s_colw = 0;
static float s_col_w_marg = 0;
static float s_plot_center_off = 0;

float absf(float x)
{
    return x < 0 ? -x : x;
}

static void load_fns()
{
    if(s_data){
        for(size_t i = 0; i < s_time; i++){
            free(s_data[i]);
        }
        free(s_data);
        s_data = NULL;
    }

    if (gram_update_lib)
        dlclose(gram_update_lib);
    gram_update_lib = dlopen(gram_fns_file, RTLD_NOW);
    if (!gram_update_lib) {
        fprintf(stderr, "Could not open `%s`\nError: %s\n", gram_fns_file, dlerror());
    }
    GRAM_LOAD_FN(gram_update);
    GRAM_LOAD_FN(gram_get_draw_type);
    if (gram_get_draw_type)
        s_draw_type = gram_get_draw_type();
    GRAM_LOAD_FN(gram_get_dimensions);
    s_dim = gram_get_dimensions ? gram_get_dimensions() : DIM;
    GRAM_LOAD_FN(gram_get_time);
    s_time = gram_get_time ? gram_get_time() : TIME;

    s_data = calloc(s_time, sizeof(float*));
    for(size_t i = 0; i < s_time; i++){
        s_data[i] = calloc(s_dim, sizeof(float));
    }
}

static void update_data()
{
    if (!gram_update)
        return;
    s_min = 0;
    s_max = 0;

    for (size_t t = 0; t < s_time; t++) {
        gram_update(t, s_data[t]);

        for (size_t d = 0; d < s_dim; d++) {
            s_min = fmin(s_data[t][d], s_min);
            s_max = fmax(s_data[t][d], s_max);
        }
    }
    s_full = s_max - s_min;
    s_colw = ((float)PLOT_W ) / s_time;
    s_col_w_marg = (s_colw * COL_MARGIN_PERCENT) / 2.;
    s_plot_center_off = (absf(s_min) / s_full) * PLOT_H;
}

static void update()
{
    if (IsKeyReleased(KEY_R)) {
        TraceLog(LOG_INFO, "RELOADING");
        load_fns();
        update_data();
    }
}

static void draw_data()
{
    Vector2 prev_c[s_dim];

    for (size_t i = 0; i < s_time; i++) {
        for (size_t d = 0; d < s_dim; d++) {
            float v = s_data[i][d];
            float screen_h = (v / s_full) * PLOT_H;
            float adjust = v > 0 ? screen_h : 0;

            switch (s_draw_type) {
            case GRAM_DRAW_RECT: {
                Rectangle r = {
                    .x = (0 + (i * s_colw) + s_col_w_marg) + PLOT_EXTERNAL_MARGIN_W,
                    .y = (PLOT_H - s_plot_center_off - adjust) + PLOT_EXTERNAL_MARGIN_H,
                    .width = s_colw - s_col_w_marg * 2,
                    .height = absf(screen_h),
                };
                DrawRectangleRec(r, RED);
            } break;
            case GRAM_DRAW_LINE: {
                Vector2 center = {
                    .x = (0 + (i * s_colw) + (s_colw / 2.0)) + PLOT_EXTERNAL_MARGIN_W,
                    .y = PLOT_H - s_plot_center_off + PLOT_EXTERNAL_MARGIN_H - screen_h
                };
                DrawCircleV(center, 1.0f, RED);
                if (i > 0) {
                    DrawLineV(prev_c[d], center, RED);
                }
                prev_c[d] = center;
            } break;
            }
        }
    }
    // 0 line
    DrawLineV(
        (Vector2) { .x = PLOT_EXTERNAL_MARGIN_W, .y = HEIGHT - PLOT_EXTERNAL_MARGIN_H - s_plot_center_off },
        (Vector2) { .x = WIDHT - PLOT_EXTERNAL_MARGIN_W, .y = HEIGHT - PLOT_EXTERNAL_MARGIN_H - s_plot_center_off },
        (Color) { .r = 255, .b = 255, .g = 255, .a = 255 / 2 });
}

static void draw_plot_region()
{
    Rectangle plot_area = {
        .x = PLOT_EXTERNAL_MARGIN_W,
        .y = PLOT_EXTERNAL_MARGIN_H,
        .width = WIDHT - PLOT_EXTERNAL_MARGIN_W * 2,
        .height = HEIGHT - PLOT_EXTERNAL_MARGIN_H * 2
    };
    DrawRectangleRec(plot_area, BLACK);
    if (gram_update)
        draw_data();
}

static void draw_legend_region()
{
    ClearBackground(WHITE);

    static const char* zero = "0";
    if (gram_update) {
        Vector2 sz = MeasureTextEx(GetFontDefault(), zero, 24, 10);
        Vector2 pos = {
            .x = PLOT_EXTERNAL_MARGIN_W - sz.x * 1.5,
            .y = HEIGHT - PLOT_EXTERNAL_MARGIN_H - s_plot_center_off - (sz.y / 2.),
        };
        DrawTextEx(GetFontDefault(), zero, pos, 24, 10, RED);
    }
}

static void draw()
{
    draw_legend_region();
    draw_plot_region();
}

int main(int argc, char** args)
{
    if (argc == 2) {
        gram_fns_file = args[1];
    } else if (argc == 1) {
        gram_fns_file = (char*)gram_fns_file_default;
    } else {
        fprintf(stderr, "Error: too many arguments\n");
        return -1;
    }
    InitWindow(WIDHT, HEIGHT, "gram");
    SetTargetFPS(60);
    load_fns();
    update_data();
    while (!WindowShouldClose()) {
        update();
        BeginDrawing();
        ClearBackground(BLACK);
        draw();
        EndDrawing();
    }
    CloseWindow();
    if (gram_update_lib)
        dlclose(gram_update_lib);
    return 0;
}
