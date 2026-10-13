#include "image.h"
#define STB_IMAGE_IMPLEMENTATION
#include "stb_image.h"
#define STB_IMAGE_WRITE_IMPLEMENTATION
#include "stb_image_write.h"
#include <stdlib.h>

Image *load_image(const char *filename)
{
    int width, height, channels;

    unsigned char *data = stbi_load(filename, &width, &height, &channels, 3);

    if (data == NULL) {
        return NULL;
    }

    Image *img = malloc(sizeof(Image));

    if (img == NULL) {
        stbi_image_free(data);
        return NULL;
    }

    img->width = width;
    img->height = height;

    img->pixels = malloc((size_t)width * height * sizeof(Pixel));

    if (img->pixels == NULL) {
        free(img);
        stbi_image_free(data);
        return NULL;
    }

    for (int i = 0; i < width * height; i++) {
        img->pixels[i].r = data[i * 3];
        img->pixels[i].g = data[i * 3 + 1];
        img->pixels[i].b = data[i * 3 + 2];
    }

    stbi_image_free(data);

    return img;
}

int save_image(const char *filename, const Image *img)
{
    if (img == NULL || img->pixels == NULL) {
        return 0;
    }

    int result = stbi_write_png(
        filename,
        img->width,
        img->height,
        3,
        img->pixels,
        img->width * sizeof(Pixel)
    );

    return result;
}

void free_image(Image *img)
{
    if (img == NULL) {
        return;
    }

    free(img->pixels);
    free(img);
}
