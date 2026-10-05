/* des_core.h - Nucleo DES (OpenSSL). Llave de 56 bits, modo ECB, len multiplo de 8. */
#ifndef DES_CORE_H
#define DES_CORE_H

#define DES_BLOQUE 8
#define DES_BITS_LLAVE 56
#define DES_MAX_BYTES_TEXTO 8192

/* Cifra ciph en sitio. Entradas: key, ciph, len. */
void encrypt(long key, char *ciph, int len);

/* Descifra ciph en sitio. Entradas: key, ciph, len. */
void decrypt(long key, char *ciph, int len);

/* Devuelve 1 si descifrar ciph con key produce un texto que contiene keyword. */
int tryKey(long key, char *ciph, int len, const char *keyword);

#endif
