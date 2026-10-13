#include <stdio.h>
#include <time.h>
#include "image.h"
#include "operations.h"

int main(void)
{
    int optiune;
    int blur_size = 21;

    printf("\n===== PROCESARE IMAGINI =====\n");
    printf("1. Grayscale\n");
    printf("2. Blur\n");
    printf("3. Horizontal Flip\n");
    printf("4. Toate cele 3 operatii\n");
    printf("============================\n");
    printf("Alege o optiune (1-4): ");

    if (scanf("%d", &optiune) != 1 ||
        optiune < 1 || optiune > 4) {
        printf("Optiune invalida!\n");
        return 1;
    }

    if (optiune == 2 || optiune == 4) {

        int intensitate;

        printf("\n===== INTENSITATE BLUR =====\n");
        printf("1. Blur usor (3x3)\n");
        printf("2. Blur mediu (9x9)\n");
        printf("3. Blur puternic (21x21)\n");
        printf("4. Personalizat\n");
        printf("===========================\n");
        printf("Alege intensitatea (1-4): ");

        if (scanf("%d", &intensitate) != 1) {
            printf("Intensitate invalida!\n");
            return 1;
        }

        switch (intensitate) {
            case 1:
                blur_size = 3;
                break;

            case 2:
                blur_size = 9;
                break;

            case 3:
                blur_size = 21;
                break;

            case 4:
                printf("Introdu dimensiunea blur-ului (numar impar 1-99): ");

                if (scanf("%d", &blur_size) != 1 ||
                    blur_size < 1 ||
                    blur_size > 99 ||
                    blur_size % 2 == 0) {

                    printf("Dimensiune invalida!\n");
                    return 1;
                }
                break;

            default:
                printf("Optiune invalida!\n");
                return 1;
        }

        printf("\nBlur selectat: %dx%d\n",
               blur_size, blur_size);
    }

    for (int operatie = 1; operatie <= 3; operatie++) {

        if (optiune != 4 && optiune != operatie) {
            continue;
        }

        Image *img = load_image("poza.jpg");

        if (img == NULL) {
            printf("Eroare la citirea imaginii!\n");
            return 1;
        }

        const char *fisier_rezultat = NULL;
        const char *nume_operatie = NULL;

        clock_t start = clock();

        switch (operatie) {

            case 1:
                grayscale(img);
                nume_operatie = "Grayscale";
                fisier_rezultat = "rez1_grayscale.png";
                break;

            case 2:
                blur(img, blur_size);
                nume_operatie = "Blur";
                fisier_rezultat = "rez2_blur.png";
                break;

            case 3:
                horizontal_flip(img);
                nume_operatie = "Horizontal Flip";
                fisier_rezultat = "rez3_flip.png";
                break;
        }

        clock_t stop = clock();

        double timp = (double)(stop - start) / CLOCKS_PER_SEC;

        if (save_image(fisier_rezultat, img)) {

            printf("\n%s realizat cu succes!\n", nume_operatie);
            printf("Fisier: %s\n", fisier_rezultat);
            printf("Timp procesare: %.6f secunde\n", timp);

        } else {

            printf("Eroare la salvarea imaginii!\n");
            free_image(img);
            return 1;
        }

        free_image(img);
    }

    printf("\nProcesare finalizata!\n");

    return 0;
}

