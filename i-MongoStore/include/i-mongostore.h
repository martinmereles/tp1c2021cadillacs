#ifndef IMONGOSTORE_H_
#define IMONGOSTORE_H_

#include <commons/log.h>
#include <sys/time.h>
#include <sys/types.h>
#include <unistd.h>
#include <pthread.h>
#include <readline/readline.h>
#include <sys/fcntl.h>
#include <poll.h>
#include "sockets_servidor.h"

enum server_status{
    RUNNING,
    END
};

void leer_consola_servidor(void* args);
void recibir_discordiador(void *args);
int recibir_operaciones_discordiador(int cliente_fd);

int status_servidor;

#endif