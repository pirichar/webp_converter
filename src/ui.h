/*
 * WebP Converter - UI module
 * Handles the graphical interface using raylib and raygui
 */

#ifndef UI_H
#define UI_H

#include "converter.h"
#include "presets.h"
#include <raylib.h>

/* Application state */
typedef enum {
    STATE_IDLE,         /* No image loaded */
    STATE_LOADED,       /* Image loaded, ready to convert */
    STATE_CONVERTING,   /* Conversion in progress */
    STATE_SUCCESS,      /* Conversion completed */
    STATE_ERROR         /* Error occurred */
} AppState;

/* UI context */
typedef struct {
    /* Window */
    int window_width;
    int window_height;

    /* State */
    AppState state;
    char status_message[256];

    /* Image */
    ImageData image;
    Texture2D preview_texture;
    bool has_preview;

    /* Conversion settings */
    ConversionParams params;
    int selected_preset;

    /* Output */
    char output_path[512];
    ConversionResult last_result;

    /* UI state */
    float preview_scale;
    Vector2 preview_offset;
    bool show_advanced;
    bool dragging_preview;

    /* Drag and drop */
    bool waiting_for_drop;
} UIContext;

/* Initialize UI */
void ui_init(UIContext *ctx);

/* Cleanup UI */
void ui_cleanup(UIContext *ctx);

/* Main UI update and render */
void ui_update(UIContext *ctx);

/* Handle file drop */
void ui_handle_drop(UIContext *ctx, const char *filepath);

/* Load image for preview */
bool ui_load_image(UIContext *ctx, const char *filepath);

/* Start conversion */
void ui_start_conversion(UIContext *ctx);

#endif /* UI_H */
