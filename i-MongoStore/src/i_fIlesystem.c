#include "i_filesystem.h"

//funcion inicial del filesystem
void iniciar_filesystem() {
	verificar_punto_montaje();
	if(!existe_filesystem()){
		crear_filesystem();
		crear_directorios();
		printf("no existe fss\n");
	}
	mapear_blocks();
	

}

void existe_directorio(char * path){
	DIR* dir = opendir(path);
	if (dir) {
		//existe directorio
		closedir(dir);
		printf("existe directorio: %s\n",path);
	}else{
		//no existe y lo creo
		mkdir(path, 0777);
		printf("no existia y cree el directorio: %s\n",path);
	}
}

void verificar_punto_montaje(){
	char* punto_montaje = string_new();
	char** directorios = string_split(fs_config.punto_montaje,"/");
	printf("primer directorio : %s\n",directorios[1]);
	string_append(&punto_montaje,"/");
	for(int i = 1; directorios[i]!=NULL;i++){
		string_append(&punto_montaje,directorios[i]);
		existe_directorio(punto_montaje);
		string_append(&punto_montaje,"/");
	}
}

void mapear_blocks(){
	char * pathBlocks = crear_path_absoluto("/Blocks.ims");
	int blocks = open(pathBlocks, O_RDWR, S_IRUSR | S_IWUSR);
	if(fstat(blocks,&blocks_stat) == -1)
	{
		perror("no se pudo obtener los datos del archivo.\n");
	}
	//printf("el tamaño del archivo es de %d\n", (int)blocks_stat.st_size);

	blocksmap = mmap (NULL, blocks_stat.st_size, PROT_READ | PROT_WRITE ,MAP_SHARED, blocks, 0 );

	//test de grabado en mapeo
	/*char * test = "se inicializo el tripulante 10";
	memcpy(archivo_en_memoria, test, strlen(test));
	memcpy(archivo_en_memoria+super_bloque.blocksize,test,strlen(test));
	char * pointer = archivo_en_memoria+super_bloque.blocksize;
	printf("pointer = %s\n", pointer);
	msync(archivo_en_memoria, blocks_stat.st_size, MS_SYNC);
	*/

	//test de lectura (lee hasta un espacio vacio)
	for (int i = 0; i<blocks_stat.st_size ; i++){
		printf("%c", blocksmap[i]);
	}
	printf("\n");
	

	//test de lectura de un bloque usando indice
	//(en practica hay que tener en cuenta el tamaño del archivo para el ultimo bloque en vez de blocksize)
	/*int indicetest = 0;
	char * contenidotest = calloc(1,super_bloque.blocksize);
	memcpy(contenidotest, archivo_en_memoria+(super_bloque.blocksize * indicetest), super_bloque.blocksize);
	printf("se leyo el bloque %d, con el contenido : %s\n",indicetest,contenidotest);
	*/
}

void crear_filesystem(){
	char * puntomontaje = crear_path_absoluto("/SuperBloque.ims");
	super_bloque.blocksize = 50;
	super_bloque.blocks = 56;
	super_bloque.bitarray = calloc(super_bloque.blocks/8, sizeof(char));
	int sizeStruct = sizeof(super_bloque.blocksize)+sizeof(super_bloque.blocks)+super_bloque.blocks/8+1;
	int sbfile = open(puntomontaje, O_RDWR | O_CREAT , S_IRUSR | S_IWUSR);
	if(fstat(sbfile,&superbloque_stat) == -1)
	{
		perror("no se pudo obtener los datos del archivo.\n");
	}
	ftruncate(sbfile, sizeStruct);
	fstat(sbfile,&superbloque_stat);
	printf("el tamaño del archivo es de %d\n", (int)superbloque_stat.st_size);
	void * archivo = mmap (NULL, superbloque_stat.st_size, PROT_READ | PROT_WRITE ,MAP_SHARED, sbfile, 0 );
	int offset = 0;
	memcpy(archivo, &super_bloque.blocksize, sizeof(super_bloque.blocksize));
	offset += sizeof(super_bloque.blocksize);
	printf("el tamaño del archivo es de %d\n", (int)superbloque_stat.st_size);
	memcpy(archivo+offset, &super_bloque.blocks, sizeof(super_bloque.blocks));
	offset += sizeof(super_bloque.blocks);
	memcpy(archivo+offset, super_bloque.bitarray, super_bloque.blocks/8+1);
	printf("el tamaño del archivo es de %d\n", (int)superbloque_stat.st_size);
	msync(archivo, superbloque_stat.st_size, MS_SYNC);
	munmap(archivo, superbloque_stat.st_size);
	close(sbfile);
	log_info(logger, "Se creo el archivo SuperBloque.ims con exito");

	//creo el blocks.
	FILE* blocks_create;
	char * block_path = crear_path_absoluto("/Blocks.ims");
	blocks_create = fopen (block_path, "w");
	uint32_t size = super_bloque.blocks * super_bloque.blocksize;
	ftruncate(fileno(blocks_create), size);
	fclose(blocks_create);
	log_info(logger, "No se encontro un archivo Blocks.ims existente y se creo uno en base a SuperBloque.ims");
			
}

void crear_directorios(){
	char * pathFiles = string_new();
	string_append(&pathFiles,fs_config.punto_montaje);
	string_append(&pathFiles, "/Files");
	mkdir(pathFiles, 0777);
	string_append(&pathFiles, "/Bitacoras");
	mkdir(pathFiles, 0777);
}

char* crear_path_absoluto(char * pathRelativo){
	char * pathAbsoluto = string_new();
	string_append(&pathAbsoluto, fs_config.punto_montaje);
	string_append(&pathAbsoluto, pathRelativo);
	return pathAbsoluto;
}

int existe_archivo(char *nombreArchivo){
	FILE* ftry;
	if((ftry = fopen(nombreArchivo, "r"))){
		fclose(ftry);
		return 1;
	}
	return 0;
}

//checkea la existencia del filesystem.
int existe_filesystem(){
	FILE* blocks_create;
	char * sb_path = crear_path_absoluto("/SuperBloque.ims");
	char * block_path = crear_path_absoluto("/Blocks.ims");
	if (existe_archivo(sb_path)){
		//intenta abrir el archivo, para confirmar su existencia
		int sbfile = open(sb_path, O_RDWR, S_IRUSR | S_IWUSR);
		if(fstat(sbfile,&superbloque_stat) == -1)
		{
			perror("no se pudo obtener los datos del archivo.\n");
		}
		log_info(logger, "Se encontro un archivo SuperBloque.ims existente");

		//mapeo SuperBloque.ims en un *void
		superbloquemap = mmap (NULL, superbloque_stat.st_size, PROT_READ | PROT_WRITE ,MAP_SHARED, sbfile, 0 );

		int offset=0;
		//parseo los datos del SuperBloque en la estructura super_bloque
		memcpy(&super_bloque.blocksize,superbloquemap,sizeof(super_bloque.blocksize));
		//printf("blocksize = %d\n", super_bloque.blocksize);
		offset = offset + sizeof(super_bloque.blocksize);
		memcpy(&super_bloque.blocks,superbloquemap+offset,sizeof(super_bloque.blocks));
		//printf("blocks = %d\n", super_bloque.blocks);
		offset = offset + sizeof(super_bloque.blocks);
		super_bloque.bitarray = calloc(super_bloque.blocks/8+1, sizeof(char*));
		//printf("tamaño bitarray %d\n", strlen(super_bloque.bitarray));
		//printf("realizo malloc de : %d\n", super_bloque.blocks/8+1);
		memcpy(super_bloque.bitarray,superbloquemap+offset,super_bloque.blocks/8+1);
		//printf("tamaño bitarray = %d\n", strlen(super_bloque.bitarray));

		//creo un t_bittarray para manipularlo con las commons
		bitmap = *bitarray_create_with_mode(super_bloque.bitarray,super_bloque.blocks/8,LSB_FIRST);

		//test sobre el bitarray
		/*int sizeBitarray = bitarray_get_max_bit(&bitmap);
		printf("size of bitarray = %d\n", sizeBitarray);
		printf("test de bitarray:");
		for(int i = 0; i<super_bloque.blocks; i++){
			int test = bitarray_test_bit(&bitmap, i);
			printf("%d",test);
		}
		printf("\n");
		int num = 0;
		int prueba = bitarray_test_bit(&bitmap, num);
		printf("test bit 1: %d\n", prueba);
		bitarray_set_bit(&bitmap, num);
		bitarray_set_bit(&bitmap, num+1);
		prueba = bitarray_test_bit(&bitmap, num);
		printf("set bit 1: %d\n", prueba);
		printf("test de bitarray:");
		for(int i = 0; i<super_bloque.blocks; i++){
			int test = bitarray_test_bit(&bitmap, i);
			printf("%d",test);
		}
		printf("\n");
		*/

		// copio al void* mapeado el contenido del bitarray para actualizar el bitmap
		memcpy(superbloquemap+offset,super_bloque.bitarray,strlen(super_bloque.bitarray)+1);
		// fuerzo un sync para que el mapeo se refleje en SuperBloque.ims
		int sync = msync(superbloquemap, superbloque_stat.st_size, MS_SYNC);
		//printf("se sincronizo:%d\n", sync);

		//compruebo la existencia de Blocks.ims
		if(existe_archivo(block_path)){
			log_info(logger, "Se encontro un archivo Blocks.ims existente");
			return 1;
		}else{
			blocks_create = fopen (block_path, "w");
			uint32_t size = super_bloque.blocks * super_bloque.blocksize;
			ftruncate(fileno(blocks_create), size);
			fclose(blocks_create);
			log_info(logger, "No se encontro un archivo Blocks.ims existente y se creo uno en base a SuperBloque.ims");
			return 1;
		}
	}else{
		log_info(logger, "No se encontro un archivo SuperBloque.ims existente");
	}
	return 0;
}


/*
//crear metadata
void crear_metadata_tallgrass(t_filesystem* fs, int blocks, int block_size) {

	char* path = string_new();

	string_append(&path, fs->mount);
	string_append(&path, "metadata/metadata.bin");

	int fd = crear_archivo(path, fs->logger);


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
	string_append(&path, ".ims");

	int fd = crear_archivo(path, logger);
	close(fd);
	free(path);
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


	crear_directorio(pto_montaje, logger);
	log_info(logger, "se crea directorio %s", pto_montaje);


	char* path = string_new();
	string_append(&path, pto_montaje);
	string_append(&path, "metadata/");
	crear_directorio(path, logger);
	log_info(logger, "se crea directorio %s", path);

	strcpy(path, pto_montaje);


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
*/