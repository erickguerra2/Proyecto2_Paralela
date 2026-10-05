/* secuencial.c - Fuerza bruta secuencial para encontrar la llave DES.
 * Uso: ./secuencial <cifrado.bin> <palabraClave> [bitsEspacio]
 * bitsEspacio (1..56, por defecto 56) limita la busqueda a [0, 2^bitsEspacio). */
#include <errno.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <time.h>
#include "des_core.h"

/* Lee el archivo cifrado a textoCifrado. Devuelve su largo, o -1 si falla. */
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

/* Tiempo monotonico en segundos. */
static double ahora(void)
{
    struct timespec t;
    clock_gettime(CLOCK_MONOTONIC, &t);
    return t.tv_sec + t.tv_nsec / 1e9;
}

int main(int argc, char *argv[])
{
    char textoCifrado[DES_MAX_BYTES_TEXTO];
    long bitsEspacio = DES_BITS_LLAVE;

    if (argc < 3 || argc > 4) {
        fprintf(stderr, "Uso: %s <cifrado.bin> <palabraClave> [bitsEspacio]\n", argv[0]);
        return 1;
    }
    const char *palabraClave = argv[2];
    if (palabraClave[0] == '\0') {
        fprintf(stderr, "Error: la palabra clave no puede estar vacia\n");
        return 1;
    }
    if (argc == 4) {
        char *fin;
        errno = 0;
        bitsEspacio = strtol(argv[3], &fin, 10);
        if (errno != 0 || *fin != '\0' || bitsEspacio < 1 || bitsEspacio > DES_BITS_LLAVE) {
            fprintf(stderr, "Error: bitsEspacio debe estar entre 1 y %d\n", DES_BITS_LLAVE);
            return 1;
        }
    }

    int largo = leerCifrado(argv[1], textoCifrado);
    if (largo < 0) return 1;

    long long totalLlaves = 1LL << bitsEspacio;
    long long llaveEncontrada = -1;

    double inicio = ahora();
    for (long long llave = 0; llave < totalLlaves; llave++) {
        if (tryKey((long)llave, textoCifrado, largo, palabraClave)) {
            llaveEncontrada = llave;
            break;
        }
    }
    double tiempo = ahora() - inicio;

    if (llaveEncontrada < 0) {
        printf("Llave no encontrada en [0, 2^%ld)\n", bitsEspacio);
        return 2;
    }

    decrypt((long)llaveEncontrada, textoCifrado, largo);
    printf("Archivo: %s\nPalabra clave: %s\nLlave: %lld\n", argv[1], palabraClave, llaveEncontrada);
    printf("Iteraciones: %lld\nTiempo: %.6f s\n", llaveEncontrada + 1, tiempo);
    printf("Texto descifrado:\n%.*s\n", largo, textoCifrado);
    return 0;
}
