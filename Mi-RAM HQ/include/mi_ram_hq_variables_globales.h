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
#include <pthread.h>
#include <semaphore.h>
#include "mapa.h"

// La direccion logica del PCB es siempre la misma
#define DIR_LOG_PCB     0x00000000

// DESPLAZAMIENTOS LOGICOS
// Para el PCB
#define DESPL_PID       0
#define DESPL_TAREAS    4
// Para el TCB
#define DESPL_TID           0
#define DESPL_ESTADO        4
#define DESPL_POS_X         5
#define DESPL_POS_Y         9
#define DESPL_PROX_INSTR    13
#define DESPL_DIR_PCB       17

// MEMORIA PRINCIPAL
void* memoria_principal;
int tamanio_memoria;

// Mapa de posiciones de memoria libres
// SEGMENTACION
t_list* lista_segmentos;
// t_bitarray*	mapa_memoria_disponible;
// char* bitarray_mapa_memoria_disponible;

// ESTRUCTURAS PARA ADMINISTRAR LA MEMORIA PRINCIPAL

// Tablas de patotas
t_list* tablas_de_patotas;

// Semaforos
sem_t reservar_liberar_memoria_mutex;
sem_t mutex_proceso_swap;
sem_t mutex_temporal;

// PUNTEROS A FUNCIONES GLOBALES
// Sirven para apuntar a las funciones que implementan segmentacion o paginacion, segun la configuracion
void (*dump_memoria)(void);
int (*crear_patota)(uint32_t, uint32_t, char*);
int (*crear_tripulante)(void**, uint32_t*, uint32_t, uint32_t, uint32_t, uint32_t);
int (*escribir_memoria_principal)(void*, uint32_t, uint32_t, void*, int);
int (*leer_memoria_principal)(void*, uint32_t, uint32_t, void*, int);
void* (*obtener_tabla_patota)(int);
int (*tamanio_tareas)(void*);
void (*eliminar_tripulante)(void*, uint32_t);
uint32_t (*espacio_disponible)(void);
//  uint32_t (*direccion_logica)(uint32_t, uint32_t);   // Hace falta??

// Funciones comunes a paginacion y segmentacion
int cantidad_tareas(char** array_tareas);
uint32_t get_desplazamiento(uint32_t direccion_logica);
void crear_archivo_dump(t_list* lista_dump, void (*funcion_dump)(void*,FILE*));
void ejecutar_rutina(void (*rutina)(void));

#endif