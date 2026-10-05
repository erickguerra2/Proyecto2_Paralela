/* test_des_core.c - Prueba rapida de encrypt, decrypt y tryKey. */
#include <stdio.h>
#include <string.h>
#include "des_core.h"

int main(void)
{
    const long llave = 123456L;
    const char *claro = "Texto de prueba con palabra zafiro dentro";
    char buffer[48];
    int len = 48;
    int fallos = 0;

    memset(buffer, 0, sizeof(buffer));
    strcpy(buffer, claro);

    encrypt(llave, buffer, len);
    if (strcmp(buffer, claro) == 0) {
        printf("FALLA: el texto no cambio al cifrar\n");
        fallos++;
    }

    if (!tryKey(llave, buffer, len, "zafiro")) {
        printf("FALLA: tryKey no acepto la llave correcta\n");
        fallos++;
    }
    if (tryKey(llave + 1, buffer, len, "zafiro")) {
        printf("FALLA: tryKey acepto una llave incorrecta\n");
        fallos++;
    }

    decrypt(llave, buffer, len);
    if (strcmp(buffer, claro) != 0) {
        printf("FALLA: decrypt no recupero el texto original\n");
        fallos++;
    }

    printf(fallos == 0 ? "OK: todas las pruebas pasaron\n" : "%d fallas\n", fallos);
    return fallos != 0;
}
