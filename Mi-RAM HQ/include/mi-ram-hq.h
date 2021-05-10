#ifndef MI_RAM_HQ_H_
#define MI_RAM_HQ_H_

#include <commons/log.h>
#include <sys/time.h>
#include <sys/types.h>
#include <unistd.h>
#include <pthread.h>
#include <sys/fcntl.h>
#include <poll.h>
#include "commons/config.h"
#include "sockets_shared.h"

enum server_status{
    RUNNING,
    END
};

void leer_consola_y_procesar();
void mi_ram_hq(int);
int comunicacion_cliente(int cliente_fd);
void atender_cliente(void *args);

int status_servidor;

#endif