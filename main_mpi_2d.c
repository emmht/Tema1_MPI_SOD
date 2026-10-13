
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

        int grid_rows = 1;
        int grid_cols = workers;

        for (int r = 1; r <= workers / r; r++) {
            if (workers % r == 0) {
                grid_rows = r;
                grid_cols = workers / r;
            }
        }

        if (grid_rows > height || grid_cols > width) {
            printf("Prea multi workeri pentru imagine!\n");
            MPI_Abort(MPI_COMM_WORLD, 1);
        }

        printf("Imagine: %d x %d pixeli\n", width, height);
        printf("Numar workeri: %d\n", workers);
        printf("Grila 2D: %d x %d\n", grid_rows, grid_cols);

        double start = MPI_Wtime();

        for (int worker = 1; worker <= workers; worker++) {

            int index = worker - 1;
            int block_row = index / grid_cols;
            int block_col = index % grid_cols;

            int y0 = block_row * height / grid_rows;
            int y1 = (block_row + 1) * height / grid_rows;
            int x0 = block_col * width / grid_cols;
            int x1 = (block_col + 1) * width / grid_cols;

            int block_width = x1 - x0;
            int block_height = y1 - y0;

            size_t count = (size_t)block_width * block_height;
            size_t bytes = count * sizeof(Pixel);

            if (bytes > INT_MAX) {
                printf("Bloc MPI prea mare!\n");
                MPI_Abort(MPI_COMM_WORLD, 1);
            }

            Pixel *block = malloc(bytes);

            if (block == NULL) {
                printf("Eroare de memorie!\n");
                MPI_Abort(MPI_COMM_WORLD, 1);
            }

            for (int y = 0; y < block_height; y++) {
                for (int x = 0; x < block_width; x++) {

                    block[(size_t)y * block_width + x] =
                        img->pixels[(size_t)(y0 + y) * width + x0 + x];
                }
            }

            int info[2] = {block_width, block_height};

            MPI_Send(info, 2, MPI_INT,
                     worker, TAG_INFO, MPI_COMM_WORLD);

            MPI_Send(block, (int)bytes, MPI_BYTE,
                     worker, TAG_DATA, MPI_COMM_WORLD);

            printf("Worker %d: bloc %d x %d, pozitie (%d, %d)\n",
                   worker, block_width, block_height, x0, y0);

            free(block);
        }

        for (int worker = 1; worker <= workers; worker++) {

            int index = worker - 1;
            int block_row = index / grid_cols;
            int block_col = index % grid_cols;

            int y0 = block_row * height / grid_rows;
            int y1 = (block_row + 1) * height / grid_rows;
            int x0 = block_col * width / grid_cols;
            int x1 = (block_col + 1) * width / grid_cols;

            int block_width = x1 - x0;
            int block_height = y1 - y0;

            size_t count = (size_t)block_width * block_height;
            size_t bytes = count * sizeof(Pixel);

            Pixel *block = malloc(bytes);

            if (block == NULL) {
                printf("Eroare de memorie!\n");
                MPI_Abort(MPI_COMM_WORLD, 1);
            }

            MPI_Recv(block, (int)bytes, MPI_BYTE,
                     worker, TAG_RESULT,
                     MPI_COMM_WORLD, MPI_STATUS_IGNORE);

            for (int y = 0; y < block_height; y++) {
                for (int x = 0; x < block_width; x++) {

                    img->pixels[(size_t)(y0 + y) * width + x0 + x] =
                        block[(size_t)y * block_width + x];
                }
            }

            free(block);
        }

        double stop = MPI_Wtime();

        printf("\nTimp MPI Grayscale 2D: %.6f secunde\n",
               stop - start);

        if (save_image("rez_mpi_grayscale_2d.png", img))
            printf("Imagine MPI 2D salvata cu succes!\n");
        else
            printf("Eroare la salvarea imaginii!\n");

        free_image(img);

    } else {

        int info[2];

        MPI_Recv(info, 2, MPI_INT,
                 0, TAG_INFO, MPI_COMM_WORLD,
                 MPI_STATUS_IGNORE);

        int block_width = info[0];
        int block_height = info[1];

        size_t count = (size_t)block_width * block_height;
        size_t bytes = count * sizeof(Pixel);

        Pixel *block = malloc(bytes);

        if (block == NULL) {
            printf("Worker %d: eroare de memorie!\n", rank);
            MPI_Abort(MPI_COMM_WORLD, 1);
        }

        MPI_Recv(block, (int)bytes, MPI_BYTE,
                 0, TAG_DATA, MPI_COMM_WORLD,
                 MPI_STATUS_IGNORE);

        for (size_t i = 0; i < count; i++) {

            unsigned char gray = (unsigned char)(
                0.299 * block[i].r +
                0.587 * block[i].g +
                0.114 * block[i].b
            );

            block[i].r = gray;
            block[i].g = gray;
            block[i].b = gray;
        }

        MPI_Send(block, (int)bytes, MPI_BYTE,
                 0, TAG_RESULT, MPI_COMM_WORLD);

        printf("Worker %d: bloc procesat!\n", rank);

        free(block);
    }

    MPI_Finalize();
    return 0;
}
