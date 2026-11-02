/* SPDX-License-Identifier: GPL-2.0-only */
#ifndef DISPLAY_SETTINGS_H
#define DISPLAY_SETTINGS_H
#include <stdbool.h>
typedef struct {
    bool fullscreen,integer,pending;
    bool high_resolution;
    unsigned render_width,render_height;
    unsigned scale; /* 1..5 window-point presets; 0 means manually resized. */
    int width,height,pixel_width,pixel_height;
    char error[160];
} display_settings;
#endif
