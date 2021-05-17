#ifndef _PLANIFICADOR_H_
#define _PLANIFICADOR_H_

#include "commons/collections/queue.h"
#include <stdbool.h>
#include "variables_globales_shared"

// en ESTADO NEW: genera las estructuras administrativas , una vez generado todo --> pasa a READY.
t_queue *cola_new;
t_queue *cola_ready;
t_queue *cola_running;
t_queue *cola_bloqueado_io;
t_queue *cola_bloqueado_emergency;

// Estructura de dato para colas del planificador / dispatcher
typedef struct {
    int     PID,
            TID;
    estado_tripulante estado_actual = NEW;
    estado_tripulante estado_previo;
}t_dato_tripulante;

// CODIGO ESTADO del Tripulante
enum estado_tripulante{
    NEW,
    READY,
    RUNNING,
    BLOCKED_IO,
    BLOCKED_EMERGENCY,
    EXIT
};

void init_dato_tripulante(t_dato_tripulante* dato, int numTripulante, int numPatota); // TODO: se agregan a la cola new cada tripulante y patota que se genera.

void cambiar_estado_dato_tripulante(t_dato_tripulante* dato, estado_tripulante estado);

char* estado_tripulante_to_string(enum estado_tripulante);

void iniciar_planificacion();
void dispatcher(char*);  
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
static void trasicion_blocked_to_ready(void *dato); // TODO

static void *split_blocked_to(enum estado_tripulante TIPO_BLOQUEO); // TODO: Una vez que entra en estado BLOCKED, implementar con Switch case segun corresponda.
static void switch_cola_to(t_queue *nueva_cola,void* dato,t_queue *cola_origen); //TODO

void pausar_planificacion(); // TODO: recibida la orden, LEER ultimo parrafo "Adm. de los tripulantes"

#endif