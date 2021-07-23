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
#include <sys/signalfd.h>

//estructura archivo config
typedef struct {
	char * puerto;
	char * punto_montaje;
	int tiempo_sincro;
    char* posiciones_sabotaje;
}filesystem_config;

typedef struct {
	uint32_t blocks;
	uint32_t blocksize;
}superbloque_config;

enum server_status{
    RUNNING,
    END
};

void leer_consola_y_procesar();
int i_mongo_store(int);
int comunicacion_cliente(int cliente_fd);
void atender_cliente(void *args);
bool leer_mensaje_cliente_y_procesar(int);
void leer_config();
void iniciar_semaforos_fs();
void handler(int num);
void liberar_recursos();

int primer_conexion_discordiador;
int discordiador_fd;
int status_servidor;
sem_t semaforo_aceptar_conexiones;
t_config *config_general;
t_config *config_test;
// t_config *config;
// t_config *config_superbloque;
filesystem_config fs_config;
superbloque_config sb_config;
t_bitarray bitmap;
void* superbloquemap;
char * blocksmap;
char * blocksmapdisco;

int sbfile;
int bfile;
char ** posiciones_sabotaje;
int num_sabotaje;
sem_t sem_mutex_superbloque;
sem_t sem_mutex_blocks;
sem_t sem_mutex_oxigeno;
sem_t sem_mutex_comida;
sem_t sem_mutex_basura;
sem_t sem_mutex_bitmap;

#endif