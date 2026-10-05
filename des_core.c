/* des_core.c - Implementacion del nucleo DES sobre OpenSSL. */

#define OPENSSL_SUPPRESS_DEPRECATED

#include <openssl/des.h>
#include <string.h>
#include "des_core.h"

/* Arma el key schedule: 7 bits de llave por byte, el bit bajo es paridad. */
static void armarSchedule(long key, DES_key_schedule *schedule)
{
    DES_cblock llaveBloque;
    for (int i = 0; i < DES_BLOQUE; i++) {
        llaveBloque[i] = (unsigned char)(((key >> (7 * i)) & 0x7F) << 1);
    }
    DES_set_odd_parity(&llaveBloque);
    DES_set_key_unchecked(&llaveBloque, schedule);
}

/* Aplica DES (ECB) bloque por bloque; modo es DES_ENCRYPT o DES_DECRYPT. */
static void procesarBloques(long key, char *ciph, int len, int modo)
{
    DES_key_schedule schedule;
    armarSchedule(key, &schedule);
    for (int i = 0; i + DES_BLOQUE <= len; i += DES_BLOQUE) {
        DES_cblock *bloque = (DES_cblock *)(ciph + i);
        DES_ecb_encrypt(bloque, bloque, &schedule, modo);
    }
}

void encrypt(long key, char *ciph, int len)
{
    procesarBloques(key, ciph, len, DES_ENCRYPT);
}

void decrypt(long key, char *ciph, int len)
{
    procesarBloques(key, ciph, len, DES_DECRYPT);
}

int tryKey(long key, char *ciph, int len, const char *keyword)
{
    char textoTemp[DES_MAX_BYTES_TEXTO + 1];

    if (ciph == NULL || keyword == NULL || len <= 0 ||
        len > DES_MAX_BYTES_TEXTO || len % DES_BLOQUE != 0) {
        return 0;
    }
    memcpy(textoTemp, ciph, len);  /* copia para conservar el cifrado */
    textoTemp[len] = '\0';
    decrypt(key, textoTemp, len);
    return strstr(textoTemp, keyword) != NULL;
}
