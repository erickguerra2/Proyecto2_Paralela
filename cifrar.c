/* cifrar.c - Cifra un .txt (max 350 palabras) con DES y una llave de 56 bits.
 * Uso: ./cifrar <entrada.txt> <salida.bin> <llave> */
#include <ctype.h>
#include <errno.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include "des_core.h"

#define MAX_PALABRAS 350

/* Cuenta palabras separadas por espacios en blanco. */
static int contarPalabras(const char *texto, long largo)
{
    int palabras = 0, enPalabra = 0;
    for (long i = 0; i < largo; i++) {
        if (isspace((unsigned char)texto[i])) {
            enPalabra = 0;
        } else if (!enPalabra) {
            enPalabra = 1;
            palabras++;
        }
    }
    return palabras;
}

/* Convierte texto a llave de 56 bits. Devuelve 0 si es invalida. */
static int leerLlave(const char *texto, long *llave)
{
    char *fin;
    errno = 0;
    long valor = strtol(texto, &fin, 10);
    if (errno != 0 || *texto == '\0' || *fin != '\0') return 0;
    if (valor < 0 || valor >= (1L << DES_BITS_LLAVE)) return 0;
    *llave = valor;
    return 1;
}

int main(int argc, char *argv[])
{
    long llave;
    char textoCifrado[DES_MAX_BYTES_TEXTO];

    if (argc != 4) {
        fprintf(stderr, "Uso: %s <entrada.txt> <salida.bin> <llave>\n", argv[0]);
        return 1;
    }
    if (!leerLlave(argv[3], &llave)) {
        fprintf(stderr, "Error: la llave debe ser un entero en [0, 2^56)\n");
        return 1;
    }

    FILE *entrada = fopen(argv[1], "rb");
    if (entrada == NULL) {
        fprintf(stderr, "Error: no se pudo abrir '%s'\n", argv[1]);
        return 1;
    }
    memset(textoCifrado, 0, sizeof(textoCifrado));
    long largo = (long)fread(textoCifrado, 1, DES_MAX_BYTES_TEXTO, entrada);
    int sobrante = fgetc(entrada);
    fclose(entrada);

    if (largo == 0) {
        fprintf(stderr, "Error: el archivo esta vacio\n");
        return 1;
    }
    if (sobrante != EOF || largo >= DES_MAX_BYTES_TEXTO) {
        fprintf(stderr, "Error: el archivo excede %d bytes\n", DES_MAX_BYTES_TEXTO - 1);
        return 1;
    }
    if (contarPalabras(textoCifrado, largo) > MAX_PALABRAS) {
        fprintf(stderr, "Error: el texto excede %d palabras\n", MAX_PALABRAS);
        return 1;
    }

    /* relleno con ceros hasta multiplo de 8 (el buffer ya esta en cero) */
    int lenCifrado = (int)((largo + DES_BLOQUE - 1) / DES_BLOQUE) * DES_BLOQUE;
    encrypt(llave, textoCifrado, lenCifrado);

    FILE *salida = fopen(argv[2], "wb");
    if (salida == NULL ||
        fwrite(textoCifrado, 1, lenCifrado, salida) != (size_t)lenCifrado) {
        fprintf(stderr, "Error: no se pudo escribir '%s'\n", argv[2]);
        if (salida != NULL) fclose(salida);
        return 1;
    }
    fclose(salida);

    printf("Cifrado: %s (%d bytes) con llave %ld\n", argv[2], lenCifrado, llave);
    return 0;
}
