#include "i_filesystem.h"

//funcion inicial del filesystem
void iniciar_filesystem() {
	verificar_punto_montaje();
	if(!existe_filesystem()){
		crear_filesystem();
		crear_directorios();
		
	}
	mapear_blocks();
	printf("creando hilo\n");

}

void * bajada_a_disco(void * arg){
	while(1){
		sleep(fs_config.tiempo_sincro);
		log_debug(logger,"Bajando datos a disco...");
		msync(blocksmap, blocks_stat.st_size, MS_SYNC);
	}
	return NULL;
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
	char * punto_montaje = string_new();
	char ** directorios = string_split(fs_config.punto_montaje,"/");
	printf("primer directorio : %s\n",directorios[1]);
	string_append(&punto_montaje,"/");
	for(int i = 1; directorios[i]!=NULL;i++){
		string_append(&punto_montaje,directorios[i]);
		existe_directorio(punto_montaje);
		string_append(&punto_montaje,"/");
	}
	liberar_char_array(directorios);
	free(punto_montaje);
}

void mapear_blocks(){
	char * pathBlocks = crear_path_absoluto("/Blocks.ims");
	bfile = open(pathBlocks, O_RDWR, S_IRUSR | S_IWUSR);
	if(fstat(bfile,&blocks_stat) == -1)
	{
		perror("no se pudo obtener los datos del archivo.\n");
	}
	//printf("el tamaño del archivo es de %d\n", (int)blocks_stat.st_size);
	sem_wait(&sem_mutex_blocks);
	blocksmap = mmap (NULL, blocks_stat.st_size, PROT_READ | PROT_WRITE ,MAP_SHARED, bfile, 0 );
	sem_post(&sem_mutex_blocks);
	//test de grabado en mapeo
	/*char * test = "se inicializo el tripulante 10";
	memcpy(archivo_en_memoria, test, strlen(test));
	memcpy(archivo_en_memoria+super_bloque.blocksize,test,strlen(test));
	char * pointer = archivo_en_memoria+super_bloque.blocksize;
	printf("pointer = %s\n", pointer);
	msync(archivo_en_memoria, blocks_stat.st_size, MS_SYNC);
	*/

	//test de lectura (lee hasta un espacio vacio)
	/*for (int i = 0; i<blocks_stat.st_size ; i++){
		printf("%c", blocksmap[i]);
	}
	printf("\n");*/
	

	//test de lectura de un bloque usando indice
	//(en practica hay que tener en cuenta el tamaño del archivo para el ultimo bloque en vez de blocksize)
	int indicetest = 3;
	char * contenidotest = calloc(1,super_bloque.blocksize);
	memcpy(contenidotest, blocksmap+(super_bloque.blocksize * indicetest), super_bloque.blocksize);
	printf("se leyo el bloque %d, con el contenido : %s\n",indicetest,contenidotest);
	free(contenidotest);
	free(pathBlocks);
	
}

void crear_filesystem(){
	char * puntomontaje = crear_path_absoluto("/SuperBloque.ims");
	super_bloque.blocksize = sb_config.blocksize;
	super_bloque.blocks = sb_config.blocks;
	printf("blocks :%d, blocksize:%d\n", super_bloque.blocks, super_bloque.blocksize);
	/*super_bloque.blocksize = 50;
	super_bloque.blocks = 500;*/
	super_bloque.bitarray = calloc(super_bloque.blocks/8, sizeof(char));
	int sizeStruct = sizeof(super_bloque.blocksize)+sizeof(super_bloque.blocks)+super_bloque.blocks/8+1;
	sbfile = open(puntomontaje, O_RDWR | O_CREAT , S_IRUSR | S_IWUSR);
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
	log_info(logger, "Se creo el archivo SuperBloque.ims con exito");

	//mapeo SuperBloque.ims en un *void
	sem_wait(&sem_mutex_superbloque);
	superbloquemap = mmap (NULL, superbloque_stat.st_size, PROT_READ | PROT_WRITE ,MAP_SHARED, sbfile, 0 );
	int offsetmapeo=0;
	//parseo los datos del SuperBloque en la estructura super_bloque
	memcpy(&super_bloque.blocksize,superbloquemap,sizeof(super_bloque.blocksize));
	offsetmapeo = offsetmapeo + sizeof(super_bloque.blocksize);
	memcpy(&super_bloque.blocks,superbloquemap+offsetmapeo,sizeof(super_bloque.blocks));
	offsetmapeo = offsetmapeo + sizeof(super_bloque.blocks);
	super_bloque.bitarray = calloc(super_bloque.blocks/8+1, sizeof(char*));
	memcpy(super_bloque.bitarray,superbloquemap+offsetmapeo,super_bloque.blocks/8+1);
	sem_post(&sem_mutex_superbloque);

	//creo un t_bittarray para manipularlo con las commons
	sem_wait(&sem_mutex_bitmap);
	bitmap = *bitarray_create_with_mode(super_bloque.bitarray,super_bloque.blocks/8,LSB_FIRST);
	sem_post(&sem_mutex_bitmap);
	//creo el blocks.
	FILE* blocks_create;
	char * block_path = crear_path_absoluto("/Blocks.ims");
	blocks_create = fopen (block_path, "w");
	uint32_t size = super_bloque.blocks * super_bloque.blocksize;
	ftruncate(fileno(blocks_create), size);
	fclose(blocks_create);
	log_info(logger, "No se encontro un archivo Blocks.ims existente y se creo uno en base a SuperBloque.ims");
	free(puntomontaje);
	free(block_path);
}

void crear_directorios(){
	char * pathFiles = string_new();
	string_append(&pathFiles,fs_config.punto_montaje);
	string_append(&pathFiles, "/Files");
	mkdir(pathFiles, 0777);
	string_append(&pathFiles, "/Bitacoras");
	mkdir(pathFiles, 0777);
	free(pathFiles);
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
		sbfile = open(sb_path, O_RDWR, S_IRUSR | S_IWUSR);
		if(fstat(sbfile,&superbloque_stat) == -1)
		{
			perror("no se pudo obtener los datos del archivo.\n");
		}
		log_info(logger, "Se encontro un archivo SuperBloque.ims existente");

		//mapeo SuperBloque.ims en un *void
		sem_wait(&sem_mutex_superbloque);
		superbloquemap = mmap (NULL, superbloque_stat.st_size, PROT_READ | PROT_WRITE ,MAP_SHARED, sbfile, 0 );
		int offset=0;
		//parseo los datos del SuperBloque en la estructura super_bloque
		memcpy(&super_bloque.blocksize,superbloquemap,sizeof(super_bloque.blocksize));
		offset = offset + sizeof(super_bloque.blocksize);
		memcpy(&super_bloque.blocks,superbloquemap+offset,sizeof(super_bloque.blocks));
		offset = offset + sizeof(super_bloque.blocks);
		super_bloque.bitarray = calloc(super_bloque.blocks/8+1, sizeof(char*));
		memcpy(super_bloque.bitarray,superbloquemap+offset,super_bloque.blocks/8+1);
		sem_post(&sem_mutex_superbloque);

		//creo un t_bittarray para manipularlo con las commons
		sem_wait(&sem_mutex_bitmap);
		bitmap = *bitarray_create_with_mode(super_bloque.bitarray,super_bloque.blocks/8,LSB_FIRST);
		sem_post(&sem_mutex_bitmap);
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
		

		// (para el test)copio al void* mapeado el contenido del bitarray para actualizar el bitmap
		memcpy(superbloquemap+offset,super_bloque.bitarray,strlen(super_bloque.bitarray)+1);
		// (para el test)fuerzo un sync para que el mapeo se refleje en SuperBloque.ims
		int sync = msync(superbloquemap, superbloque_stat.st_size, MS_SYNC);
		printf("se sincronizo:%d\n", sync);
*/
		//compruebo la existencia de Blocks.ims
		if(existe_archivo(block_path)){
			log_info(logger, "Se encontro un archivo Blocks.ims existente");
			free(block_path);
			return 1;
		}else{
			blocks_create = fopen (block_path, "w");
			uint32_t size = super_bloque.blocks * super_bloque.blocksize;
			ftruncate(fileno(blocks_create), size);
			fclose(blocks_create);
			log_info(logger, "No se encontro un archivo Blocks.ims existente y se creo uno en base a SuperBloque.ims");
			free(block_path);
			return 1;
		}
	}else{
		log_info(logger, "No se encontro un archivo SuperBloque.ims existente");
	}
	free(block_path);
	free(sb_path);
	return 0;
}