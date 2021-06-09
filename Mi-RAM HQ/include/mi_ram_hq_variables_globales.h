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
#include "commons/temporal.h"

// MEMORIA PRINCIPAL
void* memoria_principal;
int tamanio_memoria;

// Mapa de posiciones de memoria libres
t_bitarray*	mapa_memoria_disponible;
char* bitarray_mapa_memoria_disponible;

// ESTRUCTURAS PARA ADMINISTRAR LA MEMORIA PRINCIPAL

// Tablas de patotas
t_list* tablas_de_patotas;

// PUNTEROS A FUNCIONES GLOBALES
// Sirven para apuntar a las funciones que implementan segmentacion o paginacion, segun la configuracion
void (*dump_memoria)(void);
int (*crear_patota)(uint32_t, uint32_t, char*);
int (*crear_tripulante)(void**, uint32_t*, uint32_t, uint32_t, uint32_t, uint32_t);

#endif