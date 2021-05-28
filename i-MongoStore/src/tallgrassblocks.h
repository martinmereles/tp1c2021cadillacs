#include <commons/collections/list.h>
#include <commons/config.h>
#include <commons/string.h>
#include <commons/bitarray.h>
#include <commons/log.h>
#include <stdio.h>
#include <stdint.h>
#include <fcntl.h>
#include <stdlib.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <unistd.h>
#include <string.h>

#include <fcntl.h>
#include <dirent.h>
#include <limits.h>
#include <string.h>
#include <sys/mman.h>
#include <errno.h>


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
	uint32_t block_size;
	uint32_t block_count;
	char* mount;
	t_bitarray* bitmap;
	t_log* log_tallgrass;
} t_superbloque;

//estructura file
typedef struct {
	uint32_t size;
	uint32_t block_count;
	t_list*  blocks;
	char* caracter_llenado;
	char* valor_md5;
} t_file

//estructura bitacora

typedef struct {
	uint32_t size
	t_list* blocks
} t_bitacora

typedef struct {
	char* mount;
	t_log* logger;
	t_superbloque* blocks;
} t_filesystem;


// FUNCIONES TALL-GRASS
t_filesystem* iniciar_filesystem(char* mount, int blocks,
		int block_size, t_log* logger);