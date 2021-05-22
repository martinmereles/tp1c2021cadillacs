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


#define DIRECTORIO "DIRECTORY=Y\n"
#define ARCHIVO "DIRECTORY=N\n"
#define TAMANIO_ARCHIVO "SIZE=%d\n"
#define BLOQUES "BLOCKS=[]\n"
#define ABIERTO "OPEN=Y\n"
#define CERRADO "OPEN=N\n"
// estructura metadata
typedef struct {
	char* directory;
	int size;
	t_list* blocks;
	char* open;
} t_metadata;

//estructura superblocke 
typedef struct {
	uint32_t block_size;
	uint32_t block_count;
	char* mount;
	t_bitarray* bitmap;
	t_log* log_tallgrass;
} t_tallgrass_blocks;

//estructura file
typedef struct {
	uint32_t size;
	uint32_t block_count;
	t_list*  blocks
	char* valor_md5 
}
