#ifndef _PLANIFICADOR_H_
#define _PLANIFICADOR_H_

#include <commons/collections/queue.h>
#include <stdbool.h>
#include "variables_globales_shared.h"
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <stdio.h>
#include <pthread.h>

// en ESTADO NEW: genera las estructuras administrativas , una vez generado todo --> pasa a READY.
t_queue *cola_new;
t_queue *cola_ready;
t_queue *cola_running;
t_queue *cola_bloqueado_io;
t_queue *cola_bloqueado_emergency;
t_queue *cola_exit;

// agrego cola buffer que interactua con cada tripulante - estos contienen el TID respectivo
t_queue *buffer_peticiones_exec_to_blocked_io;
t_queue *buffer_peticiones_blocked_io_to_ready;
t_queue *buffer_peticiones_exec_to_ready; // RR

enum status_planificador estado_planificador;
// Estructura de dato para colas del planificador / dispatcher
typedef struct dato_tripulante{
    int PID, TID;
    int estado_previo;
    int quantum;
}t_tripulante;

// CODIGO ESTADO del Tripulante
enum estado_tripulante{
    NEW,
    READY,
    EXEC,
    BLOCKED_IO,
    BLOCKED_EMERGENCY,
    EXIT
};

enum status_planificador {
    PLANIFICADOR_OFF,
    PLANIFICADOR_RUNNING,
    PLANIFICADOR_BLOCKED
};

enum algoritmo {
    FIFO,
    RR
};

int iniciar_planificador(char *algoritmo_planificador);
int existe_tripulantes_en_cola(t_queue *cola);

void listar_tripulantes(enum algoritmo cod);
char *code_dispatcher_to_string(enum estado_tripulante code);

int hay_espacio_disponible(int grado_multiprocesamiento);
int hay_tarea_a_realizar(void);
int hay_tripulantes_a_borrar(void);
void expulsar_tripulante(void);
int hay_bloqueo_io(void);
int hay_sabotaje(void);
int se_solicito_lista_trip(void);
int termino_sabotaje(void);

enum algoritmo string_to_code_algor(char *string_algor);

int get_index_from_cola_by_tid(t_queue *src_list, int tid_buscado);
void agregar_a_buffer_peticiones(t_queue *buffer, int tid);

#endif
