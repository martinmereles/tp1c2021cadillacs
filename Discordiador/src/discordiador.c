#include "discordiador.h"

int main(void) {

	// Inicializo variables globales
	status_discordiador = RUNNING;
    estado_planificador = PLANIFICADOR_OFF;
	tripulante_a_expulsar = -1;

	generadorPID = 0;
	generadorTID = 0;

	cola_new=queue_create();

	sem_init(&sem_generador_PID,0,1);
	sem_init(&sem_generador_TID,0,1);
	sem_init(&sem_struct_iniciar_tripulante,0,0);

	// Inicializo semaforo de Planificacion
	sem_init(&sem_planificacion_fue_iniciada,0,0);

    // Inicializo semaforos del dispatcher
    sem_init(&sem_mutex_ingreso_tripulantes_new,0,1);
    sem_init(&sem_mutex_ejecutar_dispatcher,0,1);
	sem_init(&sem_puede_expulsar_tripulante,0,1);
	sem_init(&sem_confirmar_expulsion_tripulante,0,0);
	sem_init(&sem_sabotaje_activado,0,0);
	sem_init(&sem_mutex_tripulante_a_expulsar,0,1);
	
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
	algoritmo_planificador = string_new();
    char *string_quantum;
	string_quantum = string_new();
	char *string_grado_multitarea;
	string_grado_multitarea = string_new();
	char *string_duracion_sabotaje = string_new();
	char *string_retardo = string_new();
	string_append(&direccion_IP_i_Mongo_Store, config_get_string_value(config, "IP_I_MONGO_STORE"));
	string_append(&puerto_i_Mongo_Store, config_get_string_value(config, "PUERTO_I_MONGO_STORE"));
	string_append(&direccion_IP_Mi_RAM_HQ, config_get_string_value(config, "IP_MI_RAM_HQ"));
	string_append(&puerto_Mi_RAM_HQ, config_get_string_value(config, "PUERTO_MI_RAM_HQ"));
	string_append(&algoritmo_planificador, config_get_string_value(config, "ALGORITMO"));
	string_append(&string_grado_multitarea, config_get_string_value(config, "GRADO_MULTITAREA"));
	string_append(&string_duracion_sabotaje, config_get_string_value(config, "DURACION_SABOTAJE"));
	string_append(&string_retardo, config_get_string_value(config, "RETARDO_CICLO_CPU"));

	grado_multiproc = atoi(string_grado_multitarea);
	duracion_sabotaje = atoi(string_duracion_sabotaje);
	retardo_ciclo_cpu = atoi(string_retardo);

    if(strcmp(algoritmo_planificador,"RR")==0){
        string_append(&string_quantum, config_get_string_value(config, "QUANTUM"));
        quantum = atoi(string_quantum); // Variable global
    }
    else{
	if(strcmp(algoritmo_planificador,"FIFO")==0)
		quantum = 0; // Variable global
    else
        return EXIT_FAILURE;
    }
    free(string_quantum);

	// Inicializo semaforo grado multitarea
	sem_init(&sem_recurso_multitarea_disponible, 0, grado_multiproc);

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
	log_info(logger, "Conexion con Mi-RAM HQ exitosa");

	leer_fds(i_mongo_store_fd, mi_ram_hq_fd);

	// Libero recursos
	log_info(logger,"Finalizando Discordiador");
	log_destroy(logger);
	config_destroy(config);
	liberar_conexion(i_mongo_store_fd);
	liberar_conexion(mi_ram_hq_fd);
	
	return EXIT_SUCCESS;
}

void leer_fds(int i_mongo_store_fd, int mi_ram_hq_fd){
	struct pollfd pfds[2];
	pfds[0].fd = i_mongo_store_fd;	
	pfds[0].events = POLLIN;	// Avisa cuando llego un mensaje del i-Mongo-Store
	pfds[1].fd = 0;
	pfds[1].events = POLLIN;	// Avisa cuando llego un mensaje de la consola
	
	int num_events;

	while(status_discordiador != END){
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
					recibir_y_procesar_mensaje_i_mongo_store(i_mongo_store_fd);
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

void recibir_y_procesar_mensaje_i_mongo_store(int i_mongo_store_fd){
	char* mensaje;
	int cod_op = recibir_operacion(i_mongo_store_fd);
	switch(cod_op)
	{
		case COD_MENSAJE:
			mensaje = recibir_mensaje(i_mongo_store_fd);
			log_info(logger, "i-Mongo-Store envio un mensaje: %s", mensaje);
			free(mensaje);
			break;
		case COD_OBTENER_BITACORA:;
			log_info(logger, "obteniendo bitacora desde el i-mongo...");
			char * bitacora = recibir_payload(i_mongo_store_fd);
			log_info(logger, bitacora);
			/*char ** lista_bitacora = string_split(bitacora,";");
			for(int i =0 ;lista_bitacora[i]!=NULL ;i++){
				log_info(logger, lista_bitacora[i]);
			}*/
			break;
		case -1:
			log_error(logger, "El i-Mongo-Store se desconecto. Terminando Discordiador");
			status_discordiador = END;
			break;
		default:
			log_warning(logger, "Operacion desconocida. No quieras meter la pata");
			break;
	}
}

void leer_consola_y_procesar(int i_mongo_store_fd, int mi_ram_hq_fd) {
	int estado_envio_mensaje;
	enum comando_discordiador comando;
	char** argumentos;
	char linea_consola[TAM_CONSOLA];
	fgets(linea_consola,TAM_CONSOLA,stdin);
	linea_consola[strlen(linea_consola)-1]='\0';	// Le saco el \n

	argumentos = (char**) string_split(linea_consola, " ");
	 /*
	// Test: Mando cada argumento como un mensaje al i-Mongo-Store
	for(int i = 0;argumentos[i]!=NULL;i++){
		//log_info(logger, "Llego un mensaje por consola: %s", argumentos[i]);
		// Enviamos el mensaje leido al i-MongoStore (porque pinto)
		
		estado_envio_mensaje = enviar_mensaje(i_mongo_store_fd, argumentos[i]);
		if(estado_envio_mensaje != EXIT_SUCCESS)
			log_error(logger, "No se pudo mandar el mensaje al i-Mongo-Store");
		
	}*/

	// Reviso cual fue el comando ingresado y lo ejecuto
	comando = string_to_comando_discordiador(argumentos[0]);

	switch(comando){
		case INICIAR_PATOTA:
			/*
			OBSERVACION: Para una ejecucion CORRECTA es necesario ejecutar PRIMERO el comando INICIAR_PLANIFICACION.
			Esto es debido a que hay acoplamiento con la gestion de colas.

			TAMBIEN en cumplimiento con la descripcion del TP:
				Ver en -> Modulo Discordiador - Administracion de tripulantes
			*/
			iniciar_patota(argumentos, mi_ram_hq_fd);
			
			break;
		case LISTAR_TRIPULANTES:
			/*
			OBSERVACION: presenta posibles resultados no safisfactorios.
			Se recomienda evitar su uso. 
			*/
			log_info(logger, "Listando tripulantes");
			
			if (estado_planificador != PLANIFICADOR_RUNNING){
				if (listar_tripulantes() != EXIT_SUCCESS) //TODO: revisar funcion
					log_error(logger, "No hay tripulantes en ninguna de las colas");
				else 
					log_debug(logger, "Listado completado");
			}
			else {
				sem_wait(&sem_mutex_ejecutar_dispatcher);
				sem_wait(&sem_mutex_ingreso_tripulantes_new);
				if (listar_tripulantes() != EXIT_SUCCESS) //TODO: revisar funcion
					log_error(logger, "No hay tripulantes en ninguna de las colas");
				else 
					log_debug(logger, "Listado completado");
				sem_post(&sem_mutex_ingreso_tripulantes_new);
				sem_post(&sem_mutex_ejecutar_dispatcher);
			}
			
			break;
		case EXPULSAR_TRIPULANTE:{
			log_info(logger, "Expulsando tripulante");
			
			sem_wait(&sem_mutex_ejecutar_dispatcher);
			sem_wait(&sem_mutex_tripulante_a_expulsar);
			tripulante_a_expulsar = atoi(argumentos[1]);
			sem_post(&sem_mutex_tripulante_a_expulsar);
			estado_envio_mensaje = dispatcher_expulsar_tripulante( atoi(argumentos[1]) );
			if (estado_envio_mensaje != EXIT_SUCCESS){
				log_error(logger, "No se encontro el tripulante");
				sem_wait(&sem_mutex_tripulante_a_expulsar);
				tripulante_a_expulsar = -1;
				sem_post(&sem_mutex_tripulante_a_expulsar);
				sem_post(&sem_mutex_ejecutar_dispatcher);
			}
			else{
				sem_post(&sem_mutex_ejecutar_dispatcher);

				// Una vez que el hilo tripulante haya sido eliminado
				sem_wait(&sem_confirmar_expulsion_tripulante);
				log_debug(logger, "El hilo del tripulante fue eliminado/expulsado satisfactoriamente");
				
				sem_wait(&sem_mutex_tripulante_a_expulsar);
				tripulante_a_expulsar = -1;
				sem_post(&sem_mutex_tripulante_a_expulsar);
			}
			}break;
		case INICIAR_PLANIFICACION:
            log_info(logger, "Iniciando planificacion");

			// Habilito la planificacion para cada tripulante cuando se ejecuta por 1ra vez o si fue pausado:
			if (estado_planificador == PLANIFICADOR_OFF || estado_planificador == PLANIFICADOR_BLOCKED){
				sem_post(&sem_planificacion_fue_iniciada);
				// Inicia la ejecucion del HILO Dispatcher: gestor de QUEUEs del Sistema
				if ( iniciar_dispatcher(algoritmo_planificador) != EXIT_SUCCESS)
					log_error(logger, "ERROR inesperado, no se pudo iniciar el dispatcher");
			}
			else 
				log_debug(logger, "La planificacion ya fue iniciada");

			break;
		case PAUSAR_PLANIFICACION:
			log_info(logger, "Pausando planificacion");
	
			if (estado_planificador == PLANIFICADOR_RUNNING){
				sem_wait(&sem_planificacion_fue_iniciada);
				dispatcher_pausar();
				log_debug(logger, "Fue pausado exitosamente");
			}else 
				log_debug(logger, "Ya esta pausado");
			
			break;
		case OBTENER_BITACORA:
			log_info(logger, "Obteniendo bitacora");
			obtener_bitacora(argumentos,i_mongo_store_fd);
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
	iniciar_tripulante_t struct_iniciar_tripulante;
	char **posicion;
	int PID;

	FILE* archivo_de_tareas;
	char* lista_de_tareas = string_new();
	char tarea[TAM_TAREA];
          
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

	// Leemos el archivo de tareas
	log_info(logger, "Leyendo archivo de tareas");
	while(!feof(archivo_de_tareas)){
		fgets(tarea, TAM_CONSOLA, archivo_de_tareas);
		string_append(&lista_de_tareas, tarea);
	}
	log_info(logger, "Archivo de tareas leido");

	// Generamos un nuevo PID
	PID	= generarNuevoPID();
	struct_iniciar_tripulante.PID = PID;

	// Le pedimos a Mi-RAM HQ que inicie la patota
	enviar_op_iniciar_patota(mi_ram_hq_fd, PID, cantidad_tripulantes, lista_de_tareas);

	// Esperamos a la confirmacion de que fue creada con exito
	if(recibir_operacion(mi_ram_hq_fd) != COD_INICIAR_PATOTA_OK){
		log_error(logger,"Mi RAM HQ denego la creacion de la patota");
		return EXIT_FAILURE;
	}

	log_info(logger,"Mi RAM HQ creo la patota con exito");

	for(int i = 0;i < cantidad_tripulantes;i++){
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

        // Agregando nuevo tripulante a cola NEW
 
		sem_wait(&sem_mutex_ingreso_tripulantes_new);
		iniciador_tripulante(struct_iniciar_tripulante.TID, struct_iniciar_tripulante.PID);
        sem_post(&sem_mutex_ingreso_tripulantes_new);


		// Creamos el hilo para el submodulo tripulante
		// NOTA: el struct pthread_t de cada hilo tripulante se pierde
		hilo_submodulo_tripulante = malloc(sizeof(pthread_t));
		pthread_create(hilo_submodulo_tripulante, NULL, (void*) submodulo_tripulante, &struct_iniciar_tripulante);
		pthread_detach(*hilo_submodulo_tripulante);
		free(hilo_submodulo_tripulante);

		// Los hilos comparten el struct "struct_iniciar_tripulante"
		// Espero hasta que el hilo creado haya terminado de usarlo
		sem_wait(&sem_struct_iniciar_tripulante);
	}

	log_info(logger, "La patota fue inicializada");

	fclose(archivo_de_tareas);
	return EXIT_SUCCESS;
}

int obtener_bitacora(char ** argumentos, int i_mongo_store_fd){
	int cantidad_args = cantidad_argumentos(argumentos);
	char* id_tripulante;
	if(cantidad_args < 2){
		log_error(logger, "OBTENER_BITACORA: Faltan argumentos");
		return EXIT_FAILURE;
	}
	if(cantidad_args > 2){
		log_error(logger, "OBTENER_BITACORA: Sobran argumentos");
		return EXIT_FAILURE;
	}
	id_tripulante = argumentos[1];
	int estado_envio =  enviar_operacion(i_mongo_store_fd, COD_OBTENER_BITACORA, id_tripulante, strlen(id_tripulante)+1);
	if(estado_envio != EXIT_SUCCESS)
		log_error(logger, "No se pudo mandar el mensaje al i-Mongo-Store");

	return EXIT_SUCCESS;
}

int submodulo_tripulante(void* args) {
	iniciar_tripulante_t struct_iniciar_tripulante = *((iniciar_tripulante_t*) args);
	t_tarea st_tarea;
	char* tarea;
	char estado_tripulante = 'E';

	int mi_ram_hq_fd_tripulante;
	int i_mongo_store_fd_tripulante;
	int estado_envio_mensaje;
	int pos_X, pos_Y;
	pos_X = 0;
	pos_Y = 0;

	log_info(logger, "Iniciando tripulante");

	// Intento conectarme con i-MongoStore
	if(crear_conexion(direccion_IP_i_Mongo_Store, puerto_i_Mongo_Store, &i_mongo_store_fd_tripulante) == EXIT_FAILURE){
		log_error(logger, "Submodulo Tripulante: No se pudo establecer la conexion con el i-MongoStore");
		// Libero recursos
		liberar_conexion(i_mongo_store_fd_tripulante);
		sem_post(&sem_struct_iniciar_tripulante);
		return EXIT_FAILURE;
	}

	// Intento conectarme con Mi-Ram HQ
	if(crear_conexion(direccion_IP_Mi_RAM_HQ, puerto_Mi_RAM_HQ, &mi_ram_hq_fd_tripulante) == EXIT_FAILURE){
		log_error(logger, "Submodulo Tripulante: No se pudo establecer la conexion con Mi-Ram HQ");
		// Libero recursos
		liberar_conexion(i_mongo_store_fd_tripulante);
		liberar_conexion(mi_ram_hq_fd_tripulante);
		sem_post(&sem_struct_iniciar_tripulante);
		return EXIT_FAILURE;
	}

	// Le pedimos a Mi-RAM HQ que inicie al tripulante
	estado_envio_mensaje = enviar_op_iniciar_tripulante(i_mongo_store_fd_tripulante, mi_ram_hq_fd_tripulante, struct_iniciar_tripulante);

	if(estado_envio_mensaje != EXIT_SUCCESS){
		log_error(logger, "No se pudo mandar el mensaje al Mi-Ram HQ");
		// Libero recursos
		liberar_conexion(i_mongo_store_fd_tripulante);
		liberar_conexion(mi_ram_hq_fd_tripulante);
		sem_post(&sem_struct_iniciar_tripulante);
		return EXIT_FAILURE;
	}
	
	// Esperamos a la confirmacion de que fue creada con exito
	if(recibir_operacion(mi_ram_hq_fd_tripulante) != COD_INICIAR_TRIPULANTE_OK){
		log_error(logger,"Mi RAM HQ denego la creacion del tripulante");
		// Libero recursos
		liberar_conexion(i_mongo_store_fd_tripulante);
		liberar_conexion(mi_ram_hq_fd_tripulante);
		sem_post(&sem_struct_iniciar_tripulante);
		return EXIT_FAILURE;
	}
	
	sem_post(&sem_struct_iniciar_tripulante);
	
	log_info(logger, "Tripulante inicializado");

	
	// Cada tripulante queda en espera hasta estar iniciado la Planificacion
	sem_wait(&sem_planificacion_fue_iniciada);
	log_debug(logger, "El tripulante %d inicia su planificacion",struct_iniciar_tripulante.TID);
	sem_post(&sem_planificacion_fue_iniciada);
	

	while(status_discordiador != END && estado_tripulante != 'F'){
		//sleep(retardo_ciclo_cpu);

		// Implementacion de semaforo ante una pausa de la Planificacion.
		sem_wait(&sem_planificacion_fue_iniciada);
		sem_post(&sem_planificacion_fue_iniciada);

		// Implementacion de semaforo para Grado de Multitarea/Multiprocesamiento
		sem_wait(&sem_recurso_multitarea_disponible);
		log_debug(logger, "El tripulante %d cambia a estado EXEC", struct_iniciar_tripulante.TID);
		
		// Hay que ver si el servidor esta conectado?
		
		estado_envio_mensaje = enviar_op_recibir_ubicacion_tripulante(mi_ram_hq_fd_tripulante, pos_X, pos_Y);
		if(estado_envio_mensaje != EXIT_SUCCESS)
			log_error(logger, "No se pudo mandar el mensaje a Mi-Ram HQ");
		//pos_X++;
		//pos_Y++;
		//pos_Y++;

		// PIDO LA TAREA A MI RAM HQ
		estado_envio_mensaje = enviar_op_enviar_proxima_tarea(mi_ram_hq_fd_tripulante);
		if(estado_envio_mensaje != EXIT_SUCCESS)
			log_error(logger, "No se pudo mandar el mensaje a Mi-Ram HQ");

		tarea = leer_proxima_tarea_mi_ram_hq(mi_ram_hq_fd_tripulante);

		log_info(logger,"La proxima tarea a ejecutar es:\n%s",tarea);

		
		if(estado_envio_mensaje != EXIT_SUCCESS)
			log_error(logger, "No se pudo mandar el mensaje al i-Mongo-Store");
		
		if(strcmp(tarea,"FIN") == 0){
			estado_tripulante = 'F';
		}else{
			//parseo la tarea
			char ** parametros;
			char ** nombre_parametros;
			int tarea_comun = 0;
			int tiempo_ejecutado = 0;
			parametros = string_split(tarea,";");
			st_tarea.nombre = parametros[0];
			st_tarea.pos_x = parametros[1];
			st_tarea.pos_y = parametros[2];
			st_tarea.duracion = parametros[3];
			//nombre[0]=nombre de la tarea, nombre[1]=parametro numerico
			nombre_parametros = string_split(st_tarea.nombre," ");
			if(nombre_parametros[1]==NULL){
				tarea_comun=1;
			}
			//cada while es 1 sleep/1 rafaga
			int tarea_finalizada = 0;
			int primer_ejecucion = 1;
			while(!tarea_finalizada){
				sleep(retardo_ciclo_cpu);
				
				// Implementacion de semaforo ante una pausa de la Planificacion.
				sem_wait(&sem_planificacion_fue_iniciada);
				sem_post(&sem_planificacion_fue_iniciada);
				
				if(tripulante_esta_en_posicion(&struct_iniciar_tripulante, st_tarea)){
					if(primer_ejecucion){
						//printf("estoy en posicion\n");
						log_info(logger,"Comienzo a %s",nombre_parametros[0]);
						estado_envio_mensaje = enviar_operacion(i_mongo_store_fd_tripulante,COD_EJECUTAR_TAREA, tarea,strlen(tarea)+1);
						if (!tarea_comun){
							agregar_a_buffer_peticiones(buffer_peticiones_exec_to_blocked_io, struct_iniciar_tripulante.TID);
							log_debug(logger, "El tripulante %d cambia a estado BLOCKED IO", struct_iniciar_tripulante.TID);
						}
						primer_ejecucion=0;
					}
					tiempo_ejecutado++;
					if(tiempo_ejecutado==atoi(st_tarea.duracion)){
					tarea_finalizada=1;
					printf("termine la tarea\n");
					}
				}	
				//printf("ejecute 1 seg\n");		
			}
			if (!tarea_comun){
				agregar_a_buffer_peticiones(buffer_peticiones_blocked_io_to_ready, struct_iniciar_tripulante.TID);
				log_debug(logger, "El tripulante %d completo tarea de %d ciclos exitosamente, pasando a estado READY", struct_iniciar_tripulante.TID, atoi(st_tarea.duracion));	
			}
			
		}
		sem_post(&sem_recurso_multitarea_disponible);	

		/*
		// HAGO SLEEP (1 CICLO DE CPU)
		sleep(retardo_ciclo_cpu);

		// Si no estoy en la posicion de la tarea, avanzo a la tarea
		if(pos_actual != pos_tarea)
			// Me muevo un paso
			// Informa a i-MongoStore y Mi RAM HQ que se movio

		else{
			// Estoy en posicion de la tarea
			if(ciclos_cumplidos == 0){
				// Avisa a i-mongostore que arranca la tarea
			}

			// Si todavia no la termine 
			if(ciclos_cumplidos < ciclos_tarea){
				ciclos_cumplidos++;
			}
			else{
				// EJECUTA LA TAREA POSTA
				// AVISA QUE FINALIZO
				// PIDE LA SIGUIENTE TAREA
				ciclos_cumplidos = 0;	// Reinicio ciclos cumplidos
			}
		}
		*/


		/*
		// Notificacion al Planificador/Dispatcher que ingresa a cola EXEC 
		agregar_a_buffer_peticiones(buffer_peticiones_ready_to_exec, struct_iniciar_tripulante.TID);
		
		*/	

		// ¿El tripulante fue expulsado?
		sem_wait(&sem_puede_expulsar_tripulante);
		
		sem_wait(&sem_mutex_tripulante_a_expulsar);
		if(struct_iniciar_tripulante.TID == tripulante_a_expulsar){
			sem_post(&sem_mutex_tripulante_a_expulsar);
			log_debug(logger, "Solicitud de expulsion del TID: %d aceptada", struct_iniciar_tripulante.TID);

			estado_envio_mensaje = enviar_op_expulsar_tripulante(mi_ram_hq_fd_tripulante);
			if(estado_envio_mensaje != EXIT_SUCCESS)
				log_error(logger, "No se pudo mandar el mensaje a Mi-Ram HQ");
			
			sem_wait(&sem_mutex_ejecutar_dispatcher);
			
			if (dispatcher_eliminar_tripulante(struct_iniciar_tripulante.TID) != EXIT_SUCCESS){
				log_error(logger,"No se pudo eliminar de la cola EXIT");
				sem_post(&sem_puede_expulsar_tripulante);
				sem_post(&sem_mutex_ejecutar_dispatcher);
				//sem_post(&sem_recurso_multitarea_disponible);
				return EXIT_FAILURE;
			}
			
			sem_post(&sem_mutex_ejecutar_dispatcher);
			
			log_debug(logger,"Se elimino el tripulante %d exitosamente",struct_iniciar_tripulante.TID);
			sem_post(&sem_confirmar_expulsion_tripulante);
			
			// Libero recursos
			liberar_conexion(i_mongo_store_fd_tripulante);
			liberar_conexion(mi_ram_hq_fd_tripulante);
			
			sem_post(&sem_puede_expulsar_tripulante);
			//sem_post(&sem_recurso_multitarea_disponible);
			
			return EXIT_SUCCESS;
		}
		sem_post(&sem_mutex_tripulante_a_expulsar);
		sem_post(&sem_puede_expulsar_tripulante);
				
		free(tarea);

	}

	// Se desarman las estructuras administrativas del tripulante por FINALIZACION:

	estado_envio_mensaje = enviar_op_expulsar_tripulante(mi_ram_hq_fd_tripulante);
	if(estado_envio_mensaje != EXIT_SUCCESS)
		log_error(logger, "No se pudo mandar el mensaje a Mi-Ram HQ");

	// Habilito al dispatcher a eliminar al tripulante
	sem_wait(&sem_puede_expulsar_tripulante);
	sem_wait(&sem_mutex_ejecutar_dispatcher);
	sem_wait(&sem_mutex_ingreso_tripulantes_new);
	if (dispatcher_expulsar_tripulante(struct_iniciar_tripulante.TID) != EXIT_SUCCESS)
		log_error(logger,"NO se pudo expulsar tripulante %d por finalizacion", struct_iniciar_tripulante.TID);
	if (dispatcher_eliminar_tripulante(struct_iniciar_tripulante.TID) != EXIT_SUCCESS)
		log_error(logger,"No se pudo eliminar de la cola EXIT");
	sem_post(&sem_mutex_ingreso_tripulantes_new);
	sem_post(&sem_mutex_ejecutar_dispatcher);
	sem_post(&sem_puede_expulsar_tripulante);
	log_debug(logger,"Se elimino por finalizacion el tripulante %d exitosamente",struct_iniciar_tripulante.TID);

	// Libero recursos
	liberar_conexion(i_mongo_store_fd_tripulante);
	liberar_conexion(mi_ram_hq_fd_tripulante);

	return EXIT_SUCCESS;
}

char* leer_proxima_tarea_mi_ram_hq(int mi_ram_hq_fd_tripulante){
	char* tarea = NULL;
	int cod_op = recibir_operacion(mi_ram_hq_fd_tripulante);
	switch(cod_op)
	{
		case COD_PROXIMA_TAREA:
			tarea = recibir_payload(mi_ram_hq_fd_tripulante);
			break;
		case -1:
			log_error(logger, "Mi-RAM HQ se desconecto. Terminando discordiador");
			status_discordiador = END;
			break;
		default:
			log_warning(logger, "Operacion desconocida. No quieras meter la pata");
			break;
	}
	return tarea;
}

enum comando_discordiador string_to_comando_discordiador(char* string){
	if(strcmp(string,"INICIAR_PATOTA")==0)
		return INICIAR_PATOTA;
	if(strcmp(string,"LISTAR_TRIPULANTES")==0)
		return LISTAR_TRIPULANTES;
	if(strcmp(string,"EXPULSAR_TRIPULANTE")==0)
		return EXPULSAR_TRIPULANTE;
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

//mueve al tripulante y retorna true cuando esta en posicion
int tripulante_esta_en_posicion(iniciar_tripulante_t* tripulante, t_tarea tarea){
	if(tripulante->posicion_X < atoi(tarea.pos_x)){
		tripulante->posicion_X++;
		printf("me movi a: %dx %dy\n",tripulante->posicion_X,tripulante->posicion_Y);
		return 0;
	}
	if(tripulante->posicion_X > atoi(tarea.pos_x)){
		tripulante->posicion_X--;
		printf("me movi a: %dx %dy\n",tripulante->posicion_X,tripulante->posicion_Y);
		return 0;
	}
	if(tripulante->posicion_Y < atoi(tarea.pos_y)){
		tripulante->posicion_Y++;
		printf("me movi a: %dx %dy\n",tripulante->posicion_X,tripulante->posicion_Y);
		return 0;
	}
	if(tripulante->posicion_Y > atoi(tarea.pos_y)){
		tripulante->posicion_Y--;
		printf("me movi a: %dx %dy\n",tripulante->posicion_X,tripulante->posicion_Y);
		return 0;
	}
	return 1;
}