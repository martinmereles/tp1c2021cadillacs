#include "i_mongostore.h"



void handler(int num){
	printf("le envio la señal al discord");
	char * payload = "TEST";
	enviar_operacion(discordiador_fd,COD_MANEJAR_SABOTAJE,payload,strlen(payload)+1);
}

int main(void)
{
	primer_conexion_discordiador=1;
	//signal para sabotaje
	signal(SIGUSR1, handler);

	// inicializo semaforos
	iniciar_semaforos_fs();

	logger = log_create("./cfg/i-mongostore.log", "I-MongoStore", 1, LOG_LEVEL_DEBUG);
	
	// Leo IP y PUERTO del config
	config = config_create("./cfg/i-mongo-store.config");
	
	char* puerto_escucha = config_get_string_value(config, "PUERTO");
	char* ip = "127.0.0.1";
	leer_config();
	//Levanto/Creo el Filesystem
	iniciar_filesystem();

	int server_fd = iniciar_servidor(ip, puerto_escucha);
	log_info(logger, "I-MongoStore listo para recibir al Discordiador");

	i_mongo_store(server_fd);

	log_info(logger, "Cerrando socket servidor");
	close(server_fd);
	log_destroy(logger);
	munmap(superbloquemap, superbloque_stat.st_size);
	munmap(blocksmap, blocks_stat.st_size);
	close(sbfile);
	close(bfile);
	free(super_bloque.bitarray);
	return EXIT_SUCCESS;
}

void iniciar_semaforos_fs(){
	sem_init(&semaforo_aceptar_conexiones, 0, 0);
	sem_init(&sem_mutex_superbloque,0,1);
	sem_init(&sem_mutex_blocks,0,1);
	sem_init(&sem_mutex_oxigeno,0,1);
	sem_init(&sem_mutex_comida,0,1);
	sem_init(&sem_mutex_basura,0,1);
	sem_init(&sem_mutex_bitmap,0,1);
}

void leer_config(){
	config = config_create("./cfg/i-mongo-store.config");
	fs_config.puerto = config_get_string_value(config,"PUERTO");
	fs_config.punto_montaje = config_get_string_value(config,"PUNTO_MONTAJE");
	fs_config.tiempo_sincro = config_get_int_value(config, "TIEMPO_SINCRONIZACION");
	fs_config.posiciones_sabotaje = config_get_string_value(config, "TIEMPO_SINCRONIZACION");
}

void i_mongo_store(int servidor_fd) {
	// Declaramos variables
	status_servidor = RUNNING;
	pthread_t *hilo_atender_cliente;

	// Inicializamos pollfd
	struct pollfd pfds[2];
	pfds[0].fd = servidor_fd;	
	pfds[0].events = POLLIN;	// Avisa cuando llega un mensaje en el socket de escucha
	pfds[1].fd = 0;
	pfds[1].events = POLLIN;	// Avisa cuando llega un mensaje por consola
	int num_events;

	while(status_servidor != END){
		// Revisamos si hay algun evento
		num_events = poll(pfds, 2, 2500);

		// Si ocurrio un evento
		if(num_events != 0){
			// Si llego un mensaje en el socket de escucha
			if((pfds[0].revents & POLLIN)){
				// Creamos el hilo que se encarga de atender el cliente
				// NOTA: el struct pthread_t de cada hilo se pierde
				hilo_atender_cliente = malloc(sizeof(pthread_t));
				pthread_create(hilo_atender_cliente, NULL, (void*) atender_cliente, &servidor_fd);
				pthread_detach(*hilo_atender_cliente);
				free(hilo_atender_cliente);

				// No dejo continuar la ejecucion hasta que se haya aceptado la conexion dentro del hilo creado
				// Sino se pueden terminar creando mas hilos de los necesarios
				sem_wait(&semaforo_aceptar_conexiones);
			}
			else{			
				// Si llego un mensaje por consola
				if((pfds[1].revents & POLLIN)){
					// Leemos la consola y procesamos el mensaje
					leer_consola_y_procesar();
				}
				// Si ocurrio un evento inesperado
				else{
					log_error(logger, "Evento inesperado en los file descriptor: %s", strerror(pfds[0].revents));
					log_error(logger, "Evento inesperado en los file descriptor: %s", strerror(pfds[1].revents));
					status_servidor = END;
				}
			}
		}
	}
}

void atender_cliente(void *args){
	// Declaramos variables
	int servidor_fd = *((int*) args);
	struct sockaddr_in dir_cliente;					// auxiliar
	int tam_direccion = sizeof(struct sockaddr_in);	// auxiliar
	int cliente_fd;
	
	// Aceptamos la conexion
	cliente_fd = accept(servidor_fd, (void*) &dir_cliente, (socklen_t*) &tam_direccion);
	fcntl(cliente_fd, F_SETFL, O_NONBLOCK);
	log_info(logger, "Se conecto un cliente!");
	if(primer_conexion_discordiador){
		discordiador_fd = cliente_fd;
		primer_conexion_discordiador=0;
	}
	// Posteamos en el semaforo
	sem_post(&semaforo_aceptar_conexiones);

	// Nos comunicamos con el cliente
	comunicacion_cliente(cliente_fd);

	log_info(logger,"Cerrando socket cliente");
	close(cliente_fd);
	log_info(logger,"Hilo cliente finalizado");
}

void leer_consola_y_procesar() {
	char linea_consola[TAM_CONSOLA];
	fgets(linea_consola,TAM_CONSOLA,stdin);
	linea_consola[strlen(linea_consola)-1]='\0';	// Le saco el \n

	if(strcmp(linea_consola,"FIN") == 0){
		status_servidor = END;
		log_info(logger, "Finalizando i-Mongo-Store");
	}
}

int comunicacion_cliente(int cliente_fd) {
	bool cliente_conectado = true;

	struct pollfd pfds[1];
	pfds[0].fd = cliente_fd;	
	pfds[0].events = POLLIN;	// Avisa cuando hay alguna operacion para leer en el buffer
	int num_events;

	while(status_servidor != END && cliente_conectado) {
		// Revisamos si hay algun evento en el file descriptor del cliente
		num_events = poll(pfds, 1, 2500);

		// Si hay un evento
		if(num_events != 0){
			// Si hay un mensaje del cliente
			if(pfds[0].revents & POLLIN)
				cliente_conectado = leer_mensaje_cliente_y_procesar(cliente_fd);
			else{
				log_error(logger, "Evento inesperado en file descriptor del cliente: %s", strerror(pfds[0].revents));
				status_servidor = END;
			}	
		}
	}
	return EXIT_SUCCESS;
}

bool leer_mensaje_cliente_y_procesar(int cliente_fd){
	bool cliente_conectado = true;
	// Leo codigo de operacion
	int cod_op = recibir_operacion(cliente_fd);
	char * payload;
	
	switch(cod_op) {
		case COD_MENSAJE:
		//prueba COD_RECIBIR_TAREA
			recibir_payload_y_ejecutar(cliente_fd, loguear_mensaje);
			break;
		case COD_INICIAR_TRIPULANTE_I_MONGO_STORE:
			payload = recibir_payload(cliente_fd);
			iniciar_tripulante(payload);
			free(payload);
			break;
		case COD_OBTENER_BITACORA:
			log_info(logger,"procesando solicitud de bitacora..");
			payload = recibir_payload(cliente_fd);
			char* bitacora = leer_bitacora(payload);
			enviar_operacion(cliente_fd,COD_OBTENER_BITACORA,bitacora,strlen(bitacora)+1);
			free(bitacora);
			log_info(logger,"bitacora enviada");
			break;
		case COD_EJECUTAR_TAREA:
			recibir_payload_y_ejecutar(cliente_fd, recibir_tarea);
			break;
		case COD_TERMINAR_TAREA:
			recibir_payload_y_ejecutar(cliente_fd, terminar_tarea);
			break;
		case COD_MOVIMIENTO_TRIP:
			recibir_payload_y_ejecutar(cliente_fd, movimiento_tripulante);
			break;
		case -1:
			log_error(logger, "El cliente se desconecto.");
			cliente_conectado = false;
			break;
		default:
			log_warning(logger, "Operacion desconocida. No quieras meter la pata");
			break;
	}
	return cliente_conectado;
}
//funcion para obtener el tamanio del superbloque
int leer_tamanio_block(File* archivo){}
int tamanio_file_superbloque (File* superbloque){
	FILE* sp;
	sp= fopen("/Superbloque.ims", r);
	int tamanio = leer_tamanio_blocks;

	return tamanio

}

bool check_blocks_usados(){}
bool check_archivos(){}

void fsck(){
	void reparar_superbloque(){
		if (superbloque.blocks == tamanio_file_superbloque(/"/Superbloque.ims"))
		printf ("el tamanio de los bloques coincide");
		return 0;
		else {
			void ecribir_en_blocks(){}

		}

	}
	void reparar_bitmap(){
	if(check_blocks_usados && check_archivos){

			printf("el bitmap esta ok");
			return 0;
	}
	else {
		void corregir_blocks();
	}

	}
}