#ifndef IMAGE_H
#define IMAGE_H
#include <stdint.h>

typedef struct {
    uint8_t r;
    uint8_t g;
    uint8_t b;
} Pixel;

typedef struct {
    int width;
    int height;
    Pixel *pixels;
} Image;

Image *load_image(const char *filename);

int save_image(const char *filename, const Image *img);

void free_image(Image *img);

#endif
