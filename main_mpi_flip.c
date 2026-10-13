
#include <stdio.h>
#include <stdlib.h>
#include <limits.h>
#include <mpi.h>
#include "image.h"

#define TAG_INFO 1
#define TAG_DATA 2
#define TAG_RESULT 3

int main(int argc, char *argv[])
{
    MPI_Init(&argc, &argv);

    int rank, size;
    MPI_Comm_rank(MPI_COMM_WORLD, &rank);
    MPI_Comm_size(MPI_COMM_WORLD, &size);

    if (size < 2) {
        if (rank == 0)
            printf("Avem nevoie de minimum 2 procese MPI!\n");

        MPI_Finalize();
        return 1;
    }

    if (rank == 0) {

        Image *img = load_image("poza.jpg");

        if (img == NULL) {
            printf("Eroare la citirea imaginii!\n");
            MPI_Abort(MPI_COMM_WORLD, 1);
        }

        int width = img->width;
        int height = img->height;
        int workers = size - 1;

        if (workers > height) {
            printf("Prea multi workeri pentru numarul de randuri!\n");
            MPI_Abort(MPI_COMM_WORLD, 1);
        }

        printf("Imagine: %d x %d pixeli\n", width, height);
        printf("Numar workeri: %d\n", workers);

        double start = MPI_Wtime();

        for (int worker = 1; worker <= workers; worker++) {

            int start_row = (worker - 1) * height / workers;
            int end_row = worker * height / workers;
            int rows = end_row - start_row;

            int info[2] = {width, rows};

            size_t bytes = (size_t)rows * width *
                           sizeof(Pixel);

            if (bytes > INT_MAX) {
                printf("Mesaj MPI prea mare!\n");
                MPI_Abort(MPI_COMM_WORLD, 1);
            }

            MPI_Send(info, 2, MPI_INT,
                     worker, TAG_INFO, MPI_COMM_WORLD);

            MPI_Send(img->pixels + (size_t)start_row * width,
                     (int)bytes, MPI_BYTE,
                     worker, TAG_DATA, MPI_COMM_WORLD);

            printf("Coordonator: trimit %d randuri catre worker %d\n",
                   rows, worker);
        }

        for (int worker = 1; worker <= workers; worker++) {

            int start_row = (worker - 1) * height / workers;
            int end_row = worker * height / workers;
            int rows = end_row - start_row;

            size_t bytes = (size_t)rows * width *
                           sizeof(Pixel);

            MPI_Recv(img->pixels + (size_t)start_row * width,
                     (int)bytes, MPI_BYTE,
                     worker, TAG_RESULT,
                     MPI_COMM_WORLD, MPI_STATUS_IGNORE);
        }

        double stop = MPI_Wtime();

        printf("\nTimp MPI Flip: %.6f secunde\n", stop - start);

        if (save_image("rez_mpi_flip.png", img))
            printf("Imagine MPI Flip salvata cu succes!\n");
        else
            printf("Eroare la salvarea imaginii MPI Flip!\n");

        free_image(img);

    } else {

        int info[2];

        MPI_Recv(info, 2, MPI_INT,
                 0, TAG_INFO, MPI_COMM_WORLD,
                 MPI_STATUS_IGNORE);

        int width = info[0];
        int rows = info[1];

        size_t bytes = (size_t)rows * width *
                       sizeof(Pixel);

        if (bytes > INT_MAX) {
            printf("Worker %d: mesaj MPI prea mare!\n", rank);
            MPI_Abort(MPI_COMM_WORLD, 1);
        }

        Pixel *pixels = malloc(bytes);

        if (pixels == NULL) {
            printf("Worker %d: eroare de memorie!\n", rank);
            MPI_Abort(MPI_COMM_WORLD, 1);
        }

        MPI_Recv(pixels, (int)bytes, MPI_BYTE,
                 0, TAG_DATA, MPI_COMM_WORLD,
                 MPI_STATUS_IGNORE);

        for (int y = 0; y < rows; y++) {

            for (int x = 0; x < width / 2; x++) {

                size_t left = (size_t)y * width + x;
                size_t right = (size_t)y * width +
                               (width - 1 - x);

                Pixel temp = pixels[left];
                pixels[left] = pixels[right];
                pixels[right] = temp;
            }
        }

        MPI_Send(pixels, (int)bytes, MPI_BYTE,
                 0, TAG_RESULT, MPI_COMM_WORLD);

        printf("Worker %d: flip finalizat!\n", rank);

        free(pixels);
    }

    MPI_Finalize();
    return 0;
}
