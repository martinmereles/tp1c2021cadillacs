#include "discordiador.h"

int main(void) {
	// Creo logger
	logger = log_create("./cfg/discordiador.log", "Discordiador", 1, LOG_LEVEL_DEBUG);

	// Leo IP y PUERTO del config
	t_config *config = config_create("./cfg/discordiador.config");
	char* direccion_IP_i_MongoStore = config_get_string_value(config, "IP_I_MONGO_STORE");
	char* puerto_i_MongoStore = config_get_string_value(config, "PUERTO_I_MONGO_STORE");
	char* direccion_IP_Mi_RAM_HQ = config_get_string_value(config, "IP_MI_RAM_HQ");
	char* puerto_Mi_RAM_HQ = config_get_string_value(config, "PUERTO_MI_RAM_HQ");

	// Intento conectarme con i-MongoStore
	if(crear_conexion(direccion_IP_i_MongoStore, puerto_i_MongoStore, &i_mongostore_fd) == EXIT_FAILURE){
		log_error(logger, "No se pudo establecer la conexion con el i-MongoStore");
		// Libero recursos
		liberar_conexion(i_mongostore_fd);
		log_destroy(logger);
		config_destroy(config);
		return EXIT_FAILURE;
	}

	// Intento conectarme con Mi-Ram HQ
	if(crear_conexion(direccion_IP_Mi_RAM_HQ, puerto_Mi_RAM_HQ, &mi_ram_hq_fd) == EXIT_FAILURE){
		log_error(logger, "No se pudo establecer la conexion con el i-MongoStore");
		// Libero recursos
		liberar_conexion(i_mongostore_fd);
		liberar_conexion(mi_ram_hq_fd);
		log_destroy(logger);
		config_destroy(config);
		return EXIT_FAILURE;
	}

	log_info(logger, "Conexion con los servidores exitosa");
	servidor_desconectado = false;

	// Creo hilos
	pthread_t hilo_mandar_mensajes, hilo_recibir_mensajes_i_mongostore, hilo_recibir_mensajes_mi_ram_hq;

	pthread_create(&hilo_mandar_mensajes, NULL, (void*) mandar_mensajes, NULL);
	pthread_create(&hilo_recibir_mensajes_i_mongostore, NULL, (void*) recibir_operaciones_servidor, &i_mongostore_fd);
	pthread_create(&hilo_recibir_mensajes_mi_ram_hq, NULL, (void*) recibir_operaciones_servidor, &mi_ram_hq_fd);

	pthread_detach(hilo_recibir_mensajes_i_mongostore);
	pthread_detach(hilo_recibir_mensajes_mi_ram_hq);
	pthread_join(hilo_mandar_mensajes, (void**) NULL);

	// Libero recursos
	log_info(logger,"Finalizando Discordiador");
	log_destroy(logger);
	config_destroy(config);
	liberar_conexion(i_mongostore_fd);
	liberar_conexion(mi_ram_hq_fd);
	
	return EXIT_SUCCESS;
}

int recibir_operaciones_servidor(void *args) {
	int servidor_fd = *((int*) args);
	int cod_op;
	char* mensaje;

	while(1)
	{
		cod_op = recibir_operacion(servidor_fd);
		switch(cod_op)
		{
		case MENSAJE:
			mensaje = recibir_mensaje(servidor_fd);
			log_info(logger, "Recibi el mensaje: %s", mensaje);
			free(mensaje);
			break;
		case -1:
			log_error(logger, "El servidor se desconecto. Terminando cliente");
			liberar_conexion(servidor_fd);
			servidor_desconectado = true;
			return EXIT_FAILURE;
		default:
			log_warning(logger, "Operacion desconocida. No quieras meter la pata");
			break;
		}
	}
	return EXIT_SUCCESS;
}

void mandar_mensajes(void *args) {
	int estado_envio_mensaje;
	char* mensaje;
	mensaje = readline("");
	while(strcmp(mensaje,"") != 0 && !servidor_desconectado){
		estado_envio_mensaje = enviar_mensaje(i_mongostore_fd, mensaje);
		estado_envio_mensaje = enviar_mensaje(mi_ram_hq_fd, mensaje);
		if(estado_envio_mensaje != EXIT_SUCCESS)
			break;
		free(mensaje);
		mensaje = readline("");
	}
	free(mensaje);
}