/*
 * WebP Converter - Core conversion module implementation
 */

#include "converter.h"
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <ctype.h>
#include <sys/stat.h>

#define STB_IMAGE_IMPLEMENTATION
#include "stb_image.h"

#include <webp/encode.h>

void converter_init_params(ConversionParams *params) {
    params->quality = 75.0f;
    params->method = 4;
    params->lossless = false;
    params->alpha_quality = 100.0f;
    params->filter_strength = 60;
    params->filter_sharpness = 0;
    params->preprocessing = 0;
}

static size_t get_file_size(const char *filepath) {
    struct stat st;
    if (stat(filepath, &st) == 0) {
        return (size_t)st.st_size;
    }
    return 0;
}

static const char* get_extension(const char *filepath) {
    const char *dot = strrchr(filepath, '.');
    if (dot && dot != filepath) {
        return dot + 1;
    }
    return "";
}

bool converter_is_supported(const char *filepath) {
    const char *ext = get_extension(filepath);
    char lower_ext[16] = {0};

    for (int i = 0; ext[i] && i < 15; i++) {
        lower_ext[i] = tolower((unsigned char)ext[i]);
    }

    return strcmp(lower_ext, "png") == 0 ||
           strcmp(lower_ext, "jpg") == 0 ||
           strcmp(lower_ext, "jpeg") == 0 ||
           strcmp(lower_ext, "bmp") == 0 ||
           strcmp(lower_ext, "gif") == 0;
}

const char* converter_get_supported_extensions(void) {
    return "*.png;*.jpg;*.jpeg;*.bmp;*.gif";
}

bool converter_load_image(const char *filepath, ImageData *image) {
    if (!filepath || !image) return false;

    memset(image, 0, sizeof(ImageData));

    /* Check file exists and get size */
    image->file_size = get_file_size(filepath);
    if (image->file_size == 0) {
        return false;
    }

    /* Load image with stb_image (force RGBA) */
    image->data = stbi_load(filepath, &image->width, &image->height,
                            &image->channels, 4);

    if (!image->data) {
        return false;
    }

    /* Store filepath */
    strncpy(image->filepath, filepath, sizeof(image->filepath) - 1);
    image->channels = 4; /* We forced RGBA */

    return true;
}

void converter_free_image(ImageData *image) {
    if (image && image->data) {
        stbi_image_free(image->data);
        image->data = NULL;
    }
}

ConversionResult converter_to_webp(const ImageData *image,
                                    const char *output_path,
                                    const ConversionParams *params) {
    ConversionResult result = {0};

    if (!image || !image->data || !output_path || !params) {
        result.success = false;
        snprintf(result.error_message, sizeof(result.error_message),
                "Invalid parameters");
        return result;
    }

    WebPConfig config;
    WebPPicture picture;
    WebPMemoryWriter writer;

    /* Initialize WebP config */
    if (!WebPConfigInit(&config)) {
        result.success = false;
        snprintf(result.error_message, sizeof(result.error_message),
                "Failed to initialize WebP config");
        return result;
    }

    /* Set encoding parameters */
    config.quality = params->quality;
    config.method = params->method;
    config.lossless = params->lossless ? 1 : 0;
    config.alpha_quality = (int)params->alpha_quality;
    config.filter_strength = params->filter_strength;
    config.filter_sharpness = params->filter_sharpness;
    config.preprocessing = params->preprocessing;

    /* For lossless mode, quality controls compression/speed tradeoff */
    if (params->lossless) {
        config.quality = 100.0f;
    }

    /* Validate config */
    if (!WebPValidateConfig(&config)) {
        result.success = false;
        snprintf(result.error_message, sizeof(result.error_message),
                "Invalid WebP configuration");
        return result;
    }

    /* Initialize picture */
    if (!WebPPictureInit(&picture)) {
        result.success = false;
        snprintf(result.error_message, sizeof(result.error_message),
                "Failed to initialize WebP picture");
        return result;
    }

    picture.width = image->width;
    picture.height = image->height;
    picture.use_argb = 1;

    /* Allocate picture buffer */
    if (!WebPPictureAlloc(&picture)) {
        result.success = false;
        snprintf(result.error_message, sizeof(result.error_message),
                "Failed to allocate WebP picture buffer");
        return result;
    }

    /* Import RGBA data */
    if (!WebPPictureImportRGBA(&picture, image->data, image->width * 4)) {
        WebPPictureFree(&picture);
        result.success = false;
        snprintf(result.error_message, sizeof(result.error_message),
                "Failed to import image data");
        return result;
    }

    /* Setup memory writer */
    WebPMemoryWriterInit(&writer);
    picture.writer = WebPMemoryWrite;
    picture.custom_ptr = &writer;

    /* Encode */
    int encode_ok = WebPEncode(&config, &picture);
    WebPPictureFree(&picture);

    if (!encode_ok) {
        WebPMemoryWriterClear(&writer);
        result.success = false;
        snprintf(result.error_message, sizeof(result.error_message),
                "WebP encoding failed (error code: %d)", picture.error_code);
        return result;
    }

    /* Write to file */
    FILE *fp = fopen(output_path, "wb");
    if (!fp) {
        WebPMemoryWriterClear(&writer);
        result.success = false;
        snprintf(result.error_message, sizeof(result.error_message),
                "Failed to open output file: %s", output_path);
        return result;
    }

    size_t written = fwrite(writer.mem, 1, writer.size, fp);
    fclose(fp);

    if (written != writer.size) {
        WebPMemoryWriterClear(&writer);
        result.success = false;
        snprintf(result.error_message, sizeof(result.error_message),
                "Failed to write output file");
        return result;
    }

    /* Success! */
    result.success = true;
    result.output_size = writer.size;
    result.compression_ratio = (float)image->file_size / (float)writer.size;

    WebPMemoryWriterClear(&writer);
    return result;
}

size_t converter_estimate_size(const ImageData *image, const ConversionParams *params) {
    if (!image || !params) return 0;

    size_t raw_size = (size_t)image->width * image->height * 4;

    if (params->lossless) {
        /* Lossless typically achieves 40-70% of raw size */
        /* Higher method = better compression */
        float method_factor = 0.7f - (params->method / 6.0f) * 0.3f;
        return (size_t)(raw_size * method_factor);
    }

    /* Lossy estimation based on quality and method */
    /* Quality 100 ~ 8-12% of raw, quality 0 ~ 1-2% of raw */
    /* Higher method = slightly smaller files */
    float quality_ratio = 0.01f + (params->quality / 100.0f) * 0.10f;
    float method_factor = 1.0f - (params->method / 6.0f) * 0.2f;

    return (size_t)(raw_size * quality_ratio * method_factor);
}
