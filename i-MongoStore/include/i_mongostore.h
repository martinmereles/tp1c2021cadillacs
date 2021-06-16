#ifndef I_MONGOSTORE_H_
#define I_MONGOSTORE_H_

#include <commons/log.h>
#include <sys/time.h>
#include <sys/types.h>
#include <unistd.h>
#include <pthread.h>
#include <sys/fcntl.h>
#include <poll.h>
#include <commons/config.h>
#include <commons/string.h>
#include "sockets_shared.h"
#include <semaphore.h>
#include "servidor_i_mongo_store.h"
#include "i_filesystem.h"

//estructura archivo config
typedef struct {
	char * puerto;
	char * punto_montaje;
	int tiempo_sincro;
    char* posiciones_sabotaje;
}filesystem_config;



enum server_status{
    RUNNING,
    END
};

void leer_consola_y_procesar();
void i_mongo_store(int);
int comunicacion_cliente(int cliente_fd);
void atender_cliente(void *args);
bool leer_mensaje_cliente_y_procesar(int);
void leer_config();

int status_servidor;
sem_t semaforo_aceptar_conexiones;
t_config *config;
filesystem_config fs_config;
t_bitarray bitmap;
void* superbloquemap;
char * blocksmap;
extern int bitoffset;

#endif