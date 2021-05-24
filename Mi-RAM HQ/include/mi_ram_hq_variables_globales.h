#ifndef MI_RAM_HQ_VARIABLES_GLOBALES_H_
#define MI_RAM_HQ_VARIABLES_GLOBALES_H_

#define TAMANIO_PCB 8
#define TAMANIO_TCB 21

#include "variables_globales_shared.h"
#include "commons/bitarray.h"
#include "commons/config.h"
#include "commons/collections/list.h"
#include <sys/types.h>
#include <string.h>
#include "commons/string.h"

// MEMORIA PRINCIPAL
void* memoria_principal;
int tamanio_memoria;

// Mapa de posiciones de memoria libres
t_bitarray*	mapa_memoria_disponible;
char* bitarray_mapa_memoria_disponible;

#endif