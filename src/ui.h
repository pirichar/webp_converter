/*
 * WebP Converter - UI module
 * Handles the graphical interface using raylib and raygui
 */

#ifndef UI_H
#define UI_H

#include "converter.h"
#include "presets.h"
#include <raylib.h>

#define MAX_FILES 100

/* Application state */
typedef enum {
    STATE_IDLE,         /* No image loaded */
    STATE_LOADED,       /* Image loaded, ready to convert */
    STATE_CONVERTING,   /* Conversion in progress */
    STATE_SUCCESS,      /* Conversion completed */
    STATE_ERROR         /* Error occurred */
} AppState;

/* File entry for batch processing */
typedef struct {
    char input_path[512];
    char output_path[512];
    char filename[256];
    size_t file_size;
    bool converted;
    bool failed;
} FileEntry;

/* UI context */
typedef struct {
    /* Window */
    int window_width;
    int window_height;

    /* State */
    AppState state;
    char status_message[256];

    /* Multiple files */
    FileEntry files[MAX_FILES];
    int file_count;
    int current_file;       /* Currently previewed file */

    /* Preview */
    ImageData image;
    Texture2D preview_texture;
    bool has_preview;

    /* Conversion settings */
    ConversionParams params;
    int selected_preset;

    /* Output directory */
    char output_dir[512];
    bool use_same_dir;      /* Save in same directory as source */

    /* Results */
    ConversionResult last_result;
    int converted_count;
    int failed_count;
    size_t total_saved_bytes;

    /* UI state */
    float preview_scale;
    Vector2 preview_offset;
    bool show_advanced;
    bool show_popup;        /* Show completion popup */
    bool dragging_preview;
    int scroll_offset;      /* For file list scrolling */

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
void ui_handle_drop(UIContext *ctx, const char **filepaths, int count);

/* Add files to the list */
void ui_add_files(UIContext *ctx, const char **filepaths, int count);

/* Load image for preview */
bool ui_load_preview(UIContext *ctx, int file_index);

/* Start conversion of all files */
void ui_start_conversion(UIContext *ctx);

/* Clear all files */
void ui_clear_files(UIContext *ctx);

#endif /* UI_H */
