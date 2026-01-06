/*
 * WebP Converter - UI module implementation
 */

#include "ui.h"
#include "strings.h"
#include <stdio.h>
#include <string.h>
#include <libgen.h>

#define RAYGUI_IMPLEMENTATION
#include "raygui.h"

#include "tinyfiledialogs.h"

/* Colors */
#define COLOR_BACKGROUND    CLITERAL(Color){ 30, 30, 35, 255 }
#define COLOR_PANEL         CLITERAL(Color){ 45, 45, 50, 255 }
#define COLOR_PANEL_DARK    CLITERAL(Color){ 35, 35, 40, 255 }
#define COLOR_ACCENT        CLITERAL(Color){ 100, 149, 237, 255 }
#define COLOR_SUCCESS       CLITERAL(Color){ 50, 205, 50, 255 }
#define COLOR_ERROR         CLITERAL(Color){ 220, 20, 60, 255 }
#define COLOR_TEXT          CLITERAL(Color){ 220, 220, 220, 255 }
#define COLOR_TEXT_DIM      CLITERAL(Color){ 150, 150, 150, 255 }
#define COLOR_SELECTED      CLITERAL(Color){ 70, 130, 180, 255 }

/* Layout constants */
#define PANEL_PADDING 20
#define CONTROL_HEIGHT 30
#define CONTROL_SPACING 10
#define FILE_LIST_ITEM_HEIGHT 25

/* Forward declarations */
static void draw_sidebar(UIContext *ctx);
static void draw_preview_panel(UIContext *ctx);
static void draw_file_list(UIContext *ctx);
static void draw_status_bar(UIContext *ctx);
static void draw_popup(UIContext *ctx);
static void open_file_dialog(UIContext *ctx);
static void generate_output_paths(UIContext *ctx);
static const char* format_size(size_t bytes);
static const char* get_filename(const char *path);

void ui_init(UIContext *ctx) {
    memset(ctx, 0, sizeof(UIContext));

    ctx->window_width = 1100;
    ctx->window_height = 750;
    ctx->state = STATE_IDLE;
    ctx->selected_preset = PRESET_MEDIUM;
    ctx->preview_scale = 1.0f;
    ctx->waiting_for_drop = true;
    ctx->use_same_dir = true;
    ctx->current_file = -1;

    presets_apply(PRESET_MEDIUM, &ctx->params);
    strncpy(ctx->status_message, str(STR_DROP_OR_ADD), sizeof(ctx->status_message) - 1);

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

void ui_clear_files(UIContext *ctx) {
    if (ctx->has_preview) {
        UnloadTexture(ctx->preview_texture);
        ctx->has_preview = false;
    }
    converter_free_image(&ctx->image);

    ctx->file_count = 0;
    ctx->current_file = -1;
    ctx->converted_count = 0;
    ctx->failed_count = 0;
    ctx->total_input_size = 0;
    ctx->total_output_size = 0;
    ctx->state = STATE_IDLE;
    strncpy(ctx->status_message, str(STR_DROP_OR_ADD), sizeof(ctx->status_message) - 1);
}

static void open_file_dialog(UIContext *ctx) {
    const char *filters[] = { "*.png", "*.jpg", "*.jpeg", "*.bmp", "*.gif" };
    const char *result = tinyfd_openFileDialog(
        str(STR_SELECT_IMAGES),
        "",
        5,
        filters,
        "Image files",
        1  /* Allow multiple selection */
    );

    if (result) {
        /* tinyfiledialogs returns paths separated by | for multiple files */
        char *paths = strdup(result);
        char *token = strtok(paths, "|");
        const char *file_list[MAX_FILES];
        int count = 0;

        while (token && count < MAX_FILES) {
            file_list[count++] = token;
            token = strtok(NULL, "|");
        }

        ui_add_files(ctx, file_list, count);
        free(paths);
    }
}

void ui_add_files(UIContext *ctx, const char **filepaths, int count) {
    for (int i = 0; i < count && ctx->file_count < MAX_FILES; i++) {
        const char *path = filepaths[i];

        /* Check if file is supported */
        if (!converter_is_supported(path)) continue;

        /* Check if already in list */
        bool duplicate = false;
        for (int j = 0; j < ctx->file_count; j++) {
            if (strcmp(ctx->files[j].input_path, path) == 0) {
                duplicate = true;
                break;
            }
        }
        if (duplicate) continue;

        /* Add to list */
        FileEntry *entry = &ctx->files[ctx->file_count];
        strncpy(entry->input_path, path, sizeof(entry->input_path) - 1);
        strncpy(entry->filename, get_filename(path), sizeof(entry->filename) - 1);

        /* Get file size */
        ImageData temp;
        if (converter_load_image(path, &temp)) {
            entry->file_size = temp.file_size;
            converter_free_image(&temp);
        }

        entry->converted = false;
        entry->failed = false;

        ctx->file_count++;
    }

    /* Generate output paths */
    generate_output_paths(ctx);

    /* Load preview of first file if none selected */
    if (ctx->file_count > 0 && ctx->current_file < 0) {
        ui_load_preview(ctx, 0);
    }

    if (ctx->file_count > 0) {
        ctx->state = STATE_LOADED;
        snprintf(ctx->status_message, sizeof(ctx->status_message),
                str(STR_READY_TO_CONVERT), ctx->file_count);
    }
}

static void generate_output_paths(UIContext *ctx) {
    for (int i = 0; i < ctx->file_count; i++) {
        FileEntry *entry = &ctx->files[i];
        char temp[512];

        if (ctx->use_same_dir) {
            strncpy(temp, entry->input_path, sizeof(temp) - 1);
        } else {
            snprintf(temp, sizeof(temp), "%s/%s",
                    ctx->output_dir, entry->filename);
        }

        /* Replace extension with .webp */
        char *dot = strrchr(temp, '.');
        if (dot) {
            strcpy(dot, ".webp");
        } else {
            strcat(temp, ".webp");
        }

        strncpy(entry->output_path, temp, sizeof(entry->output_path) - 1);
    }
}

bool ui_load_preview(UIContext *ctx, int file_index) {
    if (file_index < 0 || file_index >= ctx->file_count) return false;

    /* Free previous preview */
    if (ctx->has_preview) {
        UnloadTexture(ctx->preview_texture);
        ctx->has_preview = false;
    }
    converter_free_image(&ctx->image);

    FileEntry *entry = &ctx->files[file_index];

    /* Load the image */
    if (!converter_load_image(entry->input_path, &ctx->image)) {
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
    ctx->current_file = file_index;

    /* Reset preview transform */
    ctx->preview_scale = 1.0f;
    ctx->preview_offset = (Vector2){ 0, 0 };

    return true;
}

void ui_start_conversion(UIContext *ctx) {
    if (ctx->file_count == 0) return;

    ctx->state = STATE_CONVERTING;
    ctx->converted_count = 0;
    ctx->failed_count = 0;
    ctx->total_input_size = 0;
    ctx->total_output_size = 0;

    for (int i = 0; i < ctx->file_count; i++) {
        FileEntry *entry = &ctx->files[i];
        entry->output_size = 0;

        snprintf(ctx->status_message, sizeof(ctx->status_message),
                str(STR_CONVERTING), i + 1, ctx->file_count, entry->filename);

        /* Load image */
        ImageData img;
        if (!converter_load_image(entry->input_path, &img)) {
            entry->failed = true;
            ctx->failed_count++;
            continue;
        }

        /* Convert */
        ConversionResult result = converter_to_webp(&img, entry->output_path, &ctx->params);
        converter_free_image(&img);

        if (result.success) {
            entry->converted = true;
            entry->output_size = result.output_size;
            ctx->converted_count++;
            ctx->total_input_size += entry->file_size;
            ctx->total_output_size += result.output_size;
        } else {
            entry->failed = true;
            ctx->failed_count++;
        }
    }

    /* Show completion popup */
    ctx->state = STATE_SUCCESS;
    ctx->show_popup = true;

    snprintf(ctx->status_message, sizeof(ctx->status_message),
            str(STR_DONE), ctx->converted_count, ctx->failed_count);
}

void ui_handle_drop(UIContext *ctx, const char **filepaths, int count) {
    ui_add_files(ctx, filepaths, count);
}

void ui_update(UIContext *ctx) {
    /* Handle file drop */
    if (IsFileDropped()) {
        FilePathList dropped = LoadDroppedFiles();
        const char **paths = (const char **)dropped.paths;
        ui_handle_drop(ctx, paths, dropped.count);
        UnloadDroppedFiles(dropped);
    }

    /* Handle mouse wheel for zoom (only when not over file list) */
    float wheel = GetMouseWheelMove();
    if (wheel != 0 && ctx->has_preview) {
        Vector2 mouse = GetMousePosition();
        int sidebar_width = 320;
        int file_list_height = 150;
        if (mouse.x < ctx->window_width - sidebar_width &&
            mouse.y > file_list_height) {
            ctx->preview_scale += wheel * 0.1f;
            if (ctx->preview_scale < 0.1f) ctx->preview_scale = 0.1f;
            if (ctx->preview_scale > 5.0f) ctx->preview_scale = 5.0f;
        }
    }

    /* Begin drawing */
    BeginDrawing();
    ClearBackground(COLOR_BACKGROUND);

    /* Draw panels */
    draw_file_list(ctx);
    draw_preview_panel(ctx);
    draw_sidebar(ctx);
    draw_status_bar(ctx);

    /* Draw popup if active */
    if (ctx->show_popup) {
        draw_popup(ctx);
    }

    EndDrawing();
}

static void draw_file_list(UIContext *ctx) {
    int sidebar_width = 320;
    int file_list_height = 150;
    Rectangle panel = {
        0, 0,
        ctx->window_width - sidebar_width,
        file_list_height
    };

    DrawRectangleRec(panel, COLOR_PANEL_DARK);
    DrawRectangleLinesEx(panel, 1, COLOR_BACKGROUND);

    /* Title */
    DrawText(str(STR_FILES), 10, 8, 12, COLOR_TEXT_DIM);

    /* File count */
    char count_text[32];
    snprintf(count_text, sizeof(count_text), "%d %s%s",
            ctx->file_count, str(STR_FILE), ctx->file_count != 1 ? "s" : "");
    DrawText(count_text, panel.width - 80, 8, 12, COLOR_TEXT_DIM);

    /* File list */
    int y = 30;
    int visible_items = (file_list_height - 35) / FILE_LIST_ITEM_HEIGHT;

    for (int i = ctx->scroll_offset; i < ctx->file_count && i < ctx->scroll_offset + visible_items; i++) {
        FileEntry *entry = &ctx->files[i];
        Rectangle item_rect = { 5, y, panel.width - 10, FILE_LIST_ITEM_HEIGHT - 2 };

        /* Highlight selected */
        if (i == ctx->current_file) {
            DrawRectangleRec(item_rect, COLOR_SELECTED);
        }

        /* Click to select */
        if (CheckCollisionPointRec(GetMousePosition(), item_rect) && IsMouseButtonPressed(MOUSE_LEFT_BUTTON)) {
            ui_load_preview(ctx, i);
        }

        /* Status indicator */
        Color status_color = COLOR_TEXT_DIM;
        if (entry->converted) status_color = COLOR_SUCCESS;
        else if (entry->failed) status_color = COLOR_ERROR;
        DrawCircle(15, y + FILE_LIST_ITEM_HEIGHT/2, 4, status_color);

        /* Filename */
        DrawText(entry->filename, 28, y + 4, 14, COLOR_TEXT);

        /* Size */
        DrawText(format_size(entry->file_size), panel.width - 80, y + 4, 12, COLOR_TEXT_DIM);

        y += FILE_LIST_ITEM_HEIGHT;
    }

    /* Scroll with mouse wheel when over file list */
    Vector2 mouse = GetMousePosition();
    if (CheckCollisionPointRec(mouse, panel)) {
        float wheel = GetMouseWheelMove();
        if (wheel != 0) {
            ctx->scroll_offset -= (int)wheel;
            if (ctx->scroll_offset < 0) ctx->scroll_offset = 0;
            if (ctx->scroll_offset > ctx->file_count - visible_items)
                ctx->scroll_offset = ctx->file_count - visible_items;
            if (ctx->scroll_offset < 0) ctx->scroll_offset = 0;
        }
    }
}

static void draw_preview_panel(UIContext *ctx) {
    int sidebar_width = 320;
    int file_list_height = 150;
    int status_height = 40;
    Rectangle panel = {
        0, file_list_height,
        ctx->window_width - sidebar_width,
        ctx->window_height - file_list_height - status_height
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
        const char *drop_text = str(STR_DROP_IMAGES);
        int text_width = MeasureText(drop_text, 24);
        DrawText(drop_text,
                panel.x + (panel.width - text_width) / 2,
                panel.y + panel.height / 2 - 12,
                24, COLOR_TEXT_DIM);

        const char *formats = str(STR_SUPPORTED_FORMATS);
        int fmt_width = MeasureText(formats, 16);
        DrawText(formats,
                panel.x + (panel.width - fmt_width) / 2,
                panel.y + panel.height / 2 + 20,
                16, COLOR_TEXT_DIM);
    }
}

static void draw_sidebar(UIContext *ctx) {
    int sidebar_width = 320;
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

    /* Language switcher */
    Language current_lang = strings_get_language();
    for (int i = 0; i < LANG_COUNT; i++) {
        bool selected = (current_lang == (Language)i);
        if (selected) {
            GuiSetStyle(BUTTON, BASE_COLOR_NORMAL, ColorToInt(COLOR_ACCENT));
        } else {
            GuiSetStyle(BUTTON, BASE_COLOR_NORMAL, ColorToInt(CLITERAL(Color){ 70, 70, 75, 255 }));
        }
        if (GuiButton((Rectangle){ x + w - 70 + i * 35, y, 30, 25 }, strings_get_language_name(i))) {
            strings_set_language(i);
        }
    }
    GuiSetStyle(BUTTON, BASE_COLOR_NORMAL, ColorToInt(CLITERAL(Color){ 70, 70, 75, 255 }));
    y += 35;

    /* Add Files / Clear buttons */
    if (GuiButton((Rectangle){ x, y, w/2 - 5, 35 }, str(STR_ADD_FILES))) {
        open_file_dialog(ctx);
    }
    if (GuiButton((Rectangle){ x + w/2 + 5, y, w/2 - 5, 35 }, str(STR_CLEAR_ALL))) {
        ui_clear_files(ctx);
    }
    y += 45;

    /* Output path info */
    DrawText(str(STR_OUTPUT_FOLDER), x, y, 12, COLOR_TEXT_DIM);
    y += 20;

    /* Toggle: same folder or custom - draw custom checkbox */
    Rectangle checkbox_rect = { x, y, 20, 20 };
    bool same_dir = ctx->use_same_dir;

    /* Draw checkbox background */
    DrawRectangleRec(checkbox_rect, same_dir ? COLOR_SUCCESS : CLITERAL(Color){ 60, 60, 65, 255 });
    DrawRectangleLinesEx(checkbox_rect, 1, same_dir ? COLOR_SUCCESS : COLOR_TEXT_DIM);

    /* Draw checkmark if checked */
    if (same_dir) {
        DrawLine(x + 4, y + 10, x + 8, y + 15, WHITE);
        DrawLine(x + 8, y + 15, x + 16, y + 5, WHITE);
        DrawLine(x + 4, y + 11, x + 8, y + 16, WHITE);
        DrawLine(x + 8, y + 16, x + 16, y + 6, WHITE);
    }

    /* Label */
    DrawText(str(STR_SAME_FOLDER), x + 28, y + 3, 14, COLOR_TEXT);

    /* Click detection */
    if (CheckCollisionPointRec(GetMousePosition(), (Rectangle){ x, y, 200, 20 }) &&
        IsMouseButtonPressed(MOUSE_LEFT_BUTTON)) {
        ctx->use_same_dir = !ctx->use_same_dir;
    }
    y += 28;

    /* Show current output folder or change button */
    if (!ctx->use_same_dir) {
        if (GuiButton((Rectangle){ x, y, w, 28 }, ctx->output_dir[0] ? str(STR_CHANGE_FOLDER) : str(STR_SELECT_FOLDER))) {
            const char *folder = tinyfd_selectFolderDialog("Select Output Folder", "");
            if (folder) {
                strncpy(ctx->output_dir, folder, sizeof(ctx->output_dir) - 1);
                generate_output_paths(ctx);
            }
        }
        y += 32;

        /* Show selected folder */
        if (ctx->output_dir[0]) {
            char display_dir[48];
            if (strlen(ctx->output_dir) > 35) {
                snprintf(display_dir, sizeof(display_dir), "...%s",
                        ctx->output_dir + strlen(ctx->output_dir) - 32);
            } else {
                strncpy(display_dir, ctx->output_dir, sizeof(display_dir) - 1);
            }
            DrawText(display_dir, x, y, 11, COLOR_TEXT_DIM);
            y += 18;
        }
    } else {
        /* Regenerate paths if switched back to same dir */
        if (ctx->file_count > 0) {
            generate_output_paths(ctx);
        }
        y += 5;
    }

    /* Divider */
    DrawLine(x, y, x + w, y, COLOR_BACKGROUND);
    y += 15;

    /* Presets section */
    DrawText(str(STR_PRESETS), x, y, 12, COLOR_TEXT_DIM);
    y += 20;

    /* Quality presets row */
    int btn_w = (w - 10) / 2;
    const char *preset_names[] = { str(STR_LOW), str(STR_MEDIUM), str(STR_HIGH), str(STR_LOSSLESS) };
    for (int i = 0; i < 4; i++) {
        int bx = x + (i % 2) * (btn_w + 10);
        int by = y + (i / 2) * 35;
        bool selected = (ctx->selected_preset == i);

        if (selected) {
            GuiSetStyle(BUTTON, BASE_COLOR_NORMAL, ColorToInt(COLOR_ACCENT));
        } else {
            GuiSetStyle(BUTTON, BASE_COLOR_NORMAL, ColorToInt(CLITERAL(Color){ 70, 70, 75, 255 }));
        }

        if (GuiButton((Rectangle){ bx, by, btn_w, 30 }, preset_names[i])) {
            ctx->selected_preset = i;
            presets_apply(i, &ctx->params);
        }
    }
    y += 80;

    /* Use-case presets */
    const char *use_case_names[] = { str(STR_WEB), str(STR_PHOTO), str(STR_THUMB) };
    for (int i = 0; i < 3; i++) {
        int bx = x + i * (w / 3 + 3);
        int bw = w / 3 - 5;
        bool selected = (ctx->selected_preset == PRESET_WEB + i);

        if (selected) {
            GuiSetStyle(BUTTON, BASE_COLOR_NORMAL, ColorToInt(COLOR_ACCENT));
        } else {
            GuiSetStyle(BUTTON, BASE_COLOR_NORMAL, ColorToInt(CLITERAL(Color){ 70, 70, 75, 255 }));
        }

        if (GuiButton((Rectangle){ bx, y, bw, 30 }, use_case_names[i])) {
            ctx->selected_preset = PRESET_WEB + i;
            presets_apply(PRESET_WEB + i, &ctx->params);
        }
    }
    y += 50;

    /* Divider */
    DrawLine(x, y, x + w, y, COLOR_BACKGROUND);
    y += 10;

    /* Settings section */
    DrawText(str(STR_SETTINGS), x, y, 12, COLOR_TEXT_DIM);
    y += 25;

    GuiSetStyle(BUTTON, BASE_COLOR_NORMAL, ColorToInt(CLITERAL(Color){ 70, 70, 75, 255 }));

    /* Quality slider */
    DrawText(str(STR_QUALITY), x, y, 14, COLOR_TEXT);
    char quality_text[16];
    snprintf(quality_text, sizeof(quality_text), "%.0f", ctx->params.quality);
    DrawText(quality_text, x + w - 30, y, 14, COLOR_TEXT);
    y += 20;
    GuiSlider((Rectangle){ x, y, w, 20 }, NULL, NULL, &ctx->params.quality, 0, 100);
    y += 35;

    /* Method slider */
    DrawText(str(STR_COMPRESSION_EFFORT), x, y, 14, COLOR_TEXT);
    char method_text[16];
    snprintf(method_text, sizeof(method_text), "%d", ctx->params.method);
    DrawText(method_text, x + w - 20, y, 14, COLOR_TEXT);
    y += 20;
    float method_f = (float)ctx->params.method;
    GuiSlider((Rectangle){ x, y, w, 20 }, NULL, NULL, &method_f, 0, 6);
    ctx->params.method = (int)method_f;
    y += 35;

    /* Lossless toggle - custom checkbox */
    {
        Rectangle cb = { x, y, 20, 20 };
        bool checked = ctx->params.lossless;
        DrawRectangleRec(cb, checked ? COLOR_SUCCESS : CLITERAL(Color){ 60, 60, 65, 255 });
        DrawRectangleLinesEx(cb, 1, checked ? COLOR_SUCCESS : COLOR_TEXT_DIM);
        if (checked) {
            DrawLine(x + 4, y + 10, x + 8, y + 15, WHITE);
            DrawLine(x + 8, y + 15, x + 16, y + 5, WHITE);
            DrawLine(x + 4, y + 11, x + 8, y + 16, WHITE);
            DrawLine(x + 8, y + 16, x + 16, y + 6, WHITE);
        }
        DrawText(str(STR_LOSSLESS_COMPRESSION), x + 28, y + 3, 14, COLOR_TEXT);
        if (CheckCollisionPointRec(GetMousePosition(), (Rectangle){ x, y, 200, 20 }) &&
            IsMouseButtonPressed(MOUSE_LEFT_BUTTON)) {
            ctx->params.lossless = !ctx->params.lossless;
        }
    }
    y += 35;

    /* Advanced toggle - custom checkbox */
    {
        Rectangle cb = { x, y, 20, 20 };
        bool checked = ctx->show_advanced;
        DrawRectangleRec(cb, checked ? COLOR_SUCCESS : CLITERAL(Color){ 60, 60, 65, 255 });
        DrawRectangleLinesEx(cb, 1, checked ? COLOR_SUCCESS : COLOR_TEXT_DIM);
        if (checked) {
            DrawLine(x + 4, y + 10, x + 8, y + 15, WHITE);
            DrawLine(x + 8, y + 15, x + 16, y + 5, WHITE);
            DrawLine(x + 4, y + 11, x + 8, y + 16, WHITE);
            DrawLine(x + 8, y + 16, x + 16, y + 6, WHITE);
        }
        DrawText(str(STR_SHOW_ADVANCED), x + 28, y + 3, 14, COLOR_TEXT);
        if (CheckCollisionPointRec(GetMousePosition(), (Rectangle){ x, y, 200, 20 }) &&
            IsMouseButtonPressed(MOUSE_LEFT_BUTTON)) {
            ctx->show_advanced = !ctx->show_advanced;
        }
    }
    y += 30;

    if (ctx->show_advanced) {
        /* Alpha quality */
        DrawText(str(STR_ALPHA_QUALITY), x, y, 14, COLOR_TEXT);
        y += 20;
        GuiSlider((Rectangle){ x, y, w, 20 }, NULL, NULL, &ctx->params.alpha_quality, 0, 100);
        y += 30;

        /* Filter strength */
        DrawText(str(STR_FILTER_STRENGTH), x, y, 14, COLOR_TEXT);
        y += 20;
        float filter_f = (float)ctx->params.filter_strength;
        GuiSlider((Rectangle){ x, y, w, 20 }, NULL, NULL, &filter_f, 0, 100);
        ctx->params.filter_strength = (int)filter_f;
        y += 30;
    }

    /* Spacer to push convert button to bottom */
    y = ctx->window_height - status_height - 95;

    /* Estimate section */
    if (ctx->file_count > 0) {
        /* Calculate total estimate */
        size_t total_input = 0;
        size_t total_estimate = 0;
        for (int i = 0; i < ctx->file_count; i++) {
            total_input += ctx->files[i].file_size;
            /* Estimate based on image dimensions */
            if (ctx->has_preview && i == ctx->current_file) {
                total_estimate += converter_estimate_size(&ctx->image, &ctx->params);
            } else {
                /* Rough estimate based on file size */
                /* Assume raw pixels ~3x compressed file size, then apply WebP ratio */
                size_t approx_raw = ctx->files[i].file_size * 3;
                float qf = ctx->params.quality / 100.0f;
                float ratio = ctx->params.lossless ? 0.3f : (0.001f + qf * qf * 0.025f);
                total_estimate += (size_t)(approx_raw * ratio);
            }
        }

        /* Show input size -> estimated output */
        char input_str[32], est_str[32];
        snprintf(input_str, sizeof(input_str), "%s", format_size(total_input));
        snprintf(est_str, sizeof(est_str), "%s", format_size(total_estimate));

        char estimate_text[128];
        snprintf(estimate_text, sizeof(estimate_text), str(STR_ESTIMATE), input_str, est_str);
        DrawText(estimate_text, x, y, 14, COLOR_TEXT_DIM);
        y += 20;

        /* File count */
        char info_text[64];
        if (ctx->file_count == 1) {
            snprintf(info_text, sizeof(info_text), "%s", str(STR_FILE_SELECTED));
        } else {
            snprintf(info_text, sizeof(info_text), str(STR_FILES_SELECTED), ctx->file_count);
        }
        DrawText(info_text, x, y, 12, COLOR_TEXT_DIM);
    }
    y += 25;

    /* Convert button */
    bool can_convert = (ctx->file_count > 0 && ctx->state != STATE_CONVERTING);
    if (can_convert) {
        GuiSetStyle(BUTTON, BASE_COLOR_NORMAL, ColorToInt(COLOR_SUCCESS));
    } else {
        GuiSetStyle(BUTTON, BASE_COLOR_NORMAL, ColorToInt(CLITERAL(Color){ 60, 60, 65, 255 }));
    }

    char convert_text[64];
    if (ctx->file_count > 1) {
        snprintf(convert_text, sizeof(convert_text), str(STR_CONVERT_FILES), ctx->file_count);
    } else {
        strncpy(convert_text, str(STR_CONVERT_TO_WEBP), sizeof(convert_text) - 1);
    }

    if (GuiButton((Rectangle){ x, y, w, 40 }, convert_text) && can_convert) {
        ui_start_conversion(ctx);
    }

    GuiSetStyle(BUTTON, BASE_COLOR_NORMAL, ColorToInt(CLITERAL(Color){ 70, 70, 75, 255 }));
}

static void draw_popup(UIContext *ctx) {
    /* Dim background */
    DrawRectangle(0, 0, ctx->window_width, ctx->window_height, CLITERAL(Color){ 0, 0, 0, 150 });

    /* Popup box */
    int popup_w = 400;
    int popup_h = 200;
    int popup_x = (ctx->window_width - popup_w) / 2;
    int popup_y = (ctx->window_height - popup_h) / 2;

    DrawRectangle(popup_x, popup_y, popup_w, popup_h, COLOR_PANEL);
    DrawRectangleLinesEx((Rectangle){ popup_x, popup_y, popup_w, popup_h }, 2, COLOR_ACCENT);

    /* Title */
    const char *title = str(STR_CONVERSION_COMPLETE);
    int title_w = MeasureText(title, 24);
    DrawText(title, popup_x + (popup_w - title_w) / 2, popup_y + 25, 24, COLOR_SUCCESS);

    /* Results */
    char results[128];
    snprintf(results, sizeof(results), str(STR_FILES_CONVERTED),
            ctx->converted_count, ctx->file_count);
    int results_w = MeasureText(results, 16);
    DrawText(results, popup_x + (popup_w - results_w) / 2, popup_y + 60, 16, COLOR_TEXT);

    /* Size info */
    if (ctx->total_output_size > 0) {
        char input_str[32], output_str[32], size_info[128];
        snprintf(input_str, sizeof(input_str), "%s", format_size(ctx->total_input_size));
        snprintf(output_str, sizeof(output_str), "%s", format_size(ctx->total_output_size));
        snprintf(size_info, sizeof(size_info), "%s  ->  %s", input_str, output_str);
        int size_w = MeasureText(size_info, 16);
        DrawText(size_info, popup_x + (popup_w - size_w) / 2, popup_y + 90, 16, COLOR_TEXT);

        /* Savings percentage */
        if (ctx->total_input_size > ctx->total_output_size) {
            int percent_saved = (int)(100.0 * (ctx->total_input_size - ctx->total_output_size) / ctx->total_input_size);
            char savings[64];
            snprintf(savings, sizeof(savings), str(STR_SAVED_SPACE), percent_saved);
            int savings_w = MeasureText(savings, 14);
            DrawText(savings, popup_x + (popup_w - savings_w) / 2, popup_y + 115, 14, COLOR_SUCCESS);
        }
    }

    /* Failed count */
    if (ctx->failed_count > 0) {
        char failed[64];
        snprintf(failed, sizeof(failed), str(STR_FILES_FAILED), ctx->failed_count);
        int failed_w = MeasureText(failed, 14);
        DrawText(failed, popup_x + (popup_w - failed_w) / 2, popup_y + 140, 14, COLOR_ERROR);
    }

    /* OK button */
    GuiSetStyle(BUTTON, BASE_COLOR_NORMAL, ColorToInt(COLOR_ACCENT));
    if (GuiButton((Rectangle){ popup_x + popup_w/2 - 60, popup_y + popup_h - 50, 120, 35 }, str(STR_OK))) {
        ctx->show_popup = false;
    }
    GuiSetStyle(BUTTON, BASE_COLOR_NORMAL, ColorToInt(CLITERAL(Color){ 70, 70, 75, 255 }));
}

/* PIR logo pixel grid (24x12) */
static const unsigned char LOGO_PIXELS[12][24] = {
    {0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0},
    {0,0,1,1,1,0,0,0,0,0,0,0,0,0,1,1,1,1,1,0,0,0,0,0},
    {0,1,1,0,1,1,0,0,0,0,0,0,0,1,1,0,0,0,1,1,0,0,0,0},
    {0,1,1,0,0,1,1,0,1,1,0,0,0,1,1,0,0,0,1,1,0,0,0,0},
    {0,1,1,0,0,1,1,0,1,1,0,0,0,1,1,0,0,1,1,0,0,0,0,0},
    {0,1,1,0,1,1,0,0,1,1,0,0,0,1,1,1,1,1,0,0,0,0,0,0},
    {0,0,1,1,1,0,0,0,1,1,0,0,0,1,1,0,1,1,0,0,0,0,0,0},
    {0,1,1,0,0,0,0,0,1,1,0,0,0,1,1,0,0,1,1,0,0,0,0,0},
    {0,1,1,0,0,0,0,0,1,1,0,0,0,1,1,0,0,0,1,1,0,0,0,0},
    {0,1,1,0,0,0,0,0,1,1,0,0,0,1,1,0,0,0,0,1,1,0,0,0},
    {0,1,1,0,0,0,0,0,1,1,0,0,0,1,1,0,0,0,0,0,1,1,0,0},
    {0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0},
};

static void draw_pir_logo(int x, int y, int scale, Color color) {
    for (int row = 0; row < 12; row++) {
        for (int col = 0; col < 24; col++) {
            if (LOGO_PIXELS[row][col]) {
                DrawRectangle(x + col * scale, y + row * scale, scale, scale, color);
            }
        }
    }
}

static void draw_status_bar(UIContext *ctx) {
    int status_height = 40;
    Rectangle status_bar = {
        0, ctx->window_height - status_height,
        ctx->window_width, status_height
    };

    Color bar_color = COLOR_PANEL;
    Color text_color = COLOR_TEXT;

    if (ctx->state == STATE_SUCCESS && !ctx->show_popup) {
        bar_color = CLITERAL(Color){ 30, 60, 30, 255 };
        text_color = COLOR_SUCCESS;
    } else if (ctx->state == STATE_ERROR) {
        bar_color = CLITERAL(Color){ 60, 30, 30, 255 };
        text_color = COLOR_ERROR;
    }

    DrawRectangleRec(status_bar, bar_color);
    DrawText(ctx->status_message, 15, status_bar.y + 12, 16, text_color);

    /* Draw PIR logo in bottom right */
    Color logo_color = CLITERAL(Color){ 78, 204, 163, 255 }; /* #4ecca3 teal */
    int logo_scale = 2;
    int logo_width = 24 * logo_scale;
    int logo_height = 12 * logo_scale;
    draw_pir_logo(ctx->window_width - logo_width - 15,
                  status_bar.y + (status_height - logo_height) / 2,
                  logo_scale, logo_color);
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

static const char* get_filename(const char *path) {
    const char *slash = strrchr(path, '/');
    if (slash) return slash + 1;
    return path;
}

