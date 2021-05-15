
//bibliotecas utiles
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

//estructuras basicas

typedef struct {
	char* directory;
	int size;
	t_list* blocks;
	char* open;
} t_metadata;

typedef struct {
	uint32_t block_size;
	uint32_t block_count;
	char* mount;
	t_bitarray* bitmap;
	t_log* log_tallgrass;
} t_tallgrass_Superblocke;
