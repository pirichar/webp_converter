/*
 * WebP Converter - Internationalization strings
 */

#ifndef STRINGS_H
#define STRINGS_H

typedef enum {
    LANG_EN,
    LANG_FR,
    LANG_COUNT
} Language;

/* String IDs */
typedef enum {
    STR_APP_TITLE,
    STR_FILES,
    STR_FILE,
    STR_ADD_FILES,
    STR_CLEAR_ALL,
    STR_OUTPUT_FOLDER,
    STR_SAME_FOLDER,
    STR_SELECT_FOLDER,
    STR_CHANGE_FOLDER,
    STR_PRESETS,
    STR_LOW,
    STR_MEDIUM,
    STR_HIGH,
    STR_LOSSLESS,
    STR_WEB,
    STR_PHOTO,
    STR_THUMB,
    STR_SETTINGS,
    STR_QUALITY,
    STR_COMPRESSION_EFFORT,
    STR_LOSSLESS_COMPRESSION,
    STR_SHOW_ADVANCED,
    STR_ALPHA_QUALITY,
    STR_FILTER_STRENGTH,
    STR_CONVERT_TO_WEBP,
    STR_CONVERT_FILES,
    STR_FILE_SELECTED,
    STR_FILES_SELECTED,
    STR_ESTIMATE,
    STR_DROP_IMAGES,
    STR_SUPPORTED_FORMATS,
    STR_CONVERSION_COMPLETE,
    STR_FILES_CONVERTED,
    STR_SAVED_SPACE,
    STR_FILES_FAILED,
    STR_OK,
    STR_CONVERTING,
    STR_DONE,
    STR_READY_TO_CONVERT,
    STR_DROP_OR_ADD,
    STR_SELECT_IMAGES,
    STR_COUNT
} StringID;

/* Get current language */
Language strings_get_language(void);

/* Set current language */
void strings_set_language(Language lang);

/* Get translated string */
const char* str(StringID id);

/* Get language name */
const char* strings_get_language_name(Language lang);

#endif /* STRINGS_H */
