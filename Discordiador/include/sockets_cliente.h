#ifndef SOCKETS_CLIENTE_H_
#define SOCKETS_CLIENTE_H_

#include <stdio.h>
#include <stdlib.h>
#include <signal.h>
#include <unistd.h>
#include <sys/socket.h>
#include <netdb.h>
#include <string.h>

#include "sockets_shared.h"

typedef struct{
    int PID;
    int posicion_X;
    int posicion_Y;            
    int TID;                                     
} iniciar_tripulante_t;

int crear_conexion(char *ip, char* puerto, int *socket_cliente);
void liberar_conexion(int socket_cliente);

int enviar_op_iniciar_patota(int mi_ram_hq_fd, int PID, char* lista_de_tareas);
int enviar_op_iniciar_tripulante(int mi_ram_hq_fd, iniciar_tripulante_t struct_iniciar_tripulante);

t_log* logger;

#endif