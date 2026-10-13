#include "operations.h"
#include <stdlib.h>
#include <string.h>

void grayscale(Image *img)
{
    for (int i = 0; i < img->width * img->height; i++) {

        Pixel *p = &img->pixels[i];

        unsigned char gray = (unsigned char)(0.299 * p->r + 0.587 * p->g + 0.114 * p->b);

        p->r = gray;
        p->g = gray;
        p->b = gray;
    }
}

void blur(Image *img, int size)
{
    if (img == NULL || img->pixels == NULL ||
        size < 1 || size % 2 == 0) {
        return;
    }

    int width = img->width;
    int height = img->height;
    int radius = size / 2;

    Pixel *original = malloc((size_t)width * height * sizeof(Pixel));

    if (original == NULL) {
        return;
    }

    memcpy(original, img->pixels,
           (size_t)width * height * sizeof(Pixel));

    for (int y = 0; y < height; y++) {
        for (int x = 0; x < width; x++) {

            unsigned long long sum_r = 0;
            unsigned long long sum_g = 0;
            unsigned long long sum_b = 0;
            unsigned long long count = 0;

            for (int dy = -radius; dy <= radius; dy++) {
                for (int dx = -radius; dx <= radius; dx++) {

                    int ny = y + dy;
                    int nx = x + dx;

                    if (ny >= 0 && ny < height &&
                        nx >= 0 && nx < width) {

                        int index = ny * width + nx;

                        sum_r += original[index].r;
                        sum_g += original[index].g;
                        sum_b += original[index].b;
                        count++;
                    }
                }
            }

            int index = y * width + x;

            img->pixels[index].r = sum_r / count;
            img->pixels[index].g = sum_g / count;
            img->pixels[index].b = sum_b / count;
        }
    }

    free(original);
}

void horizontal_flip(Image *img)
{
    int width = img->width;
    int height = img->height;

    for (int y = 0; y < height; y++) {
        for (int x = 0; x < width / 2; x++) {

            int left = y * width + x;
            int right = y * width + (width - 1 - x);

            Pixel temp = img->pixels[left];

            img->pixels[left] = img->pixels[right];
            img->pixels[right] = temp;
        }
    }
}
