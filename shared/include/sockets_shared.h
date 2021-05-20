#ifndef SOCKETS_SHARED_H_
#define SOCKETS_SHARED_H_

#include <stdbool.h>
#include <stdio.h>
#include <stdlib.h>
#include <signal.h>
#include <unistd.h>
#include <sys/socket.h>
#include <netdb.h>
#include <string.h>
#include "variables_globales_shared.h"
#include <commons/collections/list.h>



// PROTOCOLO: CODIGOS DE OPERACION 
enum op_code{
	COD_MENSAJE,
    // Codigos para servidor Mi-RAM HQ
    COD_INICIAR_PATOTA,
    COD_INICIAR_TRIPULANTE,
    COD_RECIBIR_UBICACION_TRIPULANTE,
    COD_ENVIAR_PROXIMA_TAREA,
    COD_EXPULSAR_TRIPULANTE
    // Codigos para servidor i-Mongo-Store: FALTA
};

int recibir_operacion(int servidor_fd);
char* recibir_payload(int socket_emisor);
char* recibir_mensaje(int socket_cliente);
int enviar_fin_comunicacion(int socket_destino);
int enviar_mensaje(int socket_destino, char* mensaje);
int enviar_operacion(int fd_destino, int codigo_operacion, void* payload, int longitud_payload);
int iniciar_servidor(char*, char*);
void recibir_payload_y_ejecutar(int socket_fd, void(*funcion_a_ejecutar)(char*));

#endif