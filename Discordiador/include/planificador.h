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
#include <commons/config.h>

#define CANT_ESTADOS 6
#define CANT_COLAS 11

// CODIGO ESTADO del Tripulante
enum estado_tripulante{ // Deben ir numerados desde 0 sin saltar numeros
    NEW = 0,
    READY = 1,
    EXEC = 2,
    BLOCKED_IO = 3,
    BLOCKED_EMERGENCY = 4,
    EXIT = 5,
    READY_TEMPORAL = 6,
    EXEC_TEMPORAL = 7,
};

enum peticion_transicion{
    EXEC_TO_BLOCKED_IO = 8,
    BLOCKED_IO_TO_READY = 9,
    EXEC_TO_READY = 10
};

enum status_discordiador{   RUNNING,
                            END };

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

typedef struct {
    int pos_x;    
    int pos_y;
}t_sabotaje;

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
int iniciar_dispatcher();
int listar_tripulantes(void);
char *code_dispatcher_to_string(enum estado_tripulante code);
int rutina_expulsar_tripulante(void* args);
void dispatcher_pausar(void);
int dispatcher_eliminar_tripulante(int tid_eliminar);
t_tripulante* iniciador_tripulante(int tid, int pid, int pos_x, int pos_y);
void crear_colas();
void encolar(int tipo_cola, t_tripulante* tripulante);
t_tripulante* desencolar(int tipo_cola);
t_tripulante* ojear_cola(int tipo_cola);
int cantidad_tripulantes_en_cola(int tipo_cola);
void crear_estructuras_planificador();
void liberar_estructuras_planificador();
void finalizar_discordiador();
int inicializar_algoritmo_planificacion(t_config* config);

int transicion(t_tripulante *tripulante, enum estado_tripulante estado_inicial, enum estado_tripulante estado_final);
void destructor_elementos_tripulante(void *data);
void desbloquear_tripulantes_tras_sabotaje(void);
int bloquear_tripulantes_por_sabotaje(void);
t_tripulante *desencolar_tripulante_por_tid(t_queue *cola_src, int tid_buscado);

// VARIABLES GLOBALES
t_tripulante * tripulante_sabotaje;
t_tripulante * tripulante_elegido;
t_sabotaje posicion_sabotaje;
sem_t sem_mutex_ingreso_tripulantes_new;
sem_t sem_mutex_ejecutar_dispatcher;


// Revisar semaforos
sem_t sem_sabotaje_activado;
sem_t sem_hay_evento_planificable;
sem_t sem_sabotaje_mutex;
sem_t sem_sabotaje_finalizado;
sem_t sem_sabotaje_tripulante;
sem_t sem_tripulante_disponible;

int grado_multiproc;
int quantum;

// en ESTADO NEW: genera las estructuras administrativas , una vez generado todo --> pasa a READY.
t_queue **cola;
sem_t ** mutex_cola;

enum status_planificador estado_planificador;
enum status_discordiador status_discordiador;
enum algoritmo algoritmo_planificacion;

// Lista global de tripulantes (sirve para pausar/reanudar la planificacion)
t_list* lista_tripulantes;
int sabotaje_activo;


pthread_t hilo_dispatcher;

#endif
