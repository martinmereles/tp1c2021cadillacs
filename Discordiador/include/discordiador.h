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
    char* string;
    char* nombre;
    char* parametro;
    int pos_x;    
    int pos_y;
    int duracion;
    bool es_bloqueante;
}t_tarea;



typedef struct {
	pthread_t hilo;
} ciclo_t;

enum comando_discordiador{  INICIAR_PATOTA, 
                            LISTAR_TRIPULANTES,
                            EXPULSAR_TRIPULANTE, 
                            INICIAR_PLANIFICACION,
                            PAUSAR_PLANIFICACION,
                            OBTENER_BITACORA,
                            FINALIZAR,
                            ERROR };


// PROTOTIPOS DE FUNCIONES
void leer_consola_y_procesar(int, int);
void leer_fds(int, int);
enum comando_discordiador string_to_comando_discordiador(char*);
int cantidad_argumentos(char** argumentos);
int iniciar_patota(char** argumentos, int);
int expulsar_tripulante(int argumentos);
int submodulo_tripulante();
int generarNuevoPID();
int generarNuevoTID();
void recibir_y_procesar_mensaje_i_mongo_store(int i_mongo_store_fd);
void leer_ubicacion_tripulante_mi_ram_hq(int mi_ram_hq_fd_tripulante, int* posicion_X, int* posicion_Y);
bool tripulante_esta_en_posicion(t_tripulante* tripulante, t_tarea* tarea, int mi_ram_hq_fd_tripulante, int i_mongo_store_fd_tripulante);
int obtener_bitacora(char ** argumentos, int i_mongo_store_fd);
bool planificacion_pausada();
void tripulante_mas_cercano(void *data);
void reanudar_planificacion();
t_tarea* leer_proxima_tarea_mi_ram_hq(int mi_ram_hq_fd_tripulante);
char leer_estado_mi_ram_hq(int mi_ram_hq_fd_tripulante);
void leer_estado_tripulante_mi_ram_hq(int mi_ram_hq_fd_tripulante, t_tripulante* tripulante);
t_tarea* obtener_proxima_tarea(int mi_ram_hq_fd_tripulante, t_tripulante* tripulante, int *ciclos_ejecutando_tarea);

// Funciones para crear/destruir una tarea
t_tarea* crear_tarea(char* string);
void destruir_tarea(t_tarea* tarea);

// Funciones ciclo cpu
ciclo_t* crear_ciclo_cpu();
void esperar_retardo_ciclo_cpu();
void esperar_fin_ciclo_de_cpu(ciclo_t* ciclo);

// VARIABLES GLOBALES

char* direccion_IP_i_Mongo_Store;
char* puerto_i_Mongo_Store;
char* direccion_IP_Mi_RAM_HQ;
char* puerto_Mi_RAM_HQ;
char* path_tareas;

//grado_multitarea = grado multiprocesamiento
int duracion_sabotaje; // expresado como cantidad en ciclos CPU

int generadorPID;
int generadorTID;

sem_t sem_generador_TID;
sem_t sem_generador_PID;
sem_t sem_struct_iniciar_tripulante;

#endif
