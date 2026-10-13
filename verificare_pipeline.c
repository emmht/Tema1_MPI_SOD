#include <stdio.h>
#include "image.h"
#include "operations.h"

int main(void)
{
    Image *img = load_image("poza.jpg");

    if (img == NULL) {
        printf("Eroare la incarcarea imaginii!\n");
        return 1;
    }

    printf("Imagine incarcata: %d x %d pixeli\n",
           img->width, img->height);

    printf("Aplic Grayscale...\n");
    grayscale(img);

    printf("Aplic Blur 3x3...\n");
    blur(img, 3);

    printf("Aplic Horizontal Flip...\n");
    horizontal_flip(img);

    if (!save_image("rez_pipeline_secvential.png", img)) {
        printf("Eroare la salvarea imaginii!\n");
        free_image(img);
        return 1;
    }

    printf("Pipeline secvential finalizat!\n");
    printf("Imagine salvata: rez_pipeline_secvential.png\n");

    free_image(img);

    return 0;
}
