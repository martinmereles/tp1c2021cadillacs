#ifndef DISCORDIADOR_H_
#define DISCORDIADOR_H_

#include <pthread.h>
#include "commons/config.h"
#include <poll.h>
#include "commons/string.h"
#include "sockets_cliente.h"
#include <semaphore.h>
#include "planificador.h"

//struct para parsear tareas
typedef struct {
    char* nombre;
    char* pos_x;
    char* pos_y;
    char* duracion;
}t_tarea;

enum status_discordiador{   RUNNING,
                            END };

enum comando_discordiador{  INICIAR_PATOTA, 
                            LISTAR_TRIPULANTES,
                            EXPULSAR_TRIPULANTE, 
                            INICIAR_PLANIFICACION,
                            PAUSAR_PLANIFICACION,
                            OBTENER_BITACORA,
                            ERROR };

void leer_consola_y_procesar(int, int);
void leer_fds(int, int);
enum comando_discordiador string_to_comando_discordiador(char*);
int cantidad_argumentos(char** argumentos);
int iniciar_patota(char** argumentos, int);
int submodulo_tripulante();
int generarNuevoPID();
int generarNuevoTID();
void recibir_y_procesar_mensaje_i_mongo_store(int i_mongo_store_fd);
char* leer_proxima_tarea_mi_ram_hq(int mi_ram_hq_fd_tripulante);
int tripulante_esta_en_posicion(iniciar_tripulante_t* tripulante, t_tarea tarea);
int obtener_bitacora(char ** argumentos, int i_mongo_store_fd);
void reanudar_planificacion();

char* direccion_IP_i_Mongo_Store;
char* puerto_i_Mongo_Store;
char* direccion_IP_Mi_RAM_HQ;
char* puerto_Mi_RAM_HQ;
char* algoritmo_planificador;

//grado_multitarea = grado multiprocesamiento
int duracion_sabotaje; // expresado como cantidad en ciclos CPU
int retardo_ciclo_cpu; // tiempo de espera por c/ciclo CPU
int tripulante_a_expulsar;

enum status_discordiador status_discordiador;

int generadorPID;
int generadorTID;

sem_t sem_generador_TID;
sem_t sem_generador_PID;
sem_t sem_struct_iniciar_tripulante;

//sem_t sem_planificacion_fue_iniciada;
sem_t sem_recurso_multitarea_disponible;

// Booleano para poder pausar/reanudar la planificacion
bool planificacion_pausada;

#endif
