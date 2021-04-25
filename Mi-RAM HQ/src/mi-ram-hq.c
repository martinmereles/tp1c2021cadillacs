#include "mi-ram-hq.h"

int main(void)
{
	logger = log_create("./cfg/mi-ram-hq.log", "Mi-RAM HQ", 1, LOG_LEVEL_DEBUG);

	int server_fd = iniciar_servidor();
	log_info(logger, "Mi-RAM HQ listo para recibir al Discordiador");

	pthread_t hilo_recibir_mensajes_discordiador, hilo_leer_consola;

	pthread_create(&hilo_recibir_mensajes_discordiador, NULL, (void*) recibir_discordiador, &server_fd);
	pthread_create(&hilo_leer_consola, NULL, (void*) leer_consola_servidor, NULL);	

	pthread_detach(hilo_leer_consola);
	pthread_join(hilo_recibir_mensajes_discordiador, (void**) NULL);

	log_info(logger, "Cerrando socket servidor");
	close(server_fd);
	log_destroy(logger);

	rl_clear_history();

	return EXIT_SUCCESS;
}

void leer_consola_servidor(void* args) {

	char* mensaje;
	while(status_servidor == RUNNING){
		mensaje = readline("");
		if(strcmp(mensaje,"FIN") == 0){
			status_servidor = END;
		}	
		free(mensaje);
	}
	log_info(logger, "Finalizando Mi-RAM HQ");
}

void recibir_discordiador(void *args) {
	// Declaramos variables
	int servidor_fd = *((int*) args);
	status_servidor = RUNNING;
	struct sockaddr_in dir_cliente;					// auxiliar
	int tam_direccion = sizeof(struct sockaddr_in);	// auxiliar
	int cliente_fd;

	struct pollfd pfds[1];
	pfds[0].fd = servidor_fd;	
	pfds[0].events = POLLIN;	// Avisa cuando hay alguna operacion para leer en el buffer
	int num_events;

	while(status_servidor != END){
		// Revisamos si hay un cliente que quiere conectarse
		num_events = poll(pfds, 1, 2500);

		// Si hay, lo aceptamos
		if(num_events != 0){
			if((pfds[0].revents & POLLIN)){
				// Aceptamos la conexion
				cliente_fd = accept(servidor_fd, (void*) &dir_cliente, (socklen_t*) &tam_direccion);
				fcntl(cliente_fd, F_SETFL, O_NONBLOCK);
				log_info(logger, "Se conecto el Discordiador!");

				// Recibimos las operaciones del cliente
				recibir_operaciones_discordiador(cliente_fd);

				log_info(logger,"Cerrando socket cliente");
				close(cliente_fd);
			}
			else{
				log_error(logger, "Evento inesperado en file descriptor del cliente: %s", strerror(pfds[0].revents));
			}
		}
	}
}

int recibir_operaciones_discordiador(int cliente_fd) {
	struct pollfd pfds[1];
	pfds[0].fd = cliente_fd;	
	pfds[0].events = POLLIN;	// Avisa cuando hay alguna operacion para leer en el buffer
	int num_events;
	char* mensaje;

	while(status_servidor != END) {
		// Revisamos si hay algun evento en el file descriptor del cliente
		num_events = poll(pfds, 1, 2500);

		// Si hay un evento
		if(num_events != 0){
			// Si hay algo para leer en el buffer
			if(pfds[0].revents & POLLIN){
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
						log_error(logger, "El Discordiador se desconecto.");
						return EXIT_FAILURE;
					default:
						log_warning(logger, "Operacion desconocida. No quieras meter la pata");
						break;
				}
			}
			else
				log_error(logger, "Evento inesperado en file descriptor del cliente: %s", strerror(pfds[0].revents));
		}
	}
	return EXIT_SUCCESS;
}