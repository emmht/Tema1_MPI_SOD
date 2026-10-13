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
        int blur_size;

        if (workers > height) {
            printf("Prea multi workeri pentru numarul de randuri!\n");
            MPI_Abort(MPI_COMM_WORLD, 1);
        }

        printf("Imagine: %d x %d pixeli\n", width, height);
        printf("Numar workeri: %d\n", workers);
        printf("\nDimensiune blur (numar impar 1-99): ");

        if (scanf("%d", &blur_size) != 1 ||
            blur_size < 1 ||
            blur_size > 99 ||
            blur_size % 2 == 0) {
            printf("Dimensiune blur invalida!\n");
            MPI_Abort(MPI_COMM_WORLD, 1);
        }

        int radius = blur_size / 2;

        double start = MPI_Wtime();

        for (int worker = 1; worker <= workers; worker++) {

            int start_row = (worker - 1) * height / workers;
            int end_row = worker * height / workers;

            int halo_start = start_row - radius;
            int halo_end = end_row + radius;

            if (halo_start < 0)
                halo_start = 0;

            if (halo_end > height)
                halo_end = height;

            int local_rows = halo_end - halo_start;
            int own_rows = end_row - start_row;
            int offset = start_row - halo_start;

            int info[5] = {
                width, local_rows, own_rows, offset, radius
            };

            size_t bytes = (size_t)local_rows * width *
                           sizeof(Pixel);

            if (bytes > INT_MAX) {
                printf("Mesaj MPI prea mare!\n");
                MPI_Abort(MPI_COMM_WORLD, 1);
            }

            MPI_Send(info, 5, MPI_INT,
                     worker, TAG_INFO, MPI_COMM_WORLD);

            MPI_Send(img->pixels + (size_t)halo_start * width,
                     (int)bytes, MPI_BYTE,
                     worker, TAG_DATA, MPI_COMM_WORLD);

            printf("Worker %d: %d randuri proprii, %d randuri halo\n",
                   worker, own_rows, local_rows - own_rows);
        }

        for (int worker = 1; worker <= workers; worker++) {

            int start_row = (worker - 1) * height / workers;
            int end_row = worker * height / workers;
            int own_rows = end_row - start_row;

            size_t bytes = (size_t)own_rows * width *
                           sizeof(Pixel);

            MPI_Recv(img->pixels + (size_t)start_row * width,
                     (int)bytes, MPI_BYTE,
                     worker, TAG_RESULT,
                     MPI_COMM_WORLD, MPI_STATUS_IGNORE);
        }

        double stop = MPI_Wtime();

        printf("\nTimp MPI Blur: %.6f secunde\n", stop - start);

        char filename[100];
        snprintf(filename, sizeof(filename),
                 "rez_mpi_blur_%d.png", blur_size);

        if (save_image(filename, img))
            printf("Imagine salvata: %s\n", filename);
        else
            printf("Eroare la salvarea imaginii!\n");

        free_image(img);

    } else {

        int info[5];

        MPI_Recv(info, 5, MPI_INT,
                 0, TAG_INFO, MPI_COMM_WORLD,
                 MPI_STATUS_IGNORE);

        int width = info[0];
        int local_rows = info[1];
        int own_rows = info[2];
        int offset = info[3];
        int radius = info[4];

        size_t input_bytes = (size_t)local_rows * width *
                             sizeof(Pixel);

        size_t output_bytes = (size_t)own_rows * width *
                              sizeof(Pixel);

        if (input_bytes > INT_MAX || output_bytes > INT_MAX) {
            printf("Worker %d: mesaj MPI prea mare!\n", rank);
            MPI_Abort(MPI_COMM_WORLD, 1);
        }

        Pixel *input = malloc(input_bytes);
        Pixel *output = malloc(output_bytes);

        if (input == NULL || output == NULL) {
            printf("Worker %d: eroare de memorie!\n", rank);
            MPI_Abort(MPI_COMM_WORLD, 1);
        }

        MPI_Recv(input, (int)input_bytes, MPI_BYTE,
                 0, TAG_DATA, MPI_COMM_WORLD,
                 MPI_STATUS_IGNORE);

        for (int y = 0; y < own_rows; y++) {

            int local_y = y + offset;

            for (int x = 0; x < width; x++) {

                unsigned long long sum_r = 0;
                unsigned long long sum_g = 0;
                unsigned long long sum_b = 0;
                unsigned long long count = 0;

                for (int dy = -radius; dy <= radius; dy++) {
                    for (int dx = -radius; dx <= radius; dx++) {

                        int ny = local_y + dy;
                        int nx = x + dx;

                        if (ny >= 0 && ny < local_rows &&
                            nx >= 0 && nx < width) {

                            Pixel p = input[(size_t)ny * width + nx];

                            sum_r += p.r;
                            sum_g += p.g;
                            sum_b += p.b;
                            count++;
                        }
                    }
                }

                size_t index = (size_t)y * width + x;

                output[index].r = sum_r / count;
                output[index].g = sum_g / count;
                output[index].b = sum_b / count;
            }
        }

        MPI_Send(output, (int)output_bytes, MPI_BYTE,
                 0, TAG_RESULT, MPI_COMM_WORLD);

        printf("Worker %d: blur finalizat!\n", rank);

        free(input);
        free(output);
    }

    MPI_Finalize();
    return 0;
}
