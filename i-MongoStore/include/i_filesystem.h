#ifndef I_FILESYSTEM_H_
#define I_FILESYSTEM_H_

#include <commons/bitarray.h>
#include <commons/collections/list.h>
//#include <commons/config.h>
#include <commons/log.h>
#include <commons/string.h>
#include <dirent.h>
#include <errno.h>
#include <fcntl.h>
#include<netdb.h>
#include <limits.h>
#include<signal.h>
#include <stdio.h>
#include <stdint.h>
#include <stdlib.h>
#include <string.h>
#include <sys/mman.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <unistd.h>

#include "i_mongostore.h"

//funciones del blockes
#include <commons/string.h>
#include <time.h>


#define DIRECTORIO "DIRECTORY=Y\n"
#define ARCHIVO "DIRECTORY=N\n"
#define TAMANIO_ARCHIVO "SIZE=%d\n"
#define BLOQUES "BLOCKS=[]\n"
#define ABIERTO "OPEN=Y\n"
#define CERRADO "OPEN=N\n"

//METADATA_FS
#define TAMANIO_BLOQUE "BLOCK_SIZE=%d\n"
#define BLOQUES_METADATA_FS "BLOCKS=%d\n"
#define NUMERO_MAGICO "MAGIC_NUMBER=TALL-GRASS\n"


// estructura metadata
typedef struct {
	char* directory;
	int size;
	t_list* blocks;
	char* open;
} t_metadata;

//estructura superbloque 
typedef struct {
	uint32_t blocksize;
	uint32_t blocks;
	char * bitarray;
} t_superbloque;



//estructura file
typedef struct {
	uint32_t size;
	uint32_t block_count;
	t_list*  blocks;
	char* caracter_llenado;
	char* valor_md5;
} t_file;

//estructura bitacora

typedef struct {
	uint32_t size;
	t_list* blocks;
} t_bitacora;

void verificar_punto_montaje();
void iniciar_filesystem();
char* crear_path_absoluto(char * pathRelativo);
int existe_archivo(char *nombreArchivo);
int existe_filesystem();
void mapear_blocks();
void crear_directorios();
void crear_filesystem();
void * bajada_a_disco();

t_superbloque super_bloque;
struct stat superbloque_stat;
struct stat blocks_stat;

#endif