#include "distribucion.h"

void calcularRango(int rank, int size, long long totalKeys, long long *inicio, long long *fin)
{
    long long porProceso = totalKeys / size;
    *inicio = rank * porProceso;
    *fin = (rank == size - 1) ? totalKeys : *inicio + porProceso;
}
