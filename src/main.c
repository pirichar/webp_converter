/*
 * WebP Converter - Main application
 *
 * A macOS application for converting images to WebP format
 * with visual interface and compression controls.
 */

#include <stdio.h>
#include <raylib.h>
#include "ui.h"

int main(void) {
    /* Initialize window */
    SetConfigFlags(FLAG_WINDOW_RESIZABLE | FLAG_MSAA_4X_HINT);
    InitWindow(1100, 750, "WebP Converter");
    SetTargetFPS(60);
    SetWindowMinSize(900, 600);

    /* Initialize UI */
    UIContext ctx;
    ui_init(&ctx);

    /* Main loop */
    while (!WindowShouldClose()) {
        /* Update window size */
        ctx.window_width = GetScreenWidth();
        ctx.window_height = GetScreenHeight();

        /* Update and render */
        ui_update(&ctx);
    }

    /* Cleanup */
    ui_cleanup(&ctx);
    CloseWindow();

    return 0;
}
