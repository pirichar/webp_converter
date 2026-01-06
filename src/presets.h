/*
 * WebP Converter - Preset definitions
 */

#ifndef PRESETS_H
#define PRESETS_H

#include "converter.h"

/* Preset types */
typedef enum {
    /* Quality-based presets */
    PRESET_LOW,
    PRESET_MEDIUM,
    PRESET_HIGH,
    PRESET_LOSSLESS,

    /* Use-case based presets */
    PRESET_WEB,
    PRESET_PHOTO,
    PRESET_THUMBNAIL,

    PRESET_COUNT
} PresetType;

/* Preset definition */
typedef struct {
    PresetType type;
    const char *name;
    const char *description;
    ConversionParams params;
} Preset;

/* Get all presets */
const Preset* presets_get_all(void);

/* Get a specific preset by type */
const Preset* presets_get(PresetType type);

/* Apply a preset to params */
void presets_apply(PresetType type, ConversionParams *params);

/* Get preset name */
const char* presets_get_name(PresetType type);

#endif /* PRESETS_H */
