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

#define CANT_ESTADOS 6
#define CANT_COLAS 9

// CODIGO ESTADO del Tripulante
enum estado_tripulante{ // Deben ir numerados desde 0 sin saltar numeros
    NEW = 0,
    READY = 1,
    EXEC = 2,
    BLOCKED_IO = 3,
    BLOCKED_EMERGENCY = 4,
    EXIT = 5,
};

enum peticion_transicion{
    EXEC_TO_BLOCKED_IO = 6,
    BLOCKED_IO_TO_READY = 7,
    EXEC_TO_READY = 8
};

// Estructura de dato para colas del planificador / dispatcher
typedef struct dato_tripulante{
    int PID, TID;
    enum estado_tripulante estado;
    int quantum;
    int posicion_X;
    int posicion_Y;
    sem_t* sem_planificacion_fue_reanudada;
    sem_t* sem_finalizo;
    sem_t* sem_puede_usar_dispositivo_io;
    sem_t** sem_tripulante_dejo;
}t_tripulante;

enum status_planificador {
    PLANIFICADOR_OFF,
    PLANIFICADOR_RUNNING,
    PLANIFICADOR_BLOCKED
};

enum algoritmo {
    FIFO,
    RR
};

// PROTOTIPOS DE FUNCIONES
int iniciar_dispatcher(char *algoritmo_planificador);
int listar_tripulantes(void);
char *code_dispatcher_to_string(enum estado_tripulante code);
int rutina_expulsar_tripulante(void* args);
void dispatcher_pausar(void);
enum algoritmo string_to_code_algor(char *string_algor);
int dispatcher_eliminar_tripulante(int tid_eliminar);
t_tripulante* iniciador_tripulante(int tid, int pid);
void crear_colas();
void encolar(int tipo_cola, t_tripulante* tripulante);
t_tripulante* desencolar(int tipo_cola);
t_tripulante* ojear_cola(int tipo_cola);
int cantidad_tripulantes_en_cola(int tipo_cola);

// VARIABLES GLOBALES
sem_t sem_mutex_ingreso_tripulantes_new;
sem_t sem_mutex_ejecutar_dispatcher;
sem_t sem_sabotaje_activado;

// en ESTADO NEW: genera las estructuras administrativas , una vez generado todo --> pasa a READY.
t_queue **cola;
sem_t ** mutex_cola;

// agrego cola buffer que interactua con cada tripulante - estos contienen el TID respectivo
t_queue *buffer_peticiones_exec_to_blocked_io;
t_queue *buffer_peticiones_blocked_io_to_ready;
t_queue *buffer_peticiones_exec_to_ready; // RR

enum status_planificador estado_planificador;

// Lista global de tripulantes (sirve para pausar/reanudar la planificacion)
t_list* lista_tripulantes;

#endif
