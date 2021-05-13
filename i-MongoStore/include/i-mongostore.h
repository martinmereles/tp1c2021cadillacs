#ifndef IMONGOSTORE_H_
#define IMONGOSTORE_H_

#include <commons/log.h>
#include <sys/time.h>
#include <sys/types.h>
#include <unistd.h>
#include <pthread.h>
#include <sys/fcntl.h>
#include <poll.h>
#include "commons/config.h"
#include "sockets_shared.h"
#include <semaphore.h>
#include "servidor_i_mongo_store.h"

enum server_status{
    RUNNING,
    END
};

void leer_consola_y_procesar();
void i_mongo_store(int);
int comunicacion_cliente(int cliente_fd);
void atender_cliente(void *args);
bool leer_mensaje_cliente_y_procesar(int);

int status_servidor;
sem_t semaforo_aceptar_conexiones;

#endif