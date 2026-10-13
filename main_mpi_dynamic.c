
#include <stdio.h>
#include <stdlib.h>
#include <limits.h>
#include <mpi.h>
#include "image.h"

#define TAG_TASK 1
#define TAG_DATA 2
#define TAG_RESULT 3
#define TAG_STOP 4

#define CHUNK_ROWS 100

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

        int total_chunks = (height + CHUNK_ROWS - 1) / CHUNK_ROWS;
        int next_chunk = 0;
        int completed = 0;

        size_t max_bytes = (size_t)CHUNK_ROWS * width *
                           sizeof(Pixel);

        if (max_bytes > INT_MAX) {
            printf("Dimensiunea blocului este prea mare!\n");
            MPI_Abort(MPI_COMM_WORLD, 1);
        }

        Pixel *buffer = malloc(max_bytes);

        if (buffer == NULL) {
            printf("Eroare de memorie!\n");
            MPI_Abort(MPI_COMM_WORLD, 1);
        }

        printf("Imagine: %d x %d pixeli\n", width, height);
        printf("Numar workeri: %d\n", workers);
        printf("Randuri per sarcina: %d\n", CHUNK_ROWS);
        printf("Numar total sarcini: %d\n", total_chunks);

        double start = MPI_Wtime();

        for (int worker = 1; worker <= workers; worker++) {

            if (next_chunk < total_chunks) {

                int start_row = next_chunk * CHUNK_ROWS;
                int rows = height - start_row;

                if (rows > CHUNK_ROWS)
                    rows = CHUNK_ROWS;

                int info[3] = {start_row, rows, width};

                int bytes = (int)((size_t)rows * width *
                                  sizeof(Pixel));

                MPI_Send(info, 3, MPI_INT,
                         worker, TAG_TASK, MPI_COMM_WORLD);

                MPI_Send(img->pixels + (size_t)start_row * width,
                         bytes, MPI_BYTE,
                         worker, TAG_DATA, MPI_COMM_WORLD);

                next_chunk++;

            } else {

                MPI_Send(NULL, 0, MPI_BYTE,
                         worker, TAG_STOP, MPI_COMM_WORLD);
            }
        }

        while (completed < total_chunks) {

            int info[2];
            MPI_Status status;

            MPI_Recv(info, 2, MPI_INT,
                     MPI_ANY_SOURCE, TAG_RESULT,
                     MPI_COMM_WORLD, &status);

            int worker = status.MPI_SOURCE;
            int start_row = info[0];
            int rows = info[1];

            int bytes = (int)((size_t)rows * width *
                              sizeof(Pixel));

            MPI_Recv(buffer, bytes, MPI_BYTE,
                     worker, TAG_DATA,
                     MPI_COMM_WORLD, MPI_STATUS_IGNORE);

            for (int y = 0; y < rows; y++) {
                for (int x = 0; x < width; x++) {

                    img->pixels[(size_t)(start_row + y) * width + x] =
                        buffer[(size_t)y * width + x];
                }
            }

            completed++;

            if (next_chunk < total_chunks) {

                int new_start = next_chunk * CHUNK_ROWS;
                int new_rows = height - new_start;

                if (new_rows > CHUNK_ROWS)
                    new_rows = CHUNK_ROWS;

                int new_info[3] = {new_start, new_rows, width};

                int new_bytes = (int)((size_t)new_rows * width *
                                      sizeof(Pixel));

                MPI_Send(new_info, 3, MPI_INT,
                         worker, TAG_TASK, MPI_COMM_WORLD);

                MPI_Send(img->pixels + (size_t)new_start * width,
                         new_bytes, MPI_BYTE,
                         worker, TAG_DATA, MPI_COMM_WORLD);

                next_chunk++;

            } else {

                MPI_Send(NULL, 0, MPI_BYTE,
                         worker, TAG_STOP, MPI_COMM_WORLD);
            }
        }

        double stop = MPI_Wtime();

        printf("\nTimp MPI Grayscale Dynamic: %.6f secunde\n",
               stop - start);

        if (save_image("rez_mpi_grayscale_dynamic.png", img))
            printf("Imagine MPI Dynamic salvata cu succes!\n");
        else
            printf("Eroare la salvarea imaginii!\n");

        free(buffer);
        free_image(img);

    } else {

        while (1) {

            int info[3];
            MPI_Status status;

            MPI_Recv(info, 3, MPI_INT,
                     0, MPI_ANY_TAG,
                     MPI_COMM_WORLD, &status);

            if (status.MPI_TAG == TAG_STOP)
                break;

            int start_row = info[0];
            int rows = info[1];
            int width = info[2];

            size_t count = (size_t)rows * width;
            size_t bytes = count * sizeof(Pixel);

            Pixel *pixels = malloc(bytes);

            if (pixels == NULL) {
                printf("Worker %d: eroare de memorie!\n", rank);
                MPI_Abort(MPI_COMM_WORLD, 1);
            }

            MPI_Recv(pixels, (int)bytes, MPI_BYTE,
                     0, TAG_DATA, MPI_COMM_WORLD,
                     MPI_STATUS_IGNORE);

            for (size_t i = 0; i < count; i++) {

                unsigned char gray = (unsigned char)(
                    0.299 * pixels[i].r +
                    0.587 * pixels[i].g +
                    0.114 * pixels[i].b
                );

                pixels[i].r = gray;
                pixels[i].g = gray;
                pixels[i].b = gray;
            }

            int result_info[2] = {start_row, rows};

            MPI_Send(result_info, 2, MPI_INT,
                     0, TAG_RESULT, MPI_COMM_WORLD);

            MPI_Send(pixels, (int)bytes, MPI_BYTE,
                     0, TAG_DATA, MPI_COMM_WORLD);

            free(pixels);
        }

        printf("Worker %d: toate sarcinile finalizate!\n", rank);
    }

    MPI_Finalize();
    return 0;
}
