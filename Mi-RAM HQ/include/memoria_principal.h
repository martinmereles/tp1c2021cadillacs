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

#define DIR_LOG_PCB     0x00000000
#define DIR_LOG_TAREAS  0x00010000
#define DESPL_PID       0 
#define DESPL_TAREAS    4 

#define DESPL_TID           0
#define DESPL_ESTADO        4
#define DESPL_POS_X         5
#define DESPL_POS_Y         9
#define DESPL_PROX_INSTR    13
#define DESPL_DIR_PCB       17

/*
direccion logica = nro segmento | desplazamiento
nro segmento: 16 bits
desplazamiento: 16 bits
*/

typedef struct{
    uint32_t numero_segmento;
    uint32_t inicio;
    uint32_t tamanio;
} fila_tabla_segmentos_t;

typedef struct{
    t_list *filas;
    uint32_t proximo_numero_segmento;
} tabla_segmentos_t;

fila_tabla_segmentos_t* crear_fila(tabla_segmentos_t* tabla, int tamanio);
int inicializar_estructuras_memoria(t_config* config);
void liberar_estructuras_memoria();
fila_tabla_segmentos_t* reservar_segmento(int tamanio);
void liberar_segmento(fila_tabla_segmentos_t* fila);
int escribir_memoria_principal(tabla_segmentos_t*, uint32_t, void*, int);
int leer_memoria_principal(tabla_segmentos_t*, uint32_t, void*, int);
int calcular_direccion_fisica(tabla_segmentos_t* , uint32_t );
void dump_patota(tabla_segmentos_t* tabla_patota);
void dump_tripulante(tabla_segmentos_t* tabla, int nro_fila);
tabla_segmentos_t* obtener_tabla_patota(int PID_buscado);
void quitar_fila(tabla_segmentos_t* tabla, int numero_fila);
void agregar_fila(tabla_segmentos_t* tabla, fila_tabla_segmentos_t* fila);
fila_tabla_segmentos_t* obtener_fila(tabla_segmentos_t* tabla, int numero_fila);
int cantidad_filas(tabla_segmentos_t* tabla);
tabla_segmentos_t* crear_tabla_segmentos();
void destruir_tabla_segmentos(void* tabla_de_segmentos);
uint32_t numero_de_segmento(uint32_t direccion_logica);
uint32_t desplazamiento(uint32_t direccion_logica);
void leer_tarea_memoria_principal(tabla_segmentos_t* tabla, uint32_t dir_log_tareas, char** tarea, int id_prox_tarea);
int cantidad_tareas(char** array_tareas);
int generar_nuevo_numero_segmento(tabla_segmentos_t* tabla);
void destruir_fila(fila_tabla_segmentos_t* fila);
void quitar_y_destruir_fila(tabla_segmentos_t* tabla, int numero_seg);
uint32_t direccion_logica(fila_tabla_segmentos_t* fila);
void quitar_y_destruir_tabla(tabla_segmentos_t* tabla_a_destruir);


void* memoria_principal;
int tamanio_memoria;

// ALGORITMOS DE UBICACION DE SEGMENTOS
int (*algoritmo_de_ubicacion)(int);
int first_fit(int memoria_pedida);
int best_fit(int memoria_pedida);
int inicializar_algoritmo_de_ubicacion(t_config* config);

// ESTRUCTURAS PARA ADMINISTRAR LA MEMORIA PRINCIPAL

// SEGMENTACION

// Tablas de segmentos
t_list* tablas_de_segmentos;

// Mapa de posiciones de memoria libres (segmentacion)
t_bitarray*	mapa_memoria_disponible;
char* bitarray_mapa_memoria_disponible;

#endif