#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <limits.h>
#include <mpi.h>
#include "image.h"

#define TAG_INFO 1
#define TAG_DATA 2
#define TAG_RESULT 3

void process_block(Pixel *input, Pixel *output,
                   int width, int local_rows,
                   int own_rows, int offset,
                   int operation, int radius)
{
    if (operation == 1) {

        for (int y = 0; y < own_rows; y++) {
            for (int x = 0; x < width; x++) {

                Pixel p = input[(size_t)(y + offset) * width + x];

                unsigned char gray = (unsigned char)(
                    0.299 * p.r +
                    0.587 * p.g +
                    0.114 * p.b
                );

                Pixel *q = &output[(size_t)y * width + x];

                q->r = gray;
                q->g = gray;
                q->b = gray;
            }
        }

    } else if (operation == 2) {

        for (int y = 0; y < own_rows; y++) {
            for (int x = 0; x < width; x++) {

                unsigned long long sum_r = 0;
                unsigned long long sum_g = 0;
                unsigned long long sum_b = 0;
                unsigned long long count = 0;

                int center_y = y + offset;

                for (int dy = -radius; dy <= radius; dy++) {

                    int ny = center_y + dy;

                    if (ny < 0 || ny >= local_rows)
                        continue;

                    for (int dx = -radius; dx <= radius; dx++) {

                        int nx = x + dx;

                        if (nx < 0 || nx >= width)
                            continue;

                        Pixel p = input[(size_t)ny * width + nx];

                        sum_r += p.r;
                        sum_g += p.g;
                        sum_b += p.b;
                        count++;
                    }
                }

                Pixel *q = &output[(size_t)y * width + x];

                q->r = (unsigned char)(sum_r / count);
                q->g = (unsigned char)(sum_g / count);
                q->b = (unsigned char)(sum_b / count);
            }
        }

    } else if (operation == 3) {

        for (int y = 0; y < own_rows; y++) {
            for (int x = 0; x < width; x++) {

                output[(size_t)y * width + x] =
                    input[(size_t)(y + offset) * width +
                          (width - 1 - x)];
            }
        }
    }
}

void execute_operation(Image *img, int operation,
                       int radius, int rank, int size)
{
    int width = 0;
    int height = 0;

    if (rank == 0) {
        width = img->width;
        height = img->height;
    }

    int dimensions[2] = {width, height};

    MPI_Bcast(dimensions, 2, MPI_INT, 0, MPI_COMM_WORLD);

    width = dimensions[0];
    height = dimensions[1];

    int workers = size - 1;

    if (rank == 0) {

        for (int worker = 1; worker <= workers; worker++) {

            int start_row = (worker - 1) * height / workers;
            int end_row = worker * height / workers;

            int own_rows = end_row - start_row;

            int halo_start = start_row;
            int halo_end = end_row;

            if (operation == 2) {

                halo_start -= radius;
                halo_end += radius;

                if (halo_start < 0)
                    halo_start = 0;

                if (halo_end > height)
                    halo_end = height;
            }

            int local_rows = halo_end - halo_start;
            int offset = start_row - halo_start;

            int info[6] = {
                width,
                local_rows,
                own_rows,
                offset,
                operation,
                radius
            };

            size_t bytes = (size_t)local_rows *
                           width * sizeof(Pixel);

            if (bytes > INT_MAX) {
                printf("Bloc MPI prea mare!\n");
                MPI_Abort(MPI_COMM_WORLD, 1);
            }

            MPI_Send(info, 6, MPI_INT,
                     worker, TAG_INFO, MPI_COMM_WORLD);

            MPI_Send(img->pixels + (size_t)halo_start * width,
                     (int)bytes, MPI_BYTE,
                     worker, TAG_DATA, MPI_COMM_WORLD);
        }

        for (int worker = 1; worker <= workers; worker++) {

            int start_row = (worker - 1) * height / workers;
            int end_row = worker * height / workers;
            int own_rows = end_row - start_row;

            size_t bytes = (size_t)own_rows *
                           width * sizeof(Pixel);

            MPI_Recv(img->pixels + (size_t)start_row * width,
                     (int)bytes, MPI_BYTE,
                     worker, TAG_RESULT,
                     MPI_COMM_WORLD, MPI_STATUS_IGNORE);
        }

    } else {

        int info[6];

        MPI_Recv(info, 6, MPI_INT,
                 0, TAG_INFO, MPI_COMM_WORLD,
                 MPI_STATUS_IGNORE);

        int local_width = info[0];
        int local_rows = info[1];
        int own_rows = info[2];
        int offset = info[3];
        int local_operation = info[4];
        int local_radius = info[5];

        size_t input_bytes = (size_t)local_rows *
                             local_width * sizeof(Pixel);

        size_t output_bytes = (size_t)own_rows *
                              local_width * sizeof(Pixel);

        Pixel *input = malloc(input_bytes);
        Pixel *output = malloc(output_bytes);

        if (input == NULL || output == NULL) {
            printf("Worker %d: eroare de memorie!\n", rank);
            MPI_Abort(MPI_COMM_WORLD, 1);
        }

        MPI_Recv(input, (int)input_bytes, MPI_BYTE,
                 0, TAG_DATA, MPI_COMM_WORLD,
                 MPI_STATUS_IGNORE);

        process_block(input, output,
                      local_width, local_rows,
                      own_rows, offset,
                      local_operation, local_radius);

        MPI_Send(output, (int)output_bytes, MPI_BYTE,
                 0, TAG_RESULT, MPI_COMM_WORLD);

        free(input);
        free(output);
    }
}

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

    Image *img = NULL;

    if (rank == 0) {

        img = load_image("poza.jpg");

        if (img == NULL) {
            printf("Eroare la citirea imaginii!\n");
            MPI_Abort(MPI_COMM_WORLD, 1);
        }

        if (size - 1 > img->height) {
            printf("Prea multi workeri pentru imagine!\n");
            MPI_Abort(MPI_COMM_WORLD, 1);
        }

        printf("Imagine incarcata: %d x %d pixeli\n",
               img->width, img->height);
    }

    int number_operations = 0;

    if (rank == 0) {

        printf("\nCate operatii vrei sa aplici? ");
        fflush(stdout);

        if (scanf("%d", &number_operations) != 1 ||
            number_operations < 1 ||
            number_operations > 100) {

            printf("Numar invalid de operatii!\n");
            MPI_Abort(MPI_COMM_WORLD, 1);
        }
    }

    MPI_Bcast(&number_operations, 1, MPI_INT,
              0, MPI_COMM_WORLD);

    double total_time = 0.0;

    for (int step = 0; step < number_operations; step++) {

        int operation = 0;
        int blur_size = 3;

        if (rank == 0) {

            printf("\nOperatia %d din %d\n",
                   step + 1, number_operations);

            printf("1. Grayscale\n");
            printf("2. Blur\n");
            printf("3. Horizontal Flip\n");

            printf("Alege operatia: ");
            fflush(stdout);

            if (scanf("%d", &operation) != 1 ||
                operation < 1 || operation > 3) {

                printf("Operatie invalida!\n");
                MPI_Abort(MPI_COMM_WORLD, 1);
            }

            if (operation == 2) {

                printf("Dimensiunea blur (impara, 1-99): ");
                fflush(stdout);

                if (scanf("%d", &blur_size) != 1 ||
                    blur_size < 1 ||
                    blur_size > 99 ||
                    blur_size % 2 == 0) {

                    printf("Dimensiune blur invalida!\n");
                    MPI_Abort(MPI_COMM_WORLD, 1);
                }
            }
        }

        int settings[2] = {operation, blur_size};

        MPI_Bcast(settings, 2, MPI_INT,
                  0, MPI_COMM_WORLD);

        operation = settings[0];
        blur_size = settings[1];

        int radius = blur_size / 2;

        MPI_Barrier(MPI_COMM_WORLD);

        double start = MPI_Wtime();

        execute_operation(img, operation, radius,
                          rank, size);

        MPI_Barrier(MPI_COMM_WORLD);

        double stop = MPI_Wtime();

        if (rank == 0) {

            double elapsed = stop - start;
            total_time += elapsed;

            if (operation == 1)
                printf("Grayscale finalizat: %.6f secunde\n",
                       elapsed);

            if (operation == 2)
                printf("Blur %dx%d finalizat: %.6f secunde\n",
                       blur_size, blur_size, elapsed);

            if (operation == 3)
                printf("Horizontal Flip finalizat: %.6f secunde\n",
                       elapsed);
        }
    }

    if (rank == 0) {

        printf("\nTimp total procesare MPI: %.6f secunde\n",
               total_time);

        if (save_image("rez_mpi_pipeline.png", img))
            printf("Imaginea finala a fost salvata!\n");
        else
            printf("Eroare la salvarea imaginii!\n");

        free_image(img);
    }

    MPI_Finalize();
    return 0;
}
