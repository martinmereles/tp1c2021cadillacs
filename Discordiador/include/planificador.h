#ifndef _PLANIFICADOR_H_
#define _PLANIFICADOR_H_

#include "commons/collections/queue.h"
#include <stdbool.h>
#include "variables_globales_shared.h"
#include <stdlib.h>

// en ESTADO NEW: genera las estructuras administrativas , una vez generado todo --> pasa a READY.
t_queue *cola_new;
t_queue *cola_ready;
t_queue *cola_running;
t_queue *cola_bloqueado_io;
t_queue *cola_bloqueado_emergency;
t_queue *cola_exit;

//t_queue *index_planificador;

char* algoritmo_Planificador;

enum status_planificador estado_planificador;
// Estructura de dato para colas del planificador / dispatcher
typedef struct {
    int     PID,
            TID;
}t_key_tripulante;

typedef struct {
    t_key_tripulante key;
    //enum estado_tripulante estado_actual=NEW;
    enum estado_tripulante estado_previo;

}t_dato_tripulante_fifo;

typedef struct {
    t_key_tripulante key;
    enum estado_tripulante estado_previo;
    int quantum;
}t_dato_tripulante_rr;

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
    OFF,
    RUNNING,
    BLOCKED
};

void init_dato_tripulante(t_dato_tripulante* dato, int numTripulante, int numPatota); // TODO: se agregan a la cola new cada tripulante y patota que se genera.

void cambiar_estado_dato_tripulante(t_dato_tripulante* dato, enum estado_tripulante estado);

char* estado_tripulante_to_string(enum estado_tripulante);

void iniciar_planificacion();
int dispatcher();  
// TODO: armar colas (de new, ready, exec, blocked, exit), una vez que un tripulante se creo hacer la transicion_new_to_ready - ES ciclico hasta recibir PAUSAR_planificacion.
// TODO: LEER 1er - 2do parrafo de "Adm. de los tripulantes"
void crear_colas();

void ordenamiento_estado_ready(void *dato,char* DATO_CONFIG);// TODO:aplicar algoritmo segun marque CONFIG FILE, pasar a EXEC cuando tenga vacante - es ciclico.
void ordenamiento_ready_algoritmo_fifo(int tid); // TODO:aplicar algoritmo PRIMERO.
void ordenamiento_ready_algoritmo_rr(int tid); // TODO
void fijar_limite_cola_running(DATO_LIMITE); // TODO: armar cola de running con la cantidad limite recibida.

static void transicion_new_to_ready(void *dato); // TODO: una vez que esté creado el hilo correspondiente, asignar estado ready.
// TODO:considerar NEW aquel tripulante que no tiene hilo asociado. PREFERENTEMENTE DETENER ESE INSTANTE.
static void transicion_ready_to_running(void *dato); // TODO: pasar a sig cola, pasando tarea a ejecutar.
static void transicion_running_to_blocked(void *dato); // TODO: pasar a estado bloqueado, luego ir en detalle sobre motivo.
static void transicion_running_to_ready(void *dato); // TODO 
static void transicion_blocked_to_ready(void *dato); // TODO

static void *split_blocked_to(enum estado_tripulante TIPO_BLOQUEO); // TODO: Una vez que entra en estado BLOCKED, implementar con Switch case segun corresponda.
static void switch_cola_to(t_queue *nueva_cola,void* dato);

void pausar_planificacion(); // TODO: recibida la orden, LEER ultimo parrafo "Adm. de los tripulantes"

#endif