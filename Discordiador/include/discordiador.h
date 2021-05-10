#ifndef DISCORDIADOR_H_
#define DISCORDIADOR_H_

#include <pthread.h>
#include "commons/config.h"
#include <poll.h>
#include "commons/string.h"
#include "sockets_cliente.h"

enum comando_discordiador{  INICIAR_PATOTA, 
                            LISTAR_TRIPULANTES,
                            EXPULSAR_TRIPULANTES, 
                            INICIAR_PLANIFICACION,
                            PAUSAR_PLANIFICACION,
                            OBTENER_BITACORA,
                            ERROR };

void leer_consola_y_procesar(int);
void leer_fds(int);
enum comando_discordiador string_to_comando_discordiador(char*);
int cantidad_argumentos(char** argumentos);
int iniciar_patota(char** argumentos);
int submodulo_tripulante();

char* direccion_IP_i_Mongo_Store;
char* puerto_i_Mongo_Store;
char* direccion_IP_Mi_RAM_HQ;
char* puerto_Mi_RAM_HQ;

// Hace falta?
bool servidor_desconectado;


#endif