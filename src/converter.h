/*
 * WebP Converter - Core conversion module
 * Handles image loading and WebP encoding
 */

#ifndef CONVERTER_H
#define CONVERTER_H

#include <stdbool.h>
#include <stdint.h>
#include <stddef.h>

/* Conversion parameters */
typedef struct {
    float quality;          /* 0-100, lossy quality */
    int method;             /* 0-6, compression effort (0=fast, 6=best) */
    bool lossless;          /* Use lossless encoding */
    float alpha_quality;    /* 0-100, alpha channel quality */
    int filter_strength;    /* 0-100, deblocking filter strength */
    int filter_sharpness;   /* 0-7, filter sharpness */
    int preprocessing;      /* 0-2, preprocessing filter */
} ConversionParams;

/* Image data structure */
typedef struct {
    unsigned char *data;    /* RGBA pixel data */
    int width;
    int height;
    int channels;           /* Number of channels (3=RGB, 4=RGBA) */
    char filepath[512];     /* Source file path */
    size_t file_size;       /* Original file size in bytes */
} ImageData;

/* Conversion result */
typedef struct {
    bool success;
    char error_message[256];
    size_t output_size;     /* Output file size in bytes */
    float compression_ratio; /* Original size / output size */
} ConversionResult;

/* Initialize default parameters */
void converter_init_params(ConversionParams *params);

/* Load an image from file (supports PNG, JPEG, BMP, GIF) */
bool converter_load_image(const char *filepath, ImageData *image);

/* Free image data */
void converter_free_image(ImageData *image);

/* Convert image to WebP and save to file */
ConversionResult converter_to_webp(const ImageData *image,
                                    const char *output_path,
                                    const ConversionParams *params);

/* Estimate output size (rough approximation) */
size_t converter_estimate_size(const ImageData *image, const ConversionParams *params);

/* Get supported file extensions */
const char* converter_get_supported_extensions(void);

/* Check if file extension is supported */
bool converter_is_supported(const char *filepath);

#endif /* CONVERTER_H */
