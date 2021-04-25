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

int crear_conexion(char *ip, char* puerto, int *socket_cliente);
void liberar_conexion(int socket_cliente);

t_log* logger;

#endif