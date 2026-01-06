/*
 * WebP Converter - UI module implementation
 */

#include "ui.h"
#include <stdio.h>
#include <string.h>
#include <libgen.h>

#define RAYGUI_IMPLEMENTATION
#include "raygui.h"

#include "tinyfiledialogs.h"

/* Colors */
#define COLOR_BACKGROUND    CLITERAL(Color){ 30, 30, 35, 255 }
#define COLOR_PANEL         CLITERAL(Color){ 45, 45, 50, 255 }
#define COLOR_ACCENT        CLITERAL(Color){ 100, 149, 237, 255 }
#define COLOR_SUCCESS       CLITERAL(Color){ 50, 205, 50, 255 }
#define COLOR_ERROR         CLITERAL(Color){ 220, 20, 60, 255 }
#define COLOR_TEXT          CLITERAL(Color){ 220, 220, 220, 255 }
#define COLOR_TEXT_DIM      CLITERAL(Color){ 150, 150, 150, 255 }

/* Layout constants */
#define PANEL_PADDING 20
#define CONTROL_HEIGHT 30
#define CONTROL_SPACING 10

/* Forward declarations */
static void draw_sidebar(UIContext *ctx);
static void draw_preview_panel(UIContext *ctx);
static void draw_status_bar(UIContext *ctx);
static void open_file_dialog(UIContext *ctx);
static void generate_output_path(UIContext *ctx);
static const char* format_size(size_t bytes);

void ui_init(UIContext *ctx) {
    memset(ctx, 0, sizeof(UIContext));

    ctx->window_width = 1000;
    ctx->window_height = 700;
    ctx->state = STATE_IDLE;
    ctx->selected_preset = PRESET_MEDIUM;
    ctx->preview_scale = 1.0f;
    ctx->waiting_for_drop = true;

    presets_apply(PRESET_MEDIUM, &ctx->params);
    strcpy(ctx->status_message, "Drop an image or click 'Open File' to start");

    /* Configure raygui style */
    GuiSetStyle(DEFAULT, TEXT_SIZE, 14);
    GuiSetStyle(DEFAULT, BACKGROUND_COLOR, ColorToInt(COLOR_PANEL));
    GuiSetStyle(DEFAULT, TEXT_COLOR_NORMAL, ColorToInt(COLOR_TEXT));
    GuiSetStyle(BUTTON, BASE_COLOR_NORMAL, ColorToInt(COLOR_ACCENT));
}

void ui_cleanup(UIContext *ctx) {
    if (ctx->has_preview) {
        UnloadTexture(ctx->preview_texture);
        ctx->has_preview = false;
    }
    converter_free_image(&ctx->image);
}

static void open_file_dialog(UIContext *ctx) {
    const char *filters[] = { "*.png", "*.jpg", "*.jpeg", "*.bmp", "*.gif" };
    const char *filepath = tinyfd_openFileDialog(
        "Select Image",
        "",
        5,
        filters,
        "Image files",
        0
    );

    if (filepath) {
        ui_load_image(ctx, filepath);
    }
}

bool ui_load_image(UIContext *ctx, const char *filepath) {
    /* Free previous image */
    if (ctx->has_preview) {
        UnloadTexture(ctx->preview_texture);
        ctx->has_preview = false;
    }
    converter_free_image(&ctx->image);

    /* Check if file is supported */
    if (!converter_is_supported(filepath)) {
        ctx->state = STATE_ERROR;
        snprintf(ctx->status_message, sizeof(ctx->status_message),
                "Unsupported file format");
        return false;
    }

    /* Load the image */
    if (!converter_load_image(filepath, &ctx->image)) {
        ctx->state = STATE_ERROR;
        snprintf(ctx->status_message, sizeof(ctx->status_message),
                "Failed to load image");
        return false;
    }

    /* Create preview texture */
    Image raylib_img = {
        .data = ctx->image.data,
        .width = ctx->image.width,
        .height = ctx->image.height,
        .format = PIXELFORMAT_UNCOMPRESSED_R8G8B8A8,
        .mipmaps = 1
    };
    ctx->preview_texture = LoadTextureFromImage(raylib_img);
    ctx->has_preview = true;

    /* Reset preview transform */
    ctx->preview_scale = 1.0f;
    ctx->preview_offset = (Vector2){ 0, 0 };

    /* Generate output path */
    generate_output_path(ctx);

    /* Update state */
    ctx->state = STATE_LOADED;
    ctx->waiting_for_drop = false;
    snprintf(ctx->status_message, sizeof(ctx->status_message),
            "Loaded: %dx%d - %s",
            ctx->image.width, ctx->image.height,
            format_size(ctx->image.file_size));

    return true;
}

static void generate_output_path(UIContext *ctx) {
    /* Generate output path: same directory, same name, .webp extension */
    char temp[512];
    strncpy(temp, ctx->image.filepath, sizeof(temp) - 1);

    /* Find last dot */
    char *dot = strrchr(temp, '.');
    if (dot) {
        strcpy(dot, ".webp");
    } else {
        strcat(temp, ".webp");
    }

    strncpy(ctx->output_path, temp, sizeof(ctx->output_path) - 1);
}

void ui_start_conversion(UIContext *ctx) {
    if (ctx->state != STATE_LOADED) return;

    ctx->state = STATE_CONVERTING;
    strcpy(ctx->status_message, "Converting...");

    /* Perform conversion */
    ctx->last_result = converter_to_webp(&ctx->image, ctx->output_path, &ctx->params);

    if (ctx->last_result.success) {
        ctx->state = STATE_SUCCESS;
        snprintf(ctx->status_message, sizeof(ctx->status_message),
                "Saved! %s -> %s (%.1fx compression)",
                format_size(ctx->image.file_size),
                format_size(ctx->last_result.output_size),
                ctx->last_result.compression_ratio);
    } else {
        ctx->state = STATE_ERROR;
        snprintf(ctx->status_message, sizeof(ctx->status_message),
                "Error: %s", ctx->last_result.error_message);
    }
}

void ui_handle_drop(UIContext *ctx, const char *filepath) {
    ui_load_image(ctx, filepath);
}

void ui_update(UIContext *ctx) {
    /* Handle file drop */
    if (IsFileDropped()) {
        FilePathList dropped = LoadDroppedFiles();
        if (dropped.count > 0) {
            ui_handle_drop(ctx, dropped.paths[0]);
        }
        UnloadDroppedFiles(dropped);
    }

    /* Handle mouse wheel for zoom */
    float wheel = GetMouseWheelMove();
    if (wheel != 0 && ctx->has_preview) {
        ctx->preview_scale += wheel * 0.1f;
        if (ctx->preview_scale < 0.1f) ctx->preview_scale = 0.1f;
        if (ctx->preview_scale > 5.0f) ctx->preview_scale = 5.0f;
    }

    /* Begin drawing */
    BeginDrawing();
    ClearBackground(COLOR_BACKGROUND);

    /* Draw panels */
    draw_preview_panel(ctx);
    draw_sidebar(ctx);
    draw_status_bar(ctx);

    EndDrawing();
}

static void draw_preview_panel(UIContext *ctx) {
    int sidebar_width = 300;
    int status_height = 40;
    Rectangle panel = {
        0, 0,
        ctx->window_width - sidebar_width,
        ctx->window_height - status_height
    };

    DrawRectangleRec(panel, COLOR_PANEL);
    DrawRectangleLinesEx(panel, 1, COLOR_BACKGROUND);

    if (ctx->has_preview) {
        /* Calculate scaled size */
        float scale = ctx->preview_scale;

        /* Fit to panel if scale is 1 */
        float fit_scale_x = (panel.width - 40) / ctx->preview_texture.width;
        float fit_scale_y = (panel.height - 40) / ctx->preview_texture.height;
        float fit_scale = (fit_scale_x < fit_scale_y) ? fit_scale_x : fit_scale_y;
        if (fit_scale > 1.0f) fit_scale = 1.0f;

        float display_scale = fit_scale * scale;
        float display_width = ctx->preview_texture.width * display_scale;
        float display_height = ctx->preview_texture.height * display_scale;

        /* Center in panel */
        float x = panel.x + (panel.width - display_width) / 2 + ctx->preview_offset.x;
        float y = panel.y + (panel.height - display_height) / 2 + ctx->preview_offset.y;

        /* Draw checkerboard pattern for transparency */
        int check_size = 10;
        for (int cy = (int)y; cy < y + display_height; cy += check_size) {
            for (int cx = (int)x; cx < x + display_width; cx += check_size) {
                Color c = ((cx / check_size + cy / check_size) % 2 == 0) ?
                          CLITERAL(Color){ 80, 80, 80, 255 } :
                          CLITERAL(Color){ 60, 60, 60, 255 };
                DrawRectangle(cx, cy, check_size, check_size, c);
            }
        }

        /* Draw image */
        DrawTextureEx(ctx->preview_texture, (Vector2){ x, y }, 0, display_scale, WHITE);

        /* Draw dimensions */
        char dims[64];
        snprintf(dims, sizeof(dims), "%dx%d", ctx->image.width, ctx->image.height);
        DrawText(dims, panel.x + 10, panel.y + panel.height - 25, 14, COLOR_TEXT_DIM);
    } else {
        /* Drop zone */
        const char *drop_text = "Drop image here";
        int text_width = MeasureText(drop_text, 24);
        DrawText(drop_text,
                panel.x + (panel.width - text_width) / 2,
                panel.y + panel.height / 2 - 12,
                24, COLOR_TEXT_DIM);

        const char *formats = "PNG, JPEG, BMP, GIF";
        int fmt_width = MeasureText(formats, 16);
        DrawText(formats,
                panel.x + (panel.width - fmt_width) / 2,
                panel.y + panel.height / 2 + 20,
                16, COLOR_TEXT_DIM);
    }
}

static void draw_sidebar(UIContext *ctx) {
    int sidebar_width = 300;
    int status_height = 40;
    Rectangle sidebar = {
        ctx->window_width - sidebar_width, 0,
        sidebar_width,
        ctx->window_height - status_height
    };

    DrawRectangleRec(sidebar, COLOR_PANEL);

    int x = sidebar.x + PANEL_PADDING;
    int y = PANEL_PADDING;
    int w = sidebar_width - PANEL_PADDING * 2;

    /* Open File button */
    if (GuiButton((Rectangle){ x, y, w, 40 }, "Open File...")) {
        open_file_dialog(ctx);
    }
    y += 50;

    /* Presets section */
    DrawText("PRESETS", x, y, 12, COLOR_TEXT_DIM);
    y += 20;

    /* Quality presets row */
    int btn_w = (w - 10) / 2;
    const char *preset_names[] = { "Low", "Medium", "High", "Lossless" };
    for (int i = 0; i < 4; i++) {
        int bx = x + (i % 2) * (btn_w + 10);
        int by = y + (i / 2) * 35;
        bool selected = (ctx->selected_preset == i);

        if (selected) {
            GuiSetStyle(BUTTON, BASE_COLOR_NORMAL, ColorToInt(COLOR_ACCENT));
        }

        if (GuiButton((Rectangle){ bx, by, btn_w, 30 }, preset_names[i])) {
            ctx->selected_preset = i;
            presets_apply(i, &ctx->params);
        }

        if (selected) {
            GuiSetStyle(BUTTON, BASE_COLOR_NORMAL, ColorToInt(CLITERAL(Color){ 70, 70, 75, 255 }));
        }
    }
    y += 80;

    /* Use-case presets */
    const char *use_case_names[] = { "Web", "Photo", "Thumb" };
    for (int i = 0; i < 3; i++) {
        int bx = x + i * (w / 3 + 3);
        int bw = w / 3 - 5;
        bool selected = (ctx->selected_preset == PRESET_WEB + i);

        if (selected) {
            GuiSetStyle(BUTTON, BASE_COLOR_NORMAL, ColorToInt(COLOR_ACCENT));
        }

        if (GuiButton((Rectangle){ bx, y, bw, 30 }, use_case_names[i])) {
            ctx->selected_preset = PRESET_WEB + i;
            presets_apply(PRESET_WEB + i, &ctx->params);
        }

        if (selected) {
            GuiSetStyle(BUTTON, BASE_COLOR_NORMAL, ColorToInt(CLITERAL(Color){ 70, 70, 75, 255 }));
        }
    }
    y += 50;

    /* Divider */
    DrawLine(x, y, x + w, y, COLOR_BACKGROUND);
    y += 10;

    /* Settings section */
    DrawText("SETTINGS", x, y, 12, COLOR_TEXT_DIM);
    y += 25;

    /* Quality slider */
    DrawText("Quality", x, y, 14, COLOR_TEXT);
    char quality_text[16];
    snprintf(quality_text, sizeof(quality_text), "%.0f", ctx->params.quality);
    DrawText(quality_text, x + w - 30, y, 14, COLOR_TEXT);
    y += 20;
    GuiSlider((Rectangle){ x, y, w, 20 }, NULL, NULL, &ctx->params.quality, 0, 100);
    y += 35;

    /* Method slider */
    DrawText("Compression effort", x, y, 14, COLOR_TEXT);
    char method_text[16];
    snprintf(method_text, sizeof(method_text), "%d", ctx->params.method);
    DrawText(method_text, x + w - 20, y, 14, COLOR_TEXT);
    y += 20;
    float method_f = (float)ctx->params.method;
    GuiSlider((Rectangle){ x, y, w, 20 }, NULL, NULL, &method_f, 0, 6);
    ctx->params.method = (int)method_f;
    y += 35;

    /* Lossless toggle */
    bool lossless = ctx->params.lossless;
    GuiCheckBox((Rectangle){ x, y, 20, 20 }, "Lossless compression", &lossless);
    ctx->params.lossless = lossless;
    y += 35;

    /* Advanced toggle */
    GuiCheckBox((Rectangle){ x, y, 20, 20 }, "Show advanced options", &ctx->show_advanced);
    y += 30;

    if (ctx->show_advanced) {
        /* Alpha quality */
        DrawText("Alpha quality", x, y, 14, COLOR_TEXT);
        y += 20;
        GuiSlider((Rectangle){ x, y, w, 20 }, NULL, NULL, &ctx->params.alpha_quality, 0, 100);
        y += 30;

        /* Filter strength */
        DrawText("Filter strength", x, y, 14, COLOR_TEXT);
        y += 20;
        float filter_f = (float)ctx->params.filter_strength;
        GuiSlider((Rectangle){ x, y, w, 20 }, NULL, NULL, &filter_f, 0, 100);
        ctx->params.filter_strength = (int)filter_f;
        y += 30;
    }

    /* Spacer to push convert button to bottom */
    y = ctx->window_height - status_height - 70;

    /* Estimated size */
    if (ctx->has_preview) {
        size_t est = converter_estimate_size(&ctx->image, &ctx->params);
        char est_text[128];
        snprintf(est_text, sizeof(est_text), "Est. size: ~%s", format_size(est));
        DrawText(est_text, x, y, 14, COLOR_TEXT_DIM);
    }
    y += 25;

    /* Convert button */
    bool can_convert = (ctx->state == STATE_LOADED || ctx->state == STATE_SUCCESS);
    GuiSetStyle(BUTTON, BASE_COLOR_NORMAL,
        can_convert ? ColorToInt(COLOR_SUCCESS) : ColorToInt(CLITERAL(Color){ 60, 60, 65, 255 }));

    if (GuiButton((Rectangle){ x, y, w, 40 }, "Convert to WebP") && can_convert) {
        ui_start_conversion(ctx);
    }

    GuiSetStyle(BUTTON, BASE_COLOR_NORMAL, ColorToInt(CLITERAL(Color){ 70, 70, 75, 255 }));
}

static void draw_status_bar(UIContext *ctx) {
    int status_height = 40;
    Rectangle status_bar = {
        0, ctx->window_height - status_height,
        ctx->window_width, status_height
    };

    Color bar_color = COLOR_PANEL;
    Color text_color = COLOR_TEXT;

    if (ctx->state == STATE_SUCCESS) {
        bar_color = CLITERAL(Color){ 30, 60, 30, 255 };
        text_color = COLOR_SUCCESS;
    } else if (ctx->state == STATE_ERROR) {
        bar_color = CLITERAL(Color){ 60, 30, 30, 255 };
        text_color = COLOR_ERROR;
    }

    DrawRectangleRec(status_bar, bar_color);
    DrawText(ctx->status_message, 15, status_bar.y + 12, 16, text_color);
}

static const char* format_size(size_t bytes) {
    static char buffer[32];

    if (bytes < 1024) {
        snprintf(buffer, sizeof(buffer), "%zu B", bytes);
    } else if (bytes < 1024 * 1024) {
        snprintf(buffer, sizeof(buffer), "%.1f KB", bytes / 1024.0);
    } else {
        snprintf(buffer, sizeof(buffer), "%.2f MB", bytes / (1024.0 * 1024.0));
    }

    return buffer;
}
