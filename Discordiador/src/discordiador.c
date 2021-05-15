#include "discordiador.h"

int main(void) {

	// Inicializo variables globales
	generadorPID = 0;
	generadorTID = 0;

	sem_init(&sem_generador_PID,0,1);
	sem_init(&sem_generador_TID,0,1);
	sem_init(&sem_struct_iniciar_tripulante,0,1);
	
	// Creo logger
	logger = log_create("./cfg/discordiador.log", "Discordiador", 1, LOG_LEVEL_DEBUG);
	log_info(logger, "Inicializando el Discordiador");

	int i_mongo_store_fd;
	int mi_ram_hq_fd;

	// Leo IP y PUERTO del config
	t_config *config = config_create("./cfg/discordiador.config");
	log_info(logger, "Iniciando configuracion");
	direccion_IP_i_Mongo_Store = string_new();
	puerto_i_Mongo_Store = string_new();
	direccion_IP_Mi_RAM_HQ = string_new();
	puerto_Mi_RAM_HQ = string_new();
	string_append(&direccion_IP_i_Mongo_Store, config_get_string_value(config, "IP_I_MONGO_STORE"));
	string_append(&puerto_i_Mongo_Store, config_get_string_value(config, "PUERTO_I_MONGO_STORE"));
	string_append(&direccion_IP_Mi_RAM_HQ, config_get_string_value(config, "IP_MI_RAM_HQ"));
	string_append(&puerto_Mi_RAM_HQ, config_get_string_value(config, "PUERTO_MI_RAM_HQ"));
	log_info(logger, "Configuracion terminada");

	log_info(logger, "Conectando con i-Mongo-Store");
	// Intento conectarme con i-Mongo-Store
	if(crear_conexion(direccion_IP_i_Mongo_Store, puerto_i_Mongo_Store, &i_mongo_store_fd) == EXIT_FAILURE){
		log_error(logger, "No se pudo establecer la conexion con el i-Mongo-Store");
		// Libero recursos
		liberar_conexion(i_mongo_store_fd);
		log_destroy(logger);
		config_destroy(config);
		return EXIT_FAILURE;
	}
	log_info(logger, "Conexion con el i-Mongo-Store exitosa");

	log_info(logger, "Conectando con Mi-RAM HQ");
	// Intento conectarme con Mi-RAM HQ
	if(crear_conexion(direccion_IP_Mi_RAM_HQ, puerto_Mi_RAM_HQ, &mi_ram_hq_fd) == EXIT_FAILURE){
		log_error(logger, "No se pudo establecer la conexion con Mi-RAM HQ");
		// Libero recursos
		liberar_conexion(i_mongo_store_fd);
		liberar_conexion(mi_ram_hq_fd);
		log_destroy(logger);
		config_destroy(config);
		return EXIT_FAILURE;
	}
	log_info(logger, "Conexion con el Mi-RAM HQ");
	
	servidor_desconectado = false;

	leer_fds(i_mongo_store_fd, mi_ram_hq_fd);

	// Libero recursos
	log_info(logger,"Finalizando Discordiador");
	log_destroy(logger);
	config_destroy(config);
	liberar_conexion(i_mongo_store_fd);
	
	return EXIT_SUCCESS;
}

void leer_fds(int i_mongo_store_fd, int mi_ram_hq_fd){
	struct pollfd pfds[2];
	pfds[0].fd = i_mongo_store_fd;	
	pfds[0].events = POLLIN;	// Avisa cuando llego un mensaje del i-Mongo-Store
	pfds[1].fd = 0;
	pfds[1].events = POLLIN;	// Avisa cuando llego un mensaje de la consola
	
	int num_events;

	while(1){
		// Revisamos si ocurrio un evento
		num_events = poll(pfds, 2, 2500);

		// Si ocurrio un evento
		if(num_events != 0){		
			// Si llego un mensaje por consola
			if((pfds[1].revents & POLLIN)){
				// Leemos la consola y procesamos el mensaje
				leer_consola_y_procesar(i_mongo_store_fd, mi_ram_hq_fd);
			}
			else{
				// Si llego un mensaje del i-Mongo-Store
				if((pfds[0].revents & POLLIN)){
					// Leemos el mensaje del socket y lo procesamos
					//recibir_y_procesar_mensaje_i_mongo_store();
					log_info(logger, "i-Mongo-Store envio un mensaje");
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

// REVISAR
/*
void recibir_y_procesar_mensaje_i_mongo_store(){
	cod_op = recibir_operacion(i_mongo_store_fd);
	switch(cod_op)
	{
	case MENSAJE:
		mensaje = recibir_mensaje(i_mongo_store_fd);
		log_info(logger, "Recibi el mensaje: %s", mensaje);
		free(mensaje);
		break;
	case -1:
		log_error(logger, "El servidor se desconecto. Terminando cliente");
		liberar_conexion(i_mongo_store_fd);
		servidor_desconectado = true;
		return EXIT_FAILURE;
	default:
		log_warning(logger, "Operacion desconocida. No quieras meter la pata");
		break;
}*/

void leer_consola_y_procesar(int i_mongo_store_fd, int mi_ram_hq_fd) {
	int estado_envio_mensaje;
	enum comando_discordiador comando;
	char** argumentos;
	char linea_consola[TAM_CONSOLA];
	fgets(linea_consola,TAM_CONSOLA,stdin);
	linea_consola[strlen(linea_consola)-1]='\0';	// Le saco el \n

	argumentos = (char**) string_split(linea_consola, " ");
	 
	// Test: Mando cada argumento como un mensaje al i-Mongo-Store
	for(int i = 0;argumentos[i]!=NULL;i++){
		log_info(logger, "Llego un mensaje por consola: %s", argumentos[i]);
		// Enviamos el mensaje leido al i-MongoStore (porque pinto)
		if(!servidor_desconectado)
			estado_envio_mensaje = enviar_mensaje(i_mongo_store_fd, argumentos[i]);
		if(estado_envio_mensaje != EXIT_SUCCESS)
			log_error(logger, "No se pudo mandar el mensaje al i-Mongo-Store");
	}

	// Reviso cual fue el comando ingresado y lo ejecuto
	comando = string_to_comando_discordiador(argumentos[0]);

	switch(comando){
		case INICIAR_PATOTA:
			iniciar_patota(argumentos, mi_ram_hq_fd);
			break;
		case LISTAR_TRIPULANTES:
			log_info(logger, "Listando tripulantes");
			break;
		case EXPULSAR_TRIPULANTES:
			log_info(logger, "Expulsando tripulante");
			break;
		case INICIAR_PLANIFICACION:
			log_info(logger, "Iniciando planificacion");
			break;
		case PAUSAR_PLANIFICACION:
			log_info(logger, "Pausando planificacion");
			break;
		case OBTENER_BITACORA:
			log_info(logger, "Obteniendo bitacora");
			break;
		default:
			log_error(logger, "%s: comando no encontrado", argumentos[0]);
	}

	// Libero los recursos del array argumentos
	for(int i = 0;argumentos[i]!=NULL;i++){
		free(argumentos[i]);
	}
	free(argumentos);
}

int cantidad_argumentos(char** argumentos){
	int cantidad = 0;
	for(;argumentos[cantidad]!=NULL;cantidad++);
	return cantidad;
}

int iniciar_patota(char** argumentos, int mi_ram_hq_fd){
	pthread_t *hilo_submodulo_tripulante;
	int cantidad_args, cantidad_tripulantes;
	char* lista_de_tareas = "Lista de tareas";
	FILE* archivo_de_tareas;
	iniciar_tripulante_t struct_iniciar_tripulante;
	char **posicion;
	int PID;
          
	// Verificamos la cantidad de argumentos
	cantidad_args = cantidad_argumentos(argumentos);
	if(cantidad_args < 3){
		log_error(logger, "INICIAR_PATOTA: Faltan argumentos");
		return EXIT_FAILURE;
	}
	cantidad_tripulantes = atoi(argumentos[1]);
	if(cantidad_args > cantidad_tripulantes + 3){
		log_error(logger, "INICIAR_PATOTA: Sobran argumentos");
		return EXIT_FAILURE;
	}

	// Abrimos el archivo de tareas
	archivo_de_tareas = fopen(argumentos[2],"r");
	if(archivo_de_tareas == NULL){
		log_error(logger, "INICIAR_PATOTA: %s: No existe el archivo",argumentos[2]);
		return EXIT_FAILURE;
	}

	log_info(logger, "Iniciando patota");

	// TODO: Leemos el archivo de tareas

	// Generamos un nuevo PID
	PID	= generarNuevoPID();
	struct_iniciar_tripulante.PID = PID;

	// Le pedimos a Mi-RAM HQ que inicie la patota
	enviar_op_iniciar_patota(mi_ram_hq_fd, PID, lista_de_tareas);

	for(int i = 0;i < cantidad_tripulantes;i++){
		sem_wait(&sem_struct_iniciar_tripulante);
		if( 3 + i < cantidad_args){
			posicion = string_split(argumentos[3 + i],"|");
			struct_iniciar_tripulante.posicion_X = atoi(posicion[0]);
			struct_iniciar_tripulante.posicion_Y = atoi(posicion[1]);
			// Libero los recursos del array posicion
			for(int i = 0;posicion[i]!=NULL;i++){
				free(posicion[i]);
			}
			free(posicion);
		}
		else{
			struct_iniciar_tripulante.posicion_X = 0;
			struct_iniciar_tripulante.posicion_Y = 0;
		}

		struct_iniciar_tripulante.TID = generarNuevoTID();

		// Creamos el hilo para el submodulo tripulante
		// NOTA: el struct pthread_t de cada hilo tripulante se pierde
		hilo_submodulo_tripulante = malloc(sizeof(pthread_t));
		pthread_create(hilo_submodulo_tripulante, NULL, (void*) submodulo_tripulante, &struct_iniciar_tripulante);
		pthread_detach(*hilo_submodulo_tripulante);
		free(hilo_submodulo_tripulante);
	}

	log_info(logger, "La patota fue inicializada");

	fclose(archivo_de_tareas);
	return EXIT_SUCCESS;
}

int submodulo_tripulante(void* args) {
	iniciar_tripulante_t struct_iniciar_tripulante = *((iniciar_tripulante_t*) args);
	sem_post(&sem_struct_iniciar_tripulante);

	int mi_ram_hq_fd_tripulante;
	int i_mongo_store_fd_tripulante;
	int estado_envio_mensaje;

	log_info(logger, "Iniciando tripulante");

	// Intento conectarme con i-MongoStore
	if(crear_conexion(direccion_IP_i_Mongo_Store, puerto_i_Mongo_Store, &i_mongo_store_fd_tripulante) == EXIT_FAILURE){
		log_error(logger, "Submodulo Tripulante: No se pudo establecer la conexion con el i-MongoStore");
		// Libero recursos
		liberar_conexion(i_mongo_store_fd_tripulante);
		return EXIT_FAILURE;
	}

	// Intento conectarme con Mi-Ram HQ
	if(crear_conexion(direccion_IP_Mi_RAM_HQ, puerto_Mi_RAM_HQ, &mi_ram_hq_fd_tripulante) == EXIT_FAILURE){
		log_error(logger, "Submodulo Tripulante: No se pudo establecer la conexion con Mi-Ram HQ");
		// Libero recursos
		liberar_conexion(i_mongo_store_fd_tripulante);
		liberar_conexion(mi_ram_hq_fd_tripulante);
		return EXIT_FAILURE;
	}

	// Le pedimos a Mi-RAM HQ que inicie al tripulante
	estado_envio_mensaje = enviar_op_iniciar_tripulante(mi_ram_hq_fd_tripulante, struct_iniciar_tripulante);
	if(estado_envio_mensaje != EXIT_SUCCESS)
		log_error(logger, "No se pudo mandar el mensaje al Mi-Ram HQ");
	log_info(logger, "Tripulante inicializado");

	while(1){
		sleep(5);
		// Test: Mando un mensaje al i-Mongo-Store y a Mi-Ram HQ
		
		// Hay que ver si el servidor esta conectado?
		estado_envio_mensaje = enviar_mensaje(i_mongo_store_fd_tripulante, "Hola, soy un tripulante");
		if(estado_envio_mensaje != EXIT_SUCCESS)
			log_error(logger, "No se pudo mandar el mensaje al i-Mongo-Store");
		// Hay que ver si el servidor esta conectado?
		estado_envio_mensaje = enviar_mensaje(mi_ram_hq_fd_tripulante, "Hola, soy un tripulante");
		if(estado_envio_mensaje != EXIT_SUCCESS)
			log_error(logger, "No se pudo mandar el mensaje al Mi-Ram HQ");
	}

	return EXIT_SUCCESS;
}

enum comando_discordiador string_to_comando_discordiador(char* string){
	if(strcmp(string,"INICIAR_PATOTA")==0)
		return INICIAR_PATOTA;
	if(strcmp(string,"LISTAR_TRIPULANTES")==0)
		return LISTAR_TRIPULANTES;
	if(strcmp(string,"EXPULSAR_TRIPULANTES")==0)
		return EXPULSAR_TRIPULANTES;
	if(strcmp(string,"INICIAR_PLANIFICACION")==0)
		return INICIAR_PLANIFICACION;
	if(strcmp(string,"PAUSAR_PLANIFICACION")==0)
		return PAUSAR_PLANIFICACION;
	if(strcmp(string,"OBTENER_BITACORA")==0)
		return OBTENER_BITACORA;
	return ERROR;
}

int generarNuevoPID() {
	int nuevo_valor;
	sem_wait(&sem_generador_PID);
	generadorPID++;
	nuevo_valor = generadorPID;
	sem_post(&sem_generador_PID);
	return nuevo_valor;
}

int generarNuevoTID() {
	int nuevo_valor;
	sem_wait(&sem_generador_TID);
	generadorTID++;
	nuevo_valor = generadorTID;
	sem_post(&sem_generador_TID);
	return nuevo_valor;
}