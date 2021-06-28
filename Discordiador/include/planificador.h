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
#include <semaphore.h>
#include <commons/temporal.h>

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
t_queue *buffer_peticiones_ready_to_exec;

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

sem_t sem_mutex_buffer_blocked_io_to_ready;
sem_t sem_mutex_buffer_exec_to_ready;
sem_t sem_mutex_buffer_exec_to_blocked_io;

sem_t sem_mutex_ingreso_tripulantes_new;
sem_t sem_mutex_ejecutar_dispatcher;
sem_t sem_puede_expulsar_tripulante;
sem_t sem_confirmar_expulsion_tripulante;
sem_t sem_sabotaje_activado;
sem_t sem_mutex_tripulante_a_expulsar;

int iniciar_dispatcher(char *algoritmo_planificador);
int listar_tripulantes(void);
char *code_dispatcher_to_string(enum estado_tripulante code);

int dispatcher_expulsar_tripulante(int tid_tripulante);
void dispatcher_pausar(void);
enum algoritmo string_to_code_algor(char *string_algor);
void agregar_a_buffer_peticiones(t_queue *buffer, int tid);
int dispatcher_eliminar_tripulante(int tid_eliminar);
void iniciador_tripulante(int tid, int pid);

#endif
