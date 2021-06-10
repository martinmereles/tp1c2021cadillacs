#ifndef SEGMENTACION_H_
#define SEGMENTACION_H_

#include "mi_ram_hq_variables_globales.h"

#define DIR_LOG_PCB     0x00000000
#define DIR_LOG_TAREAS  0x00010000

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

// FUNCIONES SEGMENTACION
fila_tabla_segmentos_t* reservar_segmento(int tamanio);
void liberar_segmento(fila_tabla_segmentos_t* fila);
fila_tabla_segmentos_t* crear_fila(tabla_segmentos_t* tabla, int tamanio);
int calcular_direccion_fisica(tabla_segmentos_t* , uint32_t );
tabla_segmentos_t* obtener_tabla_patota(int PID_buscado);
void quitar_fila(tabla_segmentos_t* tabla, int numero_fila);
void agregar_fila(tabla_segmentos_t* tabla, fila_tabla_segmentos_t* fila);
fila_tabla_segmentos_t* obtener_fila(tabla_segmentos_t* tabla, int numero_fila);
int cantidad_filas(tabla_segmentos_t* tabla);

// Direccionamiento
uint32_t direccion_logica_segmentacion(uint32_t inicio_logico, uint32_t desplazamiento_logico);
uint32_t direccion_logica_segmento(fila_tabla_segmentos_t* fila);
uint32_t desplazamiento(uint32_t direccion_logica);
uint32_t numero_de_segmento(uint32_t direccion_logica);

// Escritura/Lectura
int escribir_memoria_principal_segmentacion(void* args, uint32_t inicio_logico, uint32_t desplazamiento_logico, void* dato, int tamanio);
int leer_memoria_principal_segmentacion(void* args, uint32_t inicio_logico, uint32_t desplazamiento_logico, void* dato, int tamanio);

// Patotas y Tripulantes
int crear_patota_segmentacion(uint32_t PID, uint32_t longitud_tareas, char* tareas);
int crear_tripulante_segmentacion(void**, uint32_t*, uint32_t, uint32_t, uint32_t, uint32_t);

// Tablas de segmentos
void destruir_tabla_segmentos(void* tabla_de_segmentos);
void leer_tarea_memoria_principal(tabla_segmentos_t* tabla, uint32_t dir_log_tareas, char** tarea, int id_prox_tarea);
int cantidad_tareas(char** array_tareas);
int generar_nuevo_numero_segmento(tabla_segmentos_t* tabla);
void destruir_fila(void* fila);
void quitar_y_destruir_fila(tabla_segmentos_t* tabla, int numero_seg);
void quitar_y_destruir_tabla(tabla_segmentos_t* tabla_a_destruir);

// Dump
void dump_memoria_segmentacion();
void dump_patota_segmentacion(tabla_segmentos_t* tabla_patota, FILE* archivo_dump);
void dump_segmento(FILE* archivo_dump, tabla_segmentos_t* tabla, int PID, int nro_segmento);
void dump_patota_segmentacion_pruebas(void* args);
void dump_tripulante_segmentacion_pruebas(tabla_segmentos_t* tabla, int nro_fila);

// ALGORITMOS DE UBICACION DE SEGMENTOS
int (*algoritmo_de_ubicacion)(int);
int first_fit(int memoria_pedida);
int best_fit(int memoria_pedida);
int inicializar_algoritmo_de_ubicacion(t_config* config);

#endif