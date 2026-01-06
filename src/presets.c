/*
 * WebP Converter - Preset definitions implementation
 */

#include "presets.h"
#include <string.h>

static const Preset all_presets[PRESET_COUNT] = {
    /* Quality-based presets */
    {
        .type = PRESET_LOW,
        .name = "Low",
        .description = "Maximum compression, smaller files",
        .params = {
            .quality = 50.0f,
            .method = 4,
            .lossless = false,
            .alpha_quality = 50.0f,
            .filter_strength = 80,
            .filter_sharpness = 0,
            .preprocessing = 0
        }
    },
    {
        .type = PRESET_MEDIUM,
        .name = "Medium",
        .description = "Balanced quality and size",
        .params = {
            .quality = 75.0f,
            .method = 4,
            .lossless = false,
            .alpha_quality = 80.0f,
            .filter_strength = 60,
            .filter_sharpness = 0,
            .preprocessing = 0
        }
    },
    {
        .type = PRESET_HIGH,
        .name = "High",
        .description = "High quality, larger files",
        .params = {
            .quality = 90.0f,
            .method = 5,
            .lossless = false,
            .alpha_quality = 100.0f,
            .filter_strength = 40,
            .filter_sharpness = 0,
            .preprocessing = 0
        }
    },
    {
        .type = PRESET_LOSSLESS,
        .name = "Lossless",
        .description = "Perfect quality, no compression loss",
        .params = {
            .quality = 100.0f,
            .method = 6,
            .lossless = true,
            .alpha_quality = 100.0f,
            .filter_strength = 0,
            .filter_sharpness = 0,
            .preprocessing = 0
        }
    },

    /* Use-case based presets */
    {
        .type = PRESET_WEB,
        .name = "Web",
        .description = "Optimized for web pages",
        .params = {
            .quality = 75.0f,
            .method = 4,
            .lossless = false,
            .alpha_quality = 80.0f,
            .filter_strength = 60,
            .filter_sharpness = 0,
            .preprocessing = 1
        }
    },
    {
        .type = PRESET_PHOTO,
        .name = "Photo",
        .description = "Best for photographs",
        .params = {
            .quality = 95.0f,
            .method = 6,
            .lossless = false,
            .alpha_quality = 100.0f,
            .filter_strength = 20,
            .filter_sharpness = 0,
            .preprocessing = 0
        }
    },
    {
        .type = PRESET_THUMBNAIL,
        .name = "Thumbnail",
        .description = "Small previews",
        .params = {
            .quality = 70.0f,
            .method = 3,
            .lossless = false,
            .alpha_quality = 70.0f,
            .filter_strength = 70,
            .filter_sharpness = 0,
            .preprocessing = 0
        }
    }
};

const Preset* presets_get_all(void) {
    return all_presets;
}

const Preset* presets_get(PresetType type) {
    if (type >= 0 && type < PRESET_COUNT) {
        return &all_presets[type];
    }
    return &all_presets[PRESET_MEDIUM];
}

void presets_apply(PresetType type, ConversionParams *params) {
    if (!params) return;

    const Preset *preset = presets_get(type);
    memcpy(params, &preset->params, sizeof(ConversionParams));
}

const char* presets_get_name(PresetType type) {
    const Preset *preset = presets_get(type);
    return preset->name;
}
