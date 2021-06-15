#ifndef MI_RAM_HQ_H_
#define MI_RAM_HQ_H_

#include <commons/log.h>
#include <sys/time.h>
#include <sys/types.h>
#include <unistd.h>
#include <sys/fcntl.h>
#include <poll.h>
#include "commons/config.h"
#include "commons/temporal.h"
#include "servidor_mi_ram_hq.h"
#include "memoria_principal.h"

enum server_status{
    RUNNING,
    END
};

// Funciones
void leer_consola_y_procesar();
void mi_ram_hq(int);
int comunicacion_cliente(int cliente_fd);
void atender_cliente(void *args);
bool leer_mensaje_cliente_y_procesar(int, void**, uint32_t*);

// Variables globales
int status_servidor;

// Semaforos
sem_t semaforo_aceptar_conexiones;

#endif