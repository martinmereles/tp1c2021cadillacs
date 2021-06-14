#include "mi_ram_hq.h"

int main(void)
{
	// inicializo semaforos
	sem_init(&semaforo_aceptar_conexiones, 0, 0);

	logger = log_create("./cfg/mi-ram-hq.log", "Mi-RAM HQ", 1, LOG_LEVEL_DEBUG);

	// Leo IP y PUERTO del config
	t_config *config = config_create("./cfg/mi-ram-hq.config");
	char* puerto_escucha = config_get_string_value(config, "PUERTO");
	char* ip = "127.0.0.1";

	// Inicializar las estructuras para administrar la memoria
	if(inicializar_estructuras_memoria(config)==EXIT_FAILURE)
		return EXIT_FAILURE;

	int server_fd = iniciar_servidor(ip, puerto_escucha);

	log_info(logger, "Mi-RAM HQ listo para recibir al Discordiador");
	log_info(logger, "PID de Mi-RAM-HQ: %d",getpid());

	mi_ram_hq(server_fd);


	log_info(logger, "Cerrando socket servidor");
	close(server_fd);
	liberar_estructuras_memoria();
	log_destroy(logger);

	return EXIT_SUCCESS;
}

void mi_ram_hq(int servidor_fd) {
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
					if(strcmp(strerror(pfds[0].revents),"Success")!=0)
						log_error(logger, "Evento inesperado en los file descriptor: %s", strerror(pfds[0].revents));
					if(strcmp(strerror(pfds[1].revents),"Success")!=0)
						log_error(logger, "Evento inesperado en los file descriptor: %s", strerror(pfds[1].revents));
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

	if(strcmp(linea_consola,"DUMP") == 0)
		dump_memoria();

	if(strcmp(linea_consola,"COMP") == 0)
		compactacion();

	if(strcmp(linea_consola,"FIN") == 0){
		status_servidor = END;
		log_info(logger, "Finalizando Mi-RAM HQ");
	}
}

// Funcion que ejecutan los hilos una vez establecida la conexion con un tripulante
// Deberian ser dos versiones? una para segmentacion y otra para paginacion?
int comunicacion_cliente(int cliente_fd) {
	void* tabla_patota = NULL;
	uint32_t dir_log = -1;
	bool cliente_conectado = true;

	struct pollfd pfds[1];
	pfds[0].fd = cliente_fd;	
	pfds[0].events = POLLIN;	// Avisa cuando hay alguna operacion para leer en el buffer
	int num_events;

	// Inicializamos semaforo (para compactacion)
	sem_t* puede_atender_peticion = malloc(sizeof(sem_t));
	sem_init(puede_atender_peticion, 0, 1);
	int indice_sem = list_add(lista_sem_puede_atender_peticion, puede_atender_peticion);

	while(status_servidor != END && cliente_conectado) {
		// Revisamos si hay algun evento en el file descriptor del cliente
		num_events = poll(pfds, 1, 2500);
		
		// Si hay un evento
		if(num_events != 0){
			// Si hay un mensaje del cliente
			if(pfds[0].revents & POLLIN){
				sem_wait(puede_atender_peticion);	// Atendemos peticion
				cliente_conectado = leer_mensaje_cliente_y_procesar(cliente_fd, &tabla_patota, &dir_log);
				sem_post(puede_atender_peticion);
			}
			else
				log_error(logger, "Evento inesperado en file descriptor del cliente: %s", strerror(pfds[0].revents));
		}
	}

	// Liberamos el semaforo y lo sacamos de la lista
	list_remove_and_destroy_element(lista_sem_puede_atender_peticion, indice_sem, free);

	return EXIT_SUCCESS;
}

// Cada tripulante sabe a que segmento pertenece y a que proceso pertenece

bool leer_mensaje_cliente_y_procesar(int cliente_fd, void** tabla_patota, uint32_t* dir_log){
	bool cliente_conectado = true;
	// Leo codigo de operacion
	int cod_op = recibir_operacion(cliente_fd);
	char* payload;

	switch(cod_op) {
		case COD_MENSAJE:
			recibir_payload_y_ejecutar(cliente_fd, loguear_mensaje);
			break;
		case COD_INICIAR_PATOTA:
			recibir_payload_y_ejecutar(cliente_fd, iniciar_patota);
			break;
		case COD_INICIAR_TRIPULANTE:
			payload = recibir_payload(cliente_fd);
			iniciar_tripulante(payload, tabla_patota, dir_log);
			free(payload);
			break;
		case COD_RECIBIR_UBICACION_TRIPULANTE:
			payload = recibir_payload(cliente_fd);
			recibir_ubicacion_tripulante(payload, tabla_patota, dir_log);
			free(payload);
			break;
		case COD_ENVIAR_PROXIMA_TAREA:
			payload = recibir_payload(cliente_fd);
			enviar_proxima_tarea(cliente_fd, payload, tabla_patota, dir_log);
			free(payload);
			break;
		case COD_EXPULSAR_TRIPULANTE:
			payload = recibir_payload(cliente_fd);
			expulsar_tripulante(payload, tabla_patota, dir_log);
			free(payload);
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