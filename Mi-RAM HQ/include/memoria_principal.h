#ifndef MEMORIA_PRINCIPAL_H_
#define MEMORIA_PRINCIPAL_H_

#include "variables_globales_shared.h"
#include "commons/bitarray.h"
#include "commons/config.h"
#include "commons/collections/list.h"
#include <sys/types.h>
#include <string.h>
#include "commons/string.h"

#define TAMANIO_PCB 8
#define TAMANIO_TCB 21

void inicializar_estructuras_memoria(t_config* config);

void* memoria_principal;
int tamanio_memoria;

// ESTRUCTURAS PARA ADMINISTRAR LA MEMORIA PRINCIPAL

// Tablas de segmentos
t_list* tablas_de_segmentos;

// Mapa de posiciones de memoria libres
t_bitarray*	mapa_memoria_disponible;
char* bitarray_mapa_memoria_disponible;

#endif