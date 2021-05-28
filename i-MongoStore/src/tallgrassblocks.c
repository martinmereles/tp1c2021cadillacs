#include "tallgrassblocks.h"

//funciones del blockes

#include <commons/string.h>
#include <time.h>

//funcion inicial del filesystem
t_filesystem* iniciar_filesystem(char* mount, int block, int block_size,
		t_log* logger) {
	crear_directorios(mount, logger);
	crear_bloques(mount, block, logger);
	t_bitarray* bitmap = crear_bitmap(mount, (block / 8), logger);

	t_superbloque* blocks = malloc(sizeof(t_tallgrass_blocks));
	t_filesystem* fs = malloc(sizeof(t_filesystem));

	fs->logger = logger;
	fs->mount = strdup(mount);
	fs->blocks = blocks;

	blocks->mount = strdup(mount);
	blocks->block_count = block;
	blocks->block_size = block_size;
	blocks->bitmap = bitmap;
	blocks->log_tallgrass = logger;

	crear_metadata_tallgrass(fs, block, block_size);
	banner_inicio(block, block_size);

	return fs;
}
//crear metadata
void crear_metadata_tallgrass(t_filesystem* fs, int blocks, int block_size) {

	char* path = string_new();

	string_append(&path, fs->mount);
	string_append(&path, "metadata/metadata.bin");

	int fd = crear_archivo(path, fs->logger);

	/* copio la informacion del metadata */
	dprintf(fd, TAMANIO_BLOQUE, block_size);
	dprintf(fd, BLOQUES_METADATA_FS, blocks);
	dprintf(fd, NUMERO_MAGICO);

	log_info(fs->logger, "Se crea metadata Filesystem");

	close(fd);
	free(path);
}
//crear un bloque
void crear_un_bloque(char* pto_montaje, uint32_t indice, t_log* logger) {

	char* path = string_new();
	string_append(&path, pto_montaje);
	string_append(&path, "blocks/");
	char* numero = string_itoa(indice);
	string_append(&path, numero);
	string_append(&path, ".bin");

	int fd = crear_archivo(path, logger);
	close(fd);
	free(path);
}

// crear bloques de datos
void crear_bloques(char* pto_montaje, uint32_t cantidad, t_log* logger) {
	for (int i = 0; i < cantidad; i++) {
		crear_un_bloque(pto_montaje, i, logger);
	}
	log_info(logger, "Se crean %d bloques de datos", cantidad);
}

// crear bitmap
t_bitarray* crear_bitmap(char* pto_montaje, uint32_t size, t_log* logger) {
	char* path = string_new();
	string_append(&path, pto_montaje);
	string_append(&path, "metadata/bitmap.bin");

	int fd = crear_archivo(path, logger);

	ftruncate(fd, size);

	void* bmap = mmap(NULL, size, PROT_READ | PROT_WRITE, MAP_SHARED, fd, 0);

	if (bmap == MAP_FAILED) {
		perror("mmap");
		close(fd);
		exit(1);
	}

	bitmap = bitarray_create_with_mode((char*) bmap, size, LSB_FIRST);

	log_info(logger, "Se creo un bitmap de %d posiciones", size * 8);

	close(fd);

	return bitmap;
}

//DIRECTORIOS
void crear_directorios(char* pto_montaje, t_log* logger) {

	/* Se crea tall-grass/ */
	crear_directorio(pto_montaje, logger);
	log_info(logger, "se crea directorio %s", pto_montaje);

	/* se crea tall-grass/metadata/ */
	char* path = string_new();
	string_append(&path, pto_montaje);
	string_append(&path, "metadata/");
	crear_directorio(path, logger);
	log_info(logger, "se crea directorio %s", path);

	strcpy(path, pto_montaje);

	/* se crea tall-grass/blocks/ */
	string_append(&path, "blocks/");
	crear_directorio(path, logger);
	log_info(logger, "se crea directorio %s", path);

	strcpy(path, pto_montaje);

	free(path);
}

void crear_directorio(char* path_directorio, t_log* logger) {
	string_to_lower(path_directorio);
	int res = mkdir(path_directorio, 0775);
	if (res == -1) {
		log_error(logger, "Error al crear directorio %s", path_directorio);
		exit(1);
	}
}