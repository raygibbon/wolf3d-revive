/* SPDX-License-Identifier: GPL-2.0-only */
#ifndef DISPLAY_SETTINGS_H
#define DISPLAY_SETTINGS_H
#include <stdbool.h>
typedef struct {
    bool fullscreen,integer,pending;
    bool high_resolution;
    unsigned render_width,render_height;
    bool render_auto,vsync,linear;
    unsigned aspect; /* 0 Auto, 1 4:3, 2 16:9, 3 16:10, 4 21:9, 5 32:9. */
    unsigned scale; /* 1..5 window-point presets; 0 means manually resized. */
    int width,height,pixel_width,pixel_height;
    char error[160];
} display_settings;
#endif
