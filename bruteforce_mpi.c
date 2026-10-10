#include <errno.h>
#include <mpi.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include "des_core.h"
#include "distribucion.h"

#define TAG_LLAVE 1
#define INTERVALO_REVISION 1000

static void verificarMPI(int codigo, const char *operacion)
{
    if (codigo != MPI_SUCCESS) {
        char mensaje[MPI_MAX_ERROR_STRING];
        int largoMensaje;
        MPI_Error_string(codigo, mensaje, &largoMensaje);
        fprintf(stderr, "Error en %s: %s\n", operacion, mensaje);
        MPI_Abort(MPI_COMM_WORLD, 1);
    }
}

static int leerCifrado(const char *ruta, char *textoCifrado)
{
    FILE *archivo = fopen(ruta, "rb");
    if (archivo == NULL) {
        fprintf(stderr, "Error: no se pudo abrir '%s'\n", ruta);
        return -1;
    }
    int largo = (int)fread(textoCifrado, 1, DES_MAX_BYTES_TEXTO, archivo);
    int sobrante = fgetc(archivo);
    fclose(archivo);

    if (largo <= 0 || sobrante != EOF || largo % DES_BLOQUE != 0) {
        fprintf(stderr, "Error: archivo cifrado invalido (vacio, muy grande o no multiplo de %d)\n",
                DES_BLOQUE);
        return -1;
    }
    return largo;
}

static int leerBits(const char *texto, int *bits)
{
    char *fin;
    errno = 0;
    long valor = strtol(texto, &fin, 10);
    if (errno != 0 || *texto == '\0' || *fin != '\0' || valor < 1 || valor > DES_BITS_LLAVE) {
        fprintf(stderr, "Error: bitsEspacio debe estar entre 1 y %d\n", DES_BITS_LLAVE);
        return 0;
    }
    *bits = (int)valor;
    return 1;
}

static int validarEntrada(int argc, char *argv[], int *bitsEspacio, char *textoCifrado)
{
    if (argc < 3 || argc > 4) {
        fprintf(stderr, "Uso: mpirun -np <procesos> %s <cifrado.bin> <palabraClave> [bitsEspacio]\n",
                argv[0]);
        return -1;
    }
    if (argv[2][0] == '\0') {
        fprintf(stderr, "Error: la palabra clave no puede estar vacia\n");
        return -1;
    }
    if (argc == 4 && !leerBits(argv[3], bitsEspacio)) {
        return -1;
    }
    return leerCifrado(argv[1], textoCifrado);
}

int main(int argc, char *argv[])
{
    int rank, size;
    char textoCifrado[DES_MAX_BYTES_TEXTO];
    int largo = -1;
    int bitsEspacio = DES_BITS_LLAVE;

    MPI_Init(&argc, &argv);
    MPI_Comm_set_errhandler(MPI_COMM_WORLD, MPI_ERRORS_RETURN);
    MPI_Comm_rank(MPI_COMM_WORLD, &rank);
    MPI_Comm_size(MPI_COMM_WORLD, &size);

    if (rank == 0) {
        largo = validarEntrada(argc, argv, &bitsEspacio, textoCifrado);
    }
    verificarMPI(MPI_Bcast(&largo, 1, MPI_INT, 0, MPI_COMM_WORLD), "MPI_Bcast");
    if (largo < 0) {
        MPI_Finalize();
        return 1;
    }
    verificarMPI(MPI_Bcast(textoCifrado, largo, MPI_CHAR, 0, MPI_COMM_WORLD), "MPI_Bcast");
    verificarMPI(MPI_Bcast(&bitsEspacio, 1, MPI_INT, 0, MPI_COMM_WORLD), "MPI_Bcast");

    const char *palabraClave = argv[2];
    long long totalLlaves = 1LL << bitsEspacio;
    long long llaveInicio, llaveFin;
    calcularRango(rank, size, totalLlaves, &llaveInicio, &llaveFin);

    long long llaveRecibida = -1;
    long long llaveLocal = -1;
    long long iteraciones = 0;
    int recibido = 0;
    MPI_Request solicitud;

    verificarMPI(MPI_Irecv(&llaveRecibida, 1, MPI_LONG_LONG, MPI_ANY_SOURCE, TAG_LLAVE,
                           MPI_COMM_WORLD, &solicitud), "MPI_Irecv");
    verificarMPI(MPI_Barrier(MPI_COMM_WORLD), "MPI_Barrier");
    double tiempoInicio = MPI_Wtime();

    for (long long llave = llaveInicio; llave < llaveFin && !recibido; llave++) {
        iteraciones++;
        if (tryKey((long)llave, textoCifrado, largo, palabraClave)) {
            llaveLocal = llave;
            for (int destino = 0; destino < size; destino++) {
                if (destino != rank) {
                    verificarMPI(MPI_Send(&llaveLocal, 1, MPI_LONG_LONG, destino, TAG_LLAVE,
                                          MPI_COMM_WORLD), "MPI_Send");
                }
            }
            break;
        }
        if (iteraciones % INTERVALO_REVISION == 0) {
            verificarMPI(MPI_Test(&solicitud, &recibido, MPI_STATUS_IGNORE), "MPI_Test");
        }
    }

    long long llaveGlobal;
    verificarMPI(MPI_Allreduce(&llaveLocal, &llaveGlobal, 1, MPI_LONG_LONG, MPI_MAX,
                               MPI_COMM_WORLD), "MPI_Allreduce");
    double tiempo = MPI_Wtime() - tiempoInicio;

    if (!recibido) {
        if (llaveGlobal < 0 || llaveLocal >= 0) {
            verificarMPI(MPI_Cancel(&solicitud), "MPI_Cancel");
        }
        verificarMPI(MPI_Wait(&solicitud, MPI_STATUS_IGNORE), "MPI_Wait");
    }

    long long infoLocal[2] = { -1, -1 };
    long long infoGlobal[2];
    if (llaveLocal >= 0) {
        infoLocal[0] = rank;
        infoLocal[1] = iteraciones;
    }
    verificarMPI(MPI_Reduce(infoLocal, infoGlobal, 2, MPI_LONG_LONG, MPI_MAX, 0, MPI_COMM_WORLD),
                 "MPI_Reduce");

    if (rank == 0) {
        if (llaveGlobal < 0) {
            printf("Llave no encontrada en [0, 2^%d)\n", bitsEspacio);
            printf("Procesos: %d\nTiempo: %.6f s\n", size, tiempo);
        } else {
            decrypt((long)llaveGlobal, textoCifrado, largo);
            printf("Archivo: %s\nPalabra clave: %s\nLlave: %lld\n", argv[1], palabraClave, llaveGlobal);
            printf("Procesos: %d\nEncontrada por proceso: %lld\nIteraciones de ese proceso: %lld\n",
                   size, infoGlobal[0], infoGlobal[1]);
            printf("Tiempo: %.6f s\n", tiempo);
            printf("Texto descifrado:\n%.*s\n", largo, textoCifrado);
        }
    }

    MPI_Finalize();
    return llaveGlobal < 0 ? 2 : 0;
}
