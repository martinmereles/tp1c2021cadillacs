#ifndef SOCKETS_SERVIDOR_H_
#define SOCKETS_SERVIDOR_H_

#include <stdio.h>
#include <stdlib.h>
#include <sys/socket.h>
#include <unistd.h>
#include <netdb.h>
#include <commons/log.h>
#include <commons/collections/list.h>
#include <string.h>
#include "sockets_shared.h"

#define IP "127.0.0.1"
#define PUERTO "4444"

int iniciar_servidor(void);

t_log* logger;

#endif