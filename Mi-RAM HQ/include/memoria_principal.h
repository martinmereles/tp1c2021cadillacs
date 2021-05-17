#ifndef MEMORIA_PRINCIPAL_H_
#define MEMORIA_PRINCIPAL_H_

#include "variables_globales_shared.h"
#include "commons/bitarray.h"
#include "commons/config.h"
#include "commons/collections/list.h"
#include <sys/types.h>
#include <string.h>

#define TAMANIO_PCB 8
#define TAMANIO_TCB 17

#define DIR_LOG_PCB     0x00000000
#define DIR_LOG_TAREAS  0x80000000
#define DESPL_PID       0x00000000 
#define DESPL_TAREAS    0x00000004 

// Una tabla de segmentos es un array de filas??
/*
direccion logica = nro segmento | desplazamiento

nro segmento: 1 bit
desplazamiento: 31 bits
*/

typedef struct{
    uint32_t inicio;
    uint32_t tamanio;
} fila_tabla_segmentos_t;

typedef struct{
    fila_tabla_segmentos_t** filas;
} tabla_segmentos_t;

void inicializar_estructuras_memoria(t_config* config);
void liberar_estructuras_memoria();
fila_tabla_segmentos_t* reservar_segmento(int tamanio);
void liberar_segmento(fila_tabla_segmentos_t* fila);
int first_fit(int memoria_pedida);
void memcpy_tabla_segmentos(tabla_segmentos_t*, uint32_t, void*, int);

void* memoria_principal;
int tamanio_memoria;

// ESTRUCTURAS PARA ADMINISTRAR LA MEMORIA PRINCIPAL

// SEGMENTACION

// Tablas de segmentos
t_list* tablas_de_segmentos_de_patotas;
t_list* tablas_de_segmentos_de_tripulantes;

// Mapa de posiciones de memoria libres (segmentacion)
t_bitarray*	mapa_memoria_disponible;
char* bitarray_mapa_memoria_disponible;

#endif