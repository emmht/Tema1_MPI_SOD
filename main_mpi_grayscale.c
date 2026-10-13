#include <stdio.h>
#include <stdlib.h>
#include <mpi.h>
#include "image.h"

#define TAG_DATA 1
#define TAG_RESULT 2

int main(int argc, char *argv[])
{
    MPI_Init(&argc, &argv);

    int rank, size;
    MPI_Comm_rank(MPI_COMM_WORLD, &rank);
    MPI_Comm_size(MPI_COMM_WORLD, &size);

    if (size < 2) {
        if (rank == 0) {
            printf("Avem nevoie de minimum 2 procese MPI!\n");
        }
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

        printf("Imagine: %d x %d pixeli\n", width, height);
        printf("Numar workeri: %d\n", workers);

        double start = MPI_Wtime();

        for (int worker = 1; worker <= workers; worker++) {

            int start_row = (worker - 1) * height / workers;
            int end_row = worker * height / workers;
            int rows = end_row - start_row;
            int count = rows * width;

            MPI_Send(&count, 1, MPI_INT,
                     worker, TAG_DATA, MPI_COMM_WORLD);

            MPI_Send(img->pixels + start_row * width,
                     count * (int)sizeof(Pixel),
                     MPI_BYTE, worker, TAG_DATA, MPI_COMM_WORLD);

            printf("Coordonator: trimit %d randuri catre worker %d\n",
                   rows, worker);
        }

        for (int worker = 1; worker <= workers; worker++) {

            int start_row = (worker - 1) * height / workers;
            int end_row = worker * height / workers;
            int count = (end_row - start_row) * width;

            MPI_Recv(img->pixels + start_row * width,
                     count * (int)sizeof(Pixel),
                     MPI_BYTE, worker, TAG_RESULT,
                     MPI_COMM_WORLD, MPI_STATUS_IGNORE);
        }

        double stop = MPI_Wtime();

        printf("\nTimp MPI: %.6f secunde\n", stop - start);

        if (save_image("rez_mpi_grayscale.png", img)) {
            printf("Imagine MPI salvata cu succes!\n");
        } else {
            printf("Eroare la salvarea imaginii MPI!\n");
        }

        free_image(img);

    } else {


        int count;

        MPI_Recv(&count, 1, MPI_INT,
                 0, TAG_DATA, MPI_COMM_WORLD,
                 MPI_STATUS_IGNORE);

        Pixel *pixels = malloc((size_t)count * sizeof(Pixel));

        if (pixels == NULL && count > 0) {
            printf("Worker %d: eroare de memorie!\n", rank);
            MPI_Abort(MPI_COMM_WORLD, 1);
        }

        MPI_Recv(pixels, count * (int)sizeof(Pixel),
                 MPI_BYTE, 0, TAG_DATA,
                 MPI_COMM_WORLD, MPI_STATUS_IGNORE);

        for (int i = 0; i < count; i++) {

            unsigned char gray = (unsigned char)(
                0.299 * pixels[i].r +
                0.587 * pixels[i].g +
                0.114 * pixels[i].b
            );

            pixels[i].r = gray;
            pixels[i].g = gray;
            pixels[i].b = gray;
        }

        MPI_Send(pixels, count * (int)sizeof(Pixel),
                 MPI_BYTE, 0, TAG_RESULT, MPI_COMM_WORLD);

        printf("Worker %d: am procesat %d pixeli!\n",
               rank, count);

        free(pixels);
    }

    MPI_Finalize();
    return 0;
}

