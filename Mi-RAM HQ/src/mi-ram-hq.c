#include "mi-ram-hq.h"

int main(void)
{
	logger = log_create("./cfg/mi-ram-hq.log", "Mi-RAM HQ", 1, LOG_LEVEL_DEBUG);

	// Leo IP y PUERTO del config
	t_config *config = config_create("./cfg/mi-ram-hq.config");
	char* puerto_escucha = config_get_string_value(config, "PUERTO");
	char* ip = "127.0.0.1";

	int server_fd = iniciar_servidor(ip, puerto_escucha);
	log_info(logger, "Mi-RAM HQ listo para recibir al Discordiador");

	mi_ram_hq(server_fd);

	log_info(logger, "Cerrando socket servidor");
	close(server_fd);
	log_destroy(logger);

	rl_clear_history();

	return EXIT_SUCCESS;
}

void mi_ram_hq(int servidor_fd) {
	// Declaramos variables
	struct sockaddr_in dir_cliente;					// auxiliar
	int tam_direccion = sizeof(struct sockaddr_in);	// auxiliar
	int *cliente_fd;

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
				// Aceptamos la conexion
				cliente_fd = malloc(sizeof(int));
				*cliente_fd = accept(servidor_fd, (void*) &dir_cliente, (socklen_t*) &tam_direccion);
				fcntl(cliente_fd, F_SETFL, O_NONBLOCK);
				log_info(logger, "Se conecto un cliente!");
				// Creamos el hilo que se encarga de atender el cliente
				// NOTA: el struct pthread_t de cada hilo se pierde
				hilo_atender_cliente = malloc(sizeof(pthread_t));
				pthread_create(hilo_atender_cliente, NULL, (void*) atender_cliente, cliente_fd);
				pthread_detach(*hilo_atender_cliente);
				free(hilo_atender_cliente);
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
				}
			}
		}
	}
}

void atender_cliente(void *args){
	int cliente_fd = *((int*) args);
	free(args);
	// Nos comunicamos con el cliente
	comunicacion_cliente(cliente_fd);

	log_info(logger,"Cerrando socket cliente");
	close(cliente_fd);
}

void leer_consola_y_procesar() {
	char linea_consola[TAM_CONSOLA];
	fgets(linea_consola,TAM_CONSOLA,stdin);
	linea_consola[strlen(linea_consola)-1]='\0';	// Le saco el \n

	if(strcmp(linea_consola,"FIN") == 0){
		status_servidor = END;
		log_info(logger, "Finalizando Mi-RAM HQ");
	}
}

int comunicacion_cliente(int cliente_fd) {
	struct pollfd pfds[1];
	pfds[0].fd = cliente_fd;	
	pfds[0].events = POLLIN;	// Avisa cuando hay alguna operacion para leer en el buffer
	int num_events;

	while(status_servidor != END) {
		// Revisamos si hay algun evento en el file descriptor del cliente
		num_events = poll(pfds, 1, 2500);

		// Si hay un evento
		if(num_events != 0){
			// Si hay un mensaje del cliente
			if(pfds[0].revents & POLLIN)
				leer_mensaje_cliente_y_procesar(cliente_fd);
			else
				log_error(logger, "Evento inesperado en file descriptor del cliente: %s", strerror(pfds[0].revents));
		}
	}
	return EXIT_SUCCESS;
}

void leer_mensaje_cliente_y_procesar(int cliente_fd){
	char* mensaje;
	// Leo codigo de operacion
	int cod_op = recibir_operacion(cliente_fd);
	switch(cod_op) {
		// Si mando un mensaje, lo logueo
		case MENSAJE:
			mensaje = recibir_mensaje(cliente_fd);
			log_info(logger, "Recibi el mensaje: %s", mensaje);
			free(mensaje);
			break;
		case -1:
			log_error(logger, "El cliente se desconecto.");
			return EXIT_FAILURE;
		default:
			log_warning(logger, "Operacion desconocida. No quieras meter la pata");
			break;
	}
}