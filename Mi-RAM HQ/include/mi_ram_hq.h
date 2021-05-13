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
#include <semaphore.h>
#include "servidor_mi_ram_hq.h"

enum server_status{
    RUNNING,
    END
};

// Una tabla de segmentos es un array de filas
/*
direccion logica = nro segmento | desplazamiento

nro segmento: 1 bit
desplazamiento: 31 bits
*/

typedef struct{
    int numero_segmento;
    int inicio;
    uint32_t tamanio;
} fila_tabla_segmentos_t;

// Mapa de posiciones de memoria libres (segmentacion)



void leer_consola_y_procesar();
void mi_ram_hq(int);
int comunicacion_cliente(int cliente_fd);
void atender_cliente(void *args);
bool leer_mensaje_cliente_y_procesar(int);

int status_servidor;

void* memoria_principal;

sem_t semaforo_aceptar_conexiones;

#endif