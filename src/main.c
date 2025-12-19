#include <dlfcn.h>
#include <lauxlib.h>
#include <lua.h>
#include <lualib.h>
#include <math.h>
#include <raylib.h>
#include <raymath.h>
#include <stdio.h>
#include <stdlib.h>

#include "gram.h"
#include "loadfns.h"
#define PLAP_IMPLEMENTATION
#include "plap.h"

#define WIDHT 1000
#define HEIGHT 800
#define TIME 100
#define DIM 1
#define COL_MARGIN_PERCENT 0.20f
#define EXTERNAL_MARGIN_PERCENT 0.1f

const GramColor DEFAULT_COLORS[] = {
    GRAM_RED,
    GRAM_GREEN,
    GRAM_BLUE,
};

const GramColorScheme GRAM_DEFAULT_CSCHEME = {
    .colors_sz = sizeof DEFAULT_COLORS / sizeof(GramColor),
    .colors = (GramColor*)&DEFAULT_COLORS
};

const char* gram_fns_file_default = "./libgram_update.so";
const float PLOT_H = HEIGHT * (1 - EXTERNAL_MARGIN_PERCENT);
const float PLOT_W = WIDHT * (1 - EXTERNAL_MARGIN_PERCENT);
const float PLOT_EXTERNAL_MARGIN_W = WIDHT * EXTERNAL_MARGIN_PERCENT / 2.;
const float PLOT_EXTERNAL_MARGIN_H = HEIGHT * EXTERNAL_MARGIN_PERCENT / 2.;

static size_t s_time = TIME;
static size_t s_dim = DIM;
static char* gram_so_file = NULL;
static char* gram_lua_file = NULL;
static float** s_data = NULL;
static float s_min = 0;
static float s_max = 0;
static float s_min_v = 0;
static float s_max_v = 0;
static int s_draw_type = GRAM_DRAW_RECT;
static float s_full = 0;
static float s_colw = 0;
static float s_col_w_marg = 0;
static float s_plot_center_off = 0;
static const GramColorScheme* s_cscheme = &GRAM_DEFAULT_CSCHEME;

static GramExtFns gram_ext_fns = { 0 };
static lua_State* lua_state = { 0 };

float absf(float x)
{
    return x < 0 ? -x : x;
}

static void load()
{
    GramExtFns* ext = &gram_ext_fns;
    // if(ext->gram_fini) ext->gram_fini();
    if (s_data) {
        for (size_t i = 0; i < s_time; i++) {
            free(s_data[i]);
        }
        free(s_data);
        s_data = NULL;
    }
    if (gram_so_file) {
        load_from_so(gram_so_file, &gram_ext_fns);
    } else if (lua_state) {
        load_from_lua(gram_lua_file, lua_state, &gram_ext_fns);
    }

    if (ext->gram_init)
        ext->gram_init();

    if (ext->gram_get_draw_type)
        s_draw_type = ext->gram_get_draw_type();

    s_dim = ext->gram_get_dimensions ? ext->gram_get_dimensions() : DIM;

    s_time = ext->gram_get_time ? ext->gram_get_time() : TIME;

    s_data = calloc(s_time, sizeof(float*));
    for (size_t i = 0; i < s_time; i++) {
        s_data[i] = calloc(s_dim, sizeof(float));
    }

    if (ext->gram_get_color_scheme) {
        GramColorScheme* cs = ext->gram_get_color_scheme();
        if (cs)
            s_cscheme = cs;
    } else {
        s_cscheme = &GRAM_DEFAULT_CSCHEME;
    }
}

static void update_data()
{
    if (!gram_ext_fns.gram_update)
        return;
    s_min = 0;
    s_max = 0;

    for (size_t t = 0; t < s_time; t++) {
        gram_ext_fns.gram_update(t, s_data[t]);

        for (size_t d = 0; d < s_dim; d++) {
            s_min = fmin(s_data[t][d], s_min);
            s_max = fmax(s_data[t][d], s_max);
        }
    }
    s_max_v = s_max;
    s_min_v = s_min;
    s_min *= 1.05;
    s_max *= 1.05;
    s_full = s_max - s_min;
    s_colw = ((float)PLOT_W) / s_time;
    s_col_w_marg = (s_colw * COL_MARGIN_PERCENT) / 2.;
    s_plot_center_off = (absf(s_min) / s_full) * PLOT_H;
}

static void update()
{
    if (IsKeyReleased(KEY_R)) {
        TraceLog(LOG_INFO, "RELOADING");
        load();
        update_data();
    }
}

static void draw_data_point(Vector2 at, double val)
{
    static char buf[128] = { 0 };
    sprintf(buf, "%lf", val);
    Vector2 sz = MeasureTextEx(GetFontDefault(), buf, 10, 10);
    Vector2 pos = {
        .x = at.x,
        .y = at.y - sz.y
    };
    float w = sz.x * 1.1;
    float h = sz.y * 1.1;
    Rectangle box = {
        .x = pos.x - (w - sz.x) / 2.,
        .y = pos.y - (h - sz.y) / 2.,
        .width = w,
        .height = h,
    };

    DrawRectangleRounded(box, 5, 10, GRAY);
    DrawTextEx(GetFontDefault(), buf, pos, 10, 10, WHITE);
}

static void draw_data()
{
    Vector2 prev_c[s_dim];
    Vector2 mouse = GetMousePosition();

    // NAN if no data point to draw
    double data_point = NAN;

    for (size_t i = 0; i < s_time; i++) {
        for (size_t d = 0; d < s_dim; d++) {
            float v = s_data[i][d];
            float screen_h = (v / s_full) * PLOT_H;
            float adjust = v > 0 ? screen_h : 0;
            GramColor color = s_cscheme->colors[d % s_cscheme->colors_sz];

            switch (s_draw_type) {
            case GRAM_DRAW_RECT: {
                Rectangle r = {
                    .x = (0 + (i * s_colw) + s_col_w_marg) + PLOT_EXTERNAL_MARGIN_W,
                    .y = (PLOT_H - s_plot_center_off - adjust) + PLOT_EXTERNAL_MARGIN_H,
                    .width = s_colw - s_col_w_marg * 2,
                    .height = absf(screen_h),
                };
                DrawRectangleRec(r, (Color) { color.r, color.g, color.b, color.a });
                data_point = CheckCollisionPointRec(mouse, r) ? v : data_point;
            } break;
            case GRAM_DRAW_COL: {
                float w = (s_colw - s_col_w_marg * 2) / s_dim;
                Rectangle r = {
                    .x = (0 + (i * s_colw) + s_col_w_marg) + PLOT_EXTERNAL_MARGIN_W + (d * w),
                    .y = (PLOT_H - s_plot_center_off - adjust) + PLOT_EXTERNAL_MARGIN_H,
                    .width = w,
                    .height = absf(screen_h),
                };
                DrawRectangleRec(r, (Color) { color.r, color.g, color.b, color.a });
                data_point = CheckCollisionPointRec(mouse, r) ? v : data_point;
            } break;
            case GRAM_DRAW_LINE: {
                Vector2 center = {
                    .x = (0 + (i * s_colw) + (s_colw / 2.0)) + PLOT_EXTERNAL_MARGIN_W,
                    .y = PLOT_H - s_plot_center_off + PLOT_EXTERNAL_MARGIN_H - screen_h
                };
                DrawCircleV(center, 1.0f, (Color) { color.r, color.g, color.b, color.a });
                if (i > 0) {
                    DrawLineV(prev_c[d], center, (Color) { color.r, color.g, color.b, color.a });
                }
                prev_c[d] = center;
                data_point = Vector2Distance(mouse, center) ? v : data_point;
            } break;
            }
        }
    }
    // 0 line
    DrawLineV(
        (Vector2) { .x = PLOT_EXTERNAL_MARGIN_W, .y = HEIGHT - PLOT_EXTERNAL_MARGIN_H - s_plot_center_off },
        (Vector2) { .x = WIDHT - PLOT_EXTERNAL_MARGIN_W, .y = HEIGHT - PLOT_EXTERNAL_MARGIN_H - s_plot_center_off },
        (Color) { .r = 255, .b = 255, .g = 255, .a = 255 / 2 });
    if (!isnan(data_point))
        draw_data_point(mouse, data_point);
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
    if (gram_ext_fns.gram_update)
        draw_data();
}

static void draw_legend_region()
{
    ClearBackground(WHITE);

    static const char* zero = "0";
    static char buf[128 * 2] = { 0 };
    if (gram_ext_fns.gram_update) {
        Vector2 sz = MeasureTextEx(GetFontDefault(), zero, 24, 10);
        Vector2 pos = {
            .x = PLOT_EXTERNAL_MARGIN_W - sz.x * 1.5,
            .y = HEIGHT - PLOT_EXTERNAL_MARGIN_H - s_plot_center_off - (sz.y / 2.),
        };
        DrawTextEx(GetFontDefault(), zero, pos, 24, 10, RED);

        // sprintf(buf, "%lf", s_max_v);
        // sz = MeasureTextEx(GetFontDefault(), buf, 2, 10);
        // pos = (Vector2){
        //     .x = PLOT_EXTERNAL_MARGIN_W - sz.x * 1.5,
        //     .y = PLOT_EXTERNAL_MARGIN_H + ((s_max_v / s_full) * PLOT_H) - (sz.y / 2.),
        // };
        // DrawTextEx(GetFontDefault(), buf, pos, 24, 10, RED);
    }
}

static void draw()
{
    draw_legend_region();
    draw_plot_region();
}

int main(int argc, char** args)
{
    ArgsDef d = plap_args_def();
    plap_program_desc(&d, "gram", "simple graphing utility");
    plap_option_string(&d, "s", "so", "run the program with a shared object file", 1);
    plap_option_string(&d, "l", "lua", "run the program with a lua script", 1);
    plap_fail_on_no_args((&d));
    Args a = plap_parse_args(d, argc, args);

    Option* so = plap_get_option(&a, "s", "so");
    Option* lua = plap_get_option(&a, "l", "lua");
    if (so && lua) {
        fprintf(stderr, "Conflicting options `lua` and `so` (only one permitted)\n");
        exit(-1);
    } else if (so) {
        gram_so_file = so->str;
    } else if (lua) {
        gram_lua_file = lua->str;
        lua_state = luaL_newstate();
        luaL_openlibs(lua_state);
    }
    InitWindow(WIDHT, HEIGHT, "gram");
    SetTargetFPS(60);
    load();
    update_data();
    while (!WindowShouldClose()) {
        update();
        BeginDrawing();
        ClearBackground(BLACK);
        draw();
        EndDrawing();
    }
    if (gram_ext_fns.lib)
        dlclose(gram_ext_fns.lib);
    if (lua_state)
        lua_close(lua_state);
    CloseWindow();

    plap_free_args(a);
    return 0;
}
