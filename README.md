# Tema 1 – SOD: Procesarea imaginilor cu MPI

Proiect realizat în C, folosind MPI pentru procesarea paralelă a imaginilor.

Operații implementate: **Grayscale, Blur și Horizontal Flip**.

## Fișierele proiectului

### Fișiere de bază
- `image.c` / `image.h` – încărcarea, salvarea și reprezentarea imaginilor.
- `operations.c` / `operations.h` – implementarea secvențială a celor 3 operații.
- `stb_image.h` / `stb_image_write.h` – biblioteci pentru citirea și salvarea imaginilor.
- `poza.jpg` – imaginea folosită pentru testare (4016 × 6016 pixeli).

### Programe
- `main.c` – programul secvențial.
- `main_mpi.c` – Grayscale MPI, partiționare 1D statică.
- `main_mpi_grayscale.c` – copie a variantei Grayscale MPI 1D.
- `main_mpi_blur.c` – Blur MPI 1D static, cu halo rows.
- `main_mpi_flip.c` – Horizontal Flip MPI 1D static.
- `main_mpi_2d.c` – Grayscale MPI 2D static.
- `main_mpi_dynamic.c` – Grayscale MPI 1D cu distribuire dinamică.
- `main_mpi_pipeline.c` – aplică succesiv operațiile alese de utilizator, folosind MPI.
- `verificare_pipeline.c` – verifică rezultatul pipeline-ului MPI prin comparație cu varianta secvențială.

## Timpi obținuți până acum

Imagine test: **4016 × 6016 pixeli**.

| Operație | Variantă | Timp |
|---|---|---|
| Grayscale | Secvențial | 0,102170 s |
| Grayscale | MPI 1D static, 4 procese | 0,063877 s |
| Grayscale | MPI 1D static, 8 procese | 0,041237 s |
| Grayscale | MPI 2D static, 5 procese | 0,189013 s |
| Grayscale | MPI dinamic, 4 procese | 0,090777 s |
| Blur 21×21 | Secvențial | 39,387734 s |
| Blur 21×21 | MPI 1D static, 4 procese | 14,983115 s |
| Blur 3×3 | MPI 1D static, 4 procese | 0,457256 s |
| Horizontal Flip | Secvențial | 0,054443 s |
| Horizontal Flip | MPI 1D static, 4 procese | 0,058891 s |

### Pipeline MPI – 4 procese

Grayscale → Blur 3×3 → Horizontal Flip

**Timp total: 0,543620 s**

Rezultatele MPI au fost verificate prin comparație cu rezultatele secvențiale.

*Timpii sunt orientativi, din rulări individuale. Experimentele finale vor include mai multe rulări și medii.*

## Ce mai avem de făcut

- Configurarea clusterului MPI pe două laptopuri.
- Testarea cu rezoluții și numere de procese diferite.
- Măsurători repetate, grafice și concluzii.
