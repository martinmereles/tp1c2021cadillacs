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
typedef enum
{
	MENSAJE,
    FIN_COMUNICACION
}op_code;

int recibir_operacion(int servidor_fd);
void* recibir_payload(int socket_emisor);
char* recibir_mensaje(int socket_cliente);
int enviar_fin_comunicacion(int socket_destino);
int enviar_mensaje(int socket_destino, char* mensaje);
int enviar_operacion(int fd_destino, int codigo_operacion, char* payload);
int iniciar_servidor(char*, char*);

#endif