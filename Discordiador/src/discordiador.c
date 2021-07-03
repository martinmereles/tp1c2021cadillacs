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
	// sem_init(&sem_planificacion_fue_iniciada,0,0);

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

	direccion_IP_i_Mongo_Store = config_get_string_value(config, "IP_I_MONGO_STORE");
	puerto_i_Mongo_Store = config_get_string_value(config, "PUERTO_I_MONGO_STORE");
	direccion_IP_Mi_RAM_HQ = config_get_string_value(config, "IP_MI_RAM_HQ");
	puerto_Mi_RAM_HQ = config_get_string_value(config, "PUERTO_MI_RAM_HQ");
	algoritmo_planificador = config_get_string_value(config, "ALGORITMO");
	grado_multiproc = atoi(config_get_string_value(config, "GRADO_MULTITAREA"));
	duracion_sabotaje = atoi(config_get_string_value(config, "DURACION_SABOTAJE"));
	retardo_ciclo_cpu = atoi(config_get_string_value(config, "RETARDO_CICLO_CPU"));

    if(strcmp(algoritmo_planificador,"RR")==0){
		log_info(logger, "El algoritmo de planificacion es: RR");
		quantum = atoi(config_get_string_value(config, "QUANTUM"));	// Variable global
		log_info(logger, "El quantum es: %d", quantum);
    }
    else{
		if(strcmp(algoritmo_planificador,"FIFO")==0){
			log_info(logger, "El algoritmo de planificacion es: FIFO");
			quantum = 0; // Variable global
		}
		else{
			log_error(logger, "ERROR. Algoritmo de planificacion: %s. No es valido", algoritmo_planificador);
			return EXIT_FAILURE;
		}
    }

	// Inicializo semaforo grado multitarea
	sem_init(&sem_recurso_multitarea_disponible, 0, grado_multiproc);

	// Lista de tripulantes (sirve para poder pausar/reanudar la planificacion)
	lista_tripulantes = list_create();	// La inicializo

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
	list_destroy(lista_tripulantes);	// Lista global para pausar/reanudar planif.
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
			
			// Verificamos la cantidad de argumentos
			if(cantidad_argumentos(argumentos) > 1){
				log_error(logger, "INICIAR_PLANIFICACION: Sobran argumentos");
				break;
			}

            log_info(logger, "Iniciando planificacion");

			// Habilito la planificacion para cada tripulante cuando se ejecuta por 1ra vez o si fue pausado:
			if (estado_planificador == PLANIFICADOR_OFF || estado_planificador == PLANIFICADOR_BLOCKED){		
				// Inicia la ejecucion del HILO Dispatcher: gestor de QUEUEs del Sistema
				iniciar_dispatcher(algoritmo_planificador);
        		reanudar_planificacion();
			}
			else 
				log_debug(logger, "La planificacion ya fue iniciada");

			break;
		case PAUSAR_PLANIFICACION:
			log_info(logger, "Pausando planificacion");
	
			if (estado_planificador == PLANIFICADOR_RUNNING){
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

void reanudar_planificacion(){
	t_list_iterator* iterador = list_iterator_create(lista_tripulantes);
    t_tripulante* tripulante;
    int valor_semaforo;

	// Por cada tripulante, reviso su semaforo. Si estaba bloqueado, lo desbloqueo
    while(list_iterator_has_next(iterador)){
        tripulante = list_iterator_next(iterador);
		log_info(logger, "Reanudo planificacion del tripulante %d", tripulante->TID);
        sem_getvalue(tripulante->sem_planificacion_fue_reanudada, &valor_semaforo);
	    if(valor_semaforo == 0)
			sem_post(tripulante->sem_planificacion_fue_reanudada);
    }

    list_iterator_destroy(iterador);    // Liberamos el iterador
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
	t_tripulante* tripulante;
	t_tarea* tarea = NULL;
	ciclo_t* ciclo;

	int mi_ram_hq_fd_tripulante;
	int i_mongo_store_fd_tripulante;
	int estado_envio_mensaje;

	int ciclos_en_estado_actual;

	int	ciclos_ejecutando_tarea = 0;
	bool llego_a_la_tarea = 1;

	bool tripulante_finalizado = false;

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

	// TEST SERVIDOR i-Mongo-Store
	enviar_mensaje(i_mongo_store_fd_tripulante, "Hola, soy un tripulante!");

	// Le pedimos a Mi-RAM HQ que inicie al tripulante
	estado_envio_mensaje = enviar_op_iniciar_tripulante(i_mongo_store_fd_tripulante, mi_ram_hq_fd_tripulante, struct_iniciar_tripulante);

	if(estado_envio_mensaje != EXIT_SUCCESS){
		log_error(logger, "No se pudo mandar el mensaje a Mi-Ram HQ");
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
	
	// SI EL TRIPULANTE SE INICIO CON EXITO EN LA RAM

	// Agregando nuevo tripulante a cola NEW
	sem_wait(&sem_mutex_ingreso_tripulantes_new);
	tripulante = iniciador_tripulante(struct_iniciar_tripulante.TID, struct_iniciar_tripulante.PID);
	sem_post(&sem_mutex_ingreso_tripulantes_new);
	
	// Habilitamos al siguiente tripulante a inicializar
	sem_post(&sem_struct_iniciar_tripulante);

	log_info(logger, "Tripulante inicializado");
		
	while(status_discordiador != END && !tripulante_finalizado){
			
		// Si la planificacion fue pausada, se bloquea el tripulante
		if(estado_planificador != PLANIFICADOR_RUNNING){
			log_info(logger, "El tripulante %d pauso su planificacion", tripulante->TID);
			sem_wait(tripulante->sem_planificacion_fue_reanudada);
			log_debug(logger, "El tripulante %d inicia su planificacion",tripulante->TID);
		}

		// Si la planificacion no esta pausada
		switch(tripulante->estado){

			case NEW:	// El tripulante espera hasta que el planificacor lo saque de la cola de new
				log_debug(logger, "El tripulante %d llega a NEW", tripulante->TID);
				sem_wait(tripulante->sem_tripulante_dejo_new);
				log_debug(logger, "El tripulante %d sale de NEW", tripulante->TID);
				break;

			case READY:	// El tripulante espera hasta que el planificacor lo saque de la cola de ready
				log_debug(logger, "El tripulante %d llega a READY", tripulante->TID);
				sem_wait(tripulante->sem_tripulante_dejo_ready);
				log_debug(logger, "El tripulante %d sale de READY", tripulante->TID);
				break;

			case BLOCKED_IO:	// Solo hay que esperar los ciclos hasta que se libere
				
				// Me fijo si cumplio los ciclos de la tarea
				if(ciclos_en_estado_actual == tarea->duracion){
					// TODO: Le pide al planificador que lo agregue a la cola de ready
					agregar_a_buffer_peticiones(buffer_peticiones_blocked_io_to_ready, tripulante);

					// El tripulante espera hasta que el planificacor lo saque de la cola de ready
					sem_wait(tripulante->sem_tripulante_dejo_blocked_io);
					ciclos_en_estado_actual = 0;

					// TODO: Avisa que finalizo la tarea
					
					log_debug(logger, "El tripulante %d completo tarea de %d ciclos exitosamente, pasando a estado READY", tripulante->TID, tarea->duracion);	
					destruir_tarea(tarea);
					tarea = NULL;
					break;	// Salgo del switch
				}

				// Se crea un hilo aparte que termina despues de un ciclo
				ciclo = crear_ciclo_cpu();	

				ciclos_en_estado_actual++;
				log_error(logger,"EJECUTO UN CICLO DE LA TAREA EN BLOCKED IO");

				// Se espera que termine el ciclo
				esperar_fin_ciclo_de_cpu(ciclo);					
				break;

			case EXEC:

				// PARA ROUND ROBIN
				// Si al tripulante se le acabo el quantum => pasa a READY
				if(!strcmp(algoritmo_planificador, "RR") && ciclos_en_estado_actual >= quantum){
					log_debug(logger, "Por RR, comenzamos expulsion del tripulante %d de EXEC", tripulante->TID);
					// TODO: Le pide al planificador que lo agregue a la cola de ready
					queue_push(buffer_peticiones_exec_to_ready, tripulante);
					sem_wait(tripulante->sem_tripulante_dejo_exec);
					ciclos_en_estado_actual = 0;
					log_debug(logger, "El tripulante %d sale de EXEC", tripulante->TID);
					break;	// Salgo del switch
				}

				ciclos_en_estado_actual++;	// Cuento 1 ciclo de cpu (para ROUND ROBIN)

				// Inicio 1 ciclo de cpu
				ciclo = crear_ciclo_cpu();	

				// Si no inicie la tarea, la pido (consume 1 ciclo de cpu)
				if(tarea == NULL){
					
					// PIDO LA TAREA A MI RAM HQ
					estado_envio_mensaje = enviar_op_enviar_proxima_tarea(mi_ram_hq_fd_tripulante);
					if(estado_envio_mensaje != EXIT_SUCCESS)
						log_error(logger, "No se pudo mandar el mensaje a Mi-Ram HQ");

					tarea = leer_proxima_tarea_mi_ram_hq(mi_ram_hq_fd_tripulante);

					log_info(logger,"La proxima tarea a ejecutar es:\n%s",tarea->string);

					// Si la tarea leida es la tarea "FIN"
					if(strcmp(tarea->string, "FIN") == 0){
						destruir_tarea(tarea);
						tarea = NULL;
						tripulante->estado = EXIT;
						esperar_fin_ciclo_de_cpu(ciclo);	// Se espera que termine el ciclo
						break;	// Salgo del switch
					}

					// Seteo variables
					ciclos_ejecutando_tarea = 0;	// Cuenta los ciclos en exec para la tarea
					llego_a_la_tarea = false;

					// Se espera que termine el ciclo
					esperar_fin_ciclo_de_cpu(ciclo);
					break;	// Salgo del switch
				}
				
				// Si la tarea fue iniciada pero no se termino

				// Si el tripulante no llego a la tarea, 
				// muevo al tripulante y luego verifico si llego a la tarea
				if(!llego_a_la_tarea && tripulante_esta_en_posicion(tripulante, tarea, mi_ram_hq_fd_tripulante)){
					
					llego_a_la_tarea = true;

					// Si llego a la tarea:				
					log_info(logger,"Comienzo a %s",tarea->nombre);
					estado_envio_mensaje = enviar_operacion(i_mongo_store_fd_tripulante,COD_EJECUTAR_TAREA, tarea->string, strlen(tarea->string)+1);
					
					// Si la tarea es bloqueante, el tripulante se bloquea (TODO: tira error)
					if (tarea->es_bloqueante){
						agregar_a_buffer_peticiones(buffer_peticiones_exec_to_blocked_io, tripulante);
						ciclos_en_estado_actual = 0;
						sem_wait(tripulante->sem_tripulante_dejo_exec);
						log_debug(logger, "El tripulante %d cambia a estado BLOCKED IO", tripulante->TID);
					}

					// Espero que termine el ciclo de cpu
					esperar_fin_ciclo_de_cpu(ciclo);
					break;
				}

				// Si el tripulante llego la tarea 
				if(llego_a_la_tarea){
					ciclos_ejecutando_tarea++;
					log_error(logger,"EJECUTO UN CICLO DE LA TAREA");
				
					if(ciclos_ejecutando_tarea == tarea->duracion){
						printf("termine la tarea\n");
						destruir_tarea(tarea);
						tarea = NULL;
					}	
				}
			
				// Espero que termine el ciclo de cpu
				esperar_fin_ciclo_de_cpu(ciclo);
				break;

			case BLOCKED_EMERGENCY:
				// TODO
				break;

			case EXIT:	// HAY QUE REVISAR
				// Se desarman las estructuras administrativas del tripulante por FINALIZACION:
				estado_envio_mensaje = enviar_op_expulsar_tripulante(mi_ram_hq_fd_tripulante);
				if(estado_envio_mensaje != EXIT_SUCCESS)
					log_error(logger, "No se pudo mandar el mensaje a Mi-Ram HQ");

				// Habilito al dispatcher a eliminar al tripulante
				sem_wait(&sem_puede_expulsar_tripulante);
				sem_wait(&sem_mutex_ejecutar_dispatcher);
				sem_wait(&sem_mutex_ingreso_tripulantes_new);
				if (dispatcher_expulsar_tripulante(tripulante->TID) != EXIT_SUCCESS)
					log_error(logger,"NO se pudo expulsar tripulante %d por finalizacion", tripulante->TID);
				if (dispatcher_eliminar_tripulante(tripulante->TID) != EXIT_SUCCESS)
					log_error(logger,"No se pudo eliminar de la cola EXIT");
				sem_post(&sem_mutex_ingreso_tripulantes_new);
				sem_post(&sem_mutex_ejecutar_dispatcher);
				sem_post(&sem_puede_expulsar_tripulante);
				
				tripulante_finalizado = true;
				
				log_debug(logger,"Se elimino por finalizacion el tripulante %d exitosamente",tripulante->TID);
				break;
			default:
				log_error(logger, "ERROR. submodulo_tripulante: estado erroneo");
				break;
		}// FIN SWITCH
	}// FIN WHILE

	// Libero recursos
	if(tarea != NULL)
		destruir_tarea(tarea);
	liberar_conexion(i_mongo_store_fd_tripulante);
	liberar_conexion(mi_ram_hq_fd_tripulante);

	return EXIT_SUCCESS;
}

t_tarea* leer_proxima_tarea_mi_ram_hq(int mi_ram_hq_fd_tripulante){
	t_tarea* tarea = NULL;
	char* payload = NULL;
	int cod_op = recibir_operacion(mi_ram_hq_fd_tripulante);
	switch(cod_op)
	{
		case COD_PROXIMA_TAREA:
			payload = recibir_payload(mi_ram_hq_fd_tripulante);
			tarea = crear_tarea(payload);
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

// Funciones para crear/destruir una tarea
t_tarea* crear_tarea(char* string){
	t_tarea* tarea = malloc(sizeof(t_tarea));
	tarea->string = string;
    tarea->nombre = NULL;
    tarea->parametro = NULL;

	// Si la tarea leida es la tarea "FIN"
	if(strcmp(tarea->string,"FIN") == 0)
		return tarea;	// Devuelve la tarea solo con el string

	// Si es una tarea normal, la parseo
	char ** parametros;
	char ** primer_parametro;
	parametros = string_split(tarea->string,";");

	// primer_parametro[0]=nombre de la tarea, primer_parametro[1]=parametro numerico
	primer_parametro = string_split(parametros[0]," ");
	tarea->nombre = primer_parametro[0];
	tarea->parametro = primer_parametro[1];
	tarea->pos_x = atoi(parametros[1]);
	tarea->pos_y = atoi(parametros[2]);
	tarea->duracion = atoi(parametros[3]);
	tarea->es_bloqueante = tarea->parametro != NULL;

	// Libero los strings (ya no los uso)
	for(int i = 0;i <= 3;i++)
		free(parametros[i]);

	// Libero los arrays de punteros (no libero primer_parametro[0] ni primer_parametro[1])
	free(parametros);	
	free(primer_parametro);

	return tarea;
}

void destruir_tarea(t_tarea* tarea){
	free(tarea->string);
	if(tarea->nombre != NULL)		free(tarea->nombre);
	if(tarea->parametro != NULL)	free(tarea->parametro);
	free(tarea);
}

void leer_ubicacion_tripulante_mi_ram_hq(int mi_ram_hq_fd_tripulante, int* posicion_X, int* posicion_Y){
	char* payload = NULL;
	int cod_op = recibir_operacion(mi_ram_hq_fd_tripulante);
	switch(cod_op)
	{
		case COD_UBICACION_TRIPULANTE:
			payload = recibir_payload(mi_ram_hq_fd_tripulante);

			int offset = 0;
		
			memcpy(posicion_X, payload + offset, sizeof(uint32_t));
			offset += sizeof(uint32_t);

			memcpy(posicion_Y, payload + offset, sizeof(uint32_t));
			offset += sizeof(uint32_t);

			break;
		case -1:
			log_error(logger, "Mi-RAM HQ se desconecto. Terminando discordiador");
			status_discordiador = END;
			break;
		default:
			log_warning(logger, "Operacion desconocida. No quieras meter la pata");
			break;
	}
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

// mueve al tripulante y retorna true si luego de moverlo esta en posicion
bool tripulante_esta_en_posicion(t_tripulante* tripulante, t_tarea* tarea, int mi_ram_hq_fd_tripulante){

	// PIDO LA POSICION A MI RAM HQ
	int estado_envio_mensaje = enviar_op_enviar_ubicacion_tripulante(mi_ram_hq_fd_tripulante);
	if(estado_envio_mensaje != EXIT_SUCCESS)
		log_error(logger, "No se pudo mandar el mensaje a Mi-Ram HQ");

	leer_ubicacion_tripulante_mi_ram_hq(mi_ram_hq_fd_tripulante, &(tripulante->posicion_X), &(tripulante->posicion_Y));
	printf("Tripulante %d: Estoy en la posicion (%d,%d)\n", tripulante->TID, tripulante->posicion_X, tripulante->posicion_Y);
	printf("Tripulante %d: La tarea esta en (%d,%d)\n", tripulante->TID, tarea->pos_x, tarea->pos_y);

	// Si ya esta en la posicion
	if((tripulante->posicion_X == tarea->pos_x) && (tripulante->posicion_Y == tarea->pos_y))
		return true;

	// Si no lo esta, lo desplazo una casilla
	switch(1){
		default:
			if(tripulante->posicion_X < tarea->pos_x){
				tripulante->posicion_X++;
				break;
			}
			if(tripulante->posicion_X > tarea->pos_x){
				tripulante->posicion_X--;
				break;
			}
			if(tripulante->posicion_Y < tarea->pos_y){
				tripulante->posicion_Y++;
				break;
			}
			if(tripulante->posicion_Y > tarea->pos_y){
				tripulante->posicion_Y--;
				break;
			}
	}
	
	// Actualizo posicion en la memoria RAM
	// Hay que ver si el servidor esta conectado?
	estado_envio_mensaje = enviar_op_recibir_ubicacion_tripulante(mi_ram_hq_fd_tripulante, tripulante->posicion_X, tripulante->posicion_Y);
	if(estado_envio_mensaje != EXIT_SUCCESS)
		log_error(logger, "No se pudo mandar el mensaje a Mi-Ram HQ");

	printf("me movi a: %dx %dy\n",tripulante->posicion_X,tripulante->posicion_Y);

	// Devuelvo si el tripulante llego o no a la posicion
	return (tripulante->posicion_X == tarea->pos_x) && (tripulante->posicion_Y == tarea->pos_y);
}

// Funciones ciclo cpu
void esperar_retardo_ciclo_cpu(){
	sleep(retardo_ciclo_cpu);
}

ciclo_t* crear_ciclo_cpu(){
	ciclo_t* ciclo = malloc(sizeof(ciclo_t));
	pthread_create(&(ciclo->hilo), NULL, (void*) esperar_retardo_ciclo_cpu, NULL);
	return ciclo;
}

void esperar_fin_ciclo_de_cpu(ciclo_t* ciclo){
	pthread_join(ciclo->hilo, NULL);
	free(ciclo);
}