#ifndef DISCORDIADOR_H_
#define DISCORDIADOR_H_

#include <readline/readline.h>
#include <pthread.h>
#include "commons/config.h"

#include "sockets_cliente.h"

int recibir_operaciones_servidor(void *args);
void mandar_mensajes(void *args);

bool servidor_desconectado;
int i_mongostore_fd;
int mi_ram_hq_fd;

#endif