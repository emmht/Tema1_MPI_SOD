#ifndef OPERATIONS_H
#define OPERATIONS_H

#include "image.h"

void grayscale(Image *img);
void blur(Image *img, int size);
void horizontal_flip(Image *img);

#endif
