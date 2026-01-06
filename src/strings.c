/*
 * WebP Converter - Internationalization strings implementation
 */

#include "strings.h"

static Language current_language = LANG_EN;

/* All strings in all languages */
static const char* all_strings[LANG_COUNT][STR_COUNT] = {
    /* English */
    [LANG_EN] = {
        [STR_APP_TITLE] = "WebP Converter",
        [STR_FILES] = "FILES",
        [STR_FILE] = "file",
        [STR_ADD_FILES] = "Add Files...",
        [STR_CLEAR_ALL] = "Clear All",
        [STR_OUTPUT_FOLDER] = "OUTPUT FOLDER",
        [STR_SAME_FOLDER] = "Same folder as source",
        [STR_SELECT_FOLDER] = "Select Folder...",
        [STR_CHANGE_FOLDER] = "Change Folder...",
        [STR_PRESETS] = "PRESETS",
        [STR_LOW] = "Low",
        [STR_MEDIUM] = "Medium",
        [STR_HIGH] = "High",
        [STR_LOSSLESS] = "Lossless",
        [STR_WEB] = "Web",
        [STR_PHOTO] = "Photo",
        [STR_THUMB] = "Thumb",
        [STR_SETTINGS] = "SETTINGS",
        [STR_QUALITY] = "Quality",
        [STR_COMPRESSION_EFFORT] = "Compression effort",
        [STR_LOSSLESS_COMPRESSION] = "Lossless compression",
        [STR_SHOW_ADVANCED] = "Show advanced options",
        [STR_ALPHA_QUALITY] = "Alpha quality",
        [STR_FILTER_STRENGTH] = "Filter strength",
        [STR_CONVERT_TO_WEBP] = "Convert to WebP",
        [STR_CONVERT_FILES] = "Convert %d Files to WebP",
        [STR_FILE_SELECTED] = "1 file selected",
        [STR_FILES_SELECTED] = "%d files selected",
        [STR_ESTIMATE] = "Est: %s -> ~%s",
        [STR_DROP_IMAGES] = "Drop images here",
        [STR_SUPPORTED_FORMATS] = "PNG, JPEG, BMP, GIF",
        [STR_CONVERSION_COMPLETE] = "Conversion Complete!",
        [STR_FILES_CONVERTED] = "%d of %d files converted successfully",
        [STR_SAVED_SPACE] = "Saved %d%% space",
        [STR_FILES_FAILED] = "%d file(s) failed",
        [STR_OK] = "OK",
        [STR_CONVERTING] = "Converting %d/%d: %s",
        [STR_DONE] = "Done! %d converted, %d failed",
        [STR_READY_TO_CONVERT] = "%d file(s) ready to convert",
        [STR_DROP_OR_ADD] = "Drop images or click 'Add Files' to start",
        [STR_SELECT_IMAGES] = "Select Images",
    },
    /* French */
    [LANG_FR] = {
        [STR_APP_TITLE] = "Convertisseur WebP",
        [STR_FILES] = "FICHIERS",
        [STR_FILE] = "fichier",
        [STR_ADD_FILES] = "Ajouter...",
        [STR_CLEAR_ALL] = "Tout effacer",
        [STR_OUTPUT_FOLDER] = "DOSSIER DE SORTIE",
        [STR_SAME_FOLDER] = "Meme dossier que source",
        [STR_SELECT_FOLDER] = "Choisir dossier...",
        [STR_CHANGE_FOLDER] = "Changer dossier...",
        [STR_PRESETS] = "PRESELECTIONS",
        [STR_LOW] = "Basse",
        [STR_MEDIUM] = "Moyenne",
        [STR_HIGH] = "Haute",
        [STR_LOSSLESS] = "Sans perte",
        [STR_WEB] = "Web",
        [STR_PHOTO] = "Photo",
        [STR_THUMB] = "Mini",
        [STR_SETTINGS] = "PARAMETRES",
        [STR_QUALITY] = "Qualite",
        [STR_COMPRESSION_EFFORT] = "Effort de compression",
        [STR_LOSSLESS_COMPRESSION] = "Compression sans perte",
        [STR_SHOW_ADVANCED] = "Options avancees",
        [STR_ALPHA_QUALITY] = "Qualite alpha",
        [STR_FILTER_STRENGTH] = "Force du filtre",
        [STR_CONVERT_TO_WEBP] = "Convertir en WebP",
        [STR_CONVERT_FILES] = "Convertir %d fichiers",
        [STR_FILE_SELECTED] = "1 fichier selectionne",
        [STR_FILES_SELECTED] = "%d fichiers selectionnes",
        [STR_ESTIMATE] = "Est: %s -> ~%s",
        [STR_DROP_IMAGES] = "Deposez vos images ici",
        [STR_SUPPORTED_FORMATS] = "PNG, JPEG, BMP, GIF",
        [STR_CONVERSION_COMPLETE] = "Conversion terminee !",
        [STR_FILES_CONVERTED] = "%d sur %d fichiers convertis",
        [STR_SAVED_SPACE] = "%d%% d'espace economise",
        [STR_FILES_FAILED] = "%d fichier(s) echoue(s)",
        [STR_OK] = "OK",
        [STR_CONVERTING] = "Conversion %d/%d: %s",
        [STR_DONE] = "Termine ! %d converti(s), %d echoue(s)",
        [STR_READY_TO_CONVERT] = "%d fichier(s) pret(s)",
        [STR_DROP_OR_ADD] = "Deposez des images ou cliquez 'Ajouter'",
        [STR_SELECT_IMAGES] = "Choisir des images",
    },
};

static const char* language_names[LANG_COUNT] = {
    [LANG_EN] = "EN",
    [LANG_FR] = "FR",
};

Language strings_get_language(void) {
    return current_language;
}

void strings_set_language(Language lang) {
    if (lang >= 0 && lang < LANG_COUNT) {
        current_language = lang;
    }
}

const char* str(StringID id) {
    if (id >= 0 && id < STR_COUNT) {
        return all_strings[current_language][id];
    }
    return "???";
}

const char* strings_get_language_name(Language lang) {
    if (lang >= 0 && lang < LANG_COUNT) {
        return language_names[lang];
    }
    return "??";
}
