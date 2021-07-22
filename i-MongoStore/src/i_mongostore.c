#include "i_mongostore.h"
#define handle_error(msg) \
		do { perror(msg); exit(EXIT_FAILURE); } while (0)

int server_fd;

void handler(int num){
	printf("le envio la señal al discord\n");
	char * payload = "4|5";
	enviar_operacion(discordiador_fd,COD_MANEJAR_SABOTAJE,payload,strlen(payload)+1);
}

int main(int argc, char *argv[])
{
	if(argc!=2) {
		printf("FALTA NOMBRE DEL CONFIG.\n");
		return EXIT_FAILURE;
	}

	char* path_config = string_new();
	string_append(&path_config, "./cfg/tests/");
	string_append(&path_config, argv[1]);
	string_append(&path_config, ".config");

	printf("EL PATH DEL CONFIG DEL TEST ES: %s\n", path_config);

	// Inicializo config	
	config_general = config_create("./cfg/config_general.config");
	config_test = config_create(path_config);
	free(path_config);

	primer_conexion_discordiador=1;
	//signal para sabotaje
	signal(SIGUSR1, handler);

	// inicializo semaforos
	iniciar_semaforos_fs();

	logger = log_create("./cfg/i-mongostore.log", "I-MongoStore", 1, LOG_LEVEL_DEBUG);
	
	// Leo IP y PUERTO del config y datos base de SUPERBLOQUE
	char* ip = "127.0.0.1";
	leer_config();
	//Levanto/Creo el Filesystem
	iniciar_filesystem();

	server_fd = iniciar_servidor(ip, fs_config.puerto);
	log_info(logger, "I-MongoStore listo para recibir al Discordiador");

	i_mongo_store(server_fd);

	log_info(logger, "Cerrando socket servidor");
	liberar_recursos();
	
	return EXIT_SUCCESS;
}

void liberar_recursos(){
	munmap(superbloquemap, superbloque_stat.st_size);
	munmap(blocksmap, blocks_stat.st_size);
	bitarray_destroy(&bitmap);
	free(super_bloque.bitarray);
	//config_destroy(config);
	//config_destroy(config_superbloque);
	config_destroy(config_test);
	config_destroy(config_general);
	log_destroy(logger);
	close(server_fd);
	close(sbfile);
	close(bfile);
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
	fs_config.puerto = config_get_string_value(config_general,"PUERTO");
	fs_config.punto_montaje = config_get_string_value(config_general,"PUNTO_MONTAJE");
	fs_config.tiempo_sincro = config_get_int_value(config_test, "TIEMPO_SINCRONIZACION");
	fs_config.posiciones_sabotaje = config_get_string_value(config_test, "POSICIONES_SABOTAJE");
	// config_superbloque = config_create("./cfg/superbloque.config");
	sb_config.blocks = config_get_int_value(config_test,"BLOCKS");
	sb_config.blocksize = config_get_int_value(config_test,"BLOCK_SIZE");
}

void sighandler(int signum) {
    (void)signum;
}

int i_mongo_store(int servidor_fd) {
	// Declaramos variables
	status_servidor = RUNNING;
	pthread_t *hilo_atender_cliente;
	//
	sigset_t mask;
	int sfd;
	struct signalfd_siginfo fdsi;
	ssize_t s;

	sigemptyset(&mask);
	sigaddset(&mask, SIGUSR1);
	sigaddset(&mask, SIGQUIT);

	/* Block signals so that they aren't handled
		according to their default dispositions. */

	sfd = signalfd(-1, &mask, 0);

	// Inicializamos pollfd
	struct pollfd pfds[3];
	pfds[0].fd = servidor_fd;	
	pfds[0].events = POLLIN;	// Avisa cuando llega un mensaje en el socket de escucha
	pfds[1].fd = 0;
	pfds[1].events = POLLIN;	// Avisa cuando llega un mensaje por consola
	pfds[2].fd = sfd;
    pfds[2].events = POLLIN;

	int num_events;

	while(status_servidor != END){
		// Revisamos si hay algun evento
		num_events = poll(pfds, 3, 2500);
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
					if((pfds[2].revents & POLLIN)){
						if (read(sfd, &fdsi, sizeof(fdsi)) != sizeof(fdsi)) {
							handle_error("read siginfo");
						}
						break;
					}else{
						log_error(logger, "Evento inesperado en los file descriptor: %s", strerror(pfds[0].revents));
						log_error(logger, "Evento inesperado en los file descriptor: %s", strerror(pfds[1].revents));
						status_servidor = END;	
					}
				}
			}
		}
	}
	return 0;
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
	if(strcmp(linea_consola,"test") == 0){
		testear_md5();
		log_info(logger, "Finalizando test");
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
			free(payload);
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
		case COD_MANEJAR_SABOTAJE:;
			char * payload = recibir_payload(cliente_fd);
			printf("el trip %s intentara resolver el sabotaje\n",payload);
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