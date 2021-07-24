#include "discordiador.h"

t_tarea tarea_sabotaje;

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
	t_config *config_general = config_create("./cfg/config_general.config");
	t_config *config_test = config_create(path_config);
	free(path_config);

	// Creo logger
	logger = log_create("./cfg/discordiador.log", "Discordiador", 1, LOG_LEVEL_DEBUG);
	log_info(logger, "Inicializando el Discordiador");

	// Inicializo variables globales
	status_discordiador = RUNNING;

	generadorPID = 0;
	generadorTID = 0;

	sem_init(&sem_generador_PID, 0, 1);
	sem_init(&sem_generador_TID, 0, 1);
	sem_init(&sem_struct_iniciar_tripulante, 0, 0);
	//semaforos sabotaje
	sem_init(&sem_sabotaje_finalizado,0,0);
	//sem_init(&sem_sabotaje_tripulante,0,0);
	//sem_init(&sem_tripulante_disponible,0,0);

	int i_mongo_store_fd;
	int mi_ram_hq_fd;

	// Leo IP y PUERTO del config
	log_info(logger, "Iniciando configuracion");

	direccion_IP_i_Mongo_Store = config_get_string_value(config_general, "IP_I_MONGO_STORE");
	puerto_i_Mongo_Store = config_get_string_value(config_general, "PUERTO_I_MONGO_STORE");
	direccion_IP_Mi_RAM_HQ = config_get_string_value(config_general, "IP_MI_RAM_HQ");
	puerto_Mi_RAM_HQ = config_get_string_value(config_general, "PUERTO_MI_RAM_HQ");
	grado_multiproc = atoi(config_get_string_value(config_test, "GRADO_MULTITAREA"));
	duracion_sabotaje = atoi(config_get_string_value(config_test, "DURACION_SABOTAJE"));
	retardo_ciclo_cpu = atoi(config_get_string_value(config_test, "RETARDO_CICLO_CPU"));
	path_tareas = config_get_string_value(config_general, "PATH_TAREAS");

	// Creo estructuras del planificador
	crear_estructuras_planificador();

	if(inicializar_algoritmo_planificacion(config_test) == EXIT_FAILURE)
		return EXIT_FAILURE;

	log_info(logger, "Configuracion terminada");

	log_info(logger, "Conectando con i-Mongo-Store");
	// Intento conectarme con i-Mongo-Store
	if(crear_conexion(direccion_IP_i_Mongo_Store, puerto_i_Mongo_Store, &i_mongo_store_fd) == EXIT_FAILURE){
		log_error(logger, "No se pudo establecer la conexion con el i-Mongo-Store");
		// Libero recursos
		liberar_conexion(i_mongo_store_fd);
		log_destroy(logger);
		config_destroy(config_general);
		config_destroy(config_test);
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
		config_destroy(config_general);
		config_destroy(config_test);
		return EXIT_FAILURE;
	}
	log_info(logger, "Conexion con Mi-RAM HQ exitosa");

	leer_fds(i_mongo_store_fd, mi_ram_hq_fd);

	// Espero a que finalice el planificador
	pthread_join(hilo_dispatcher, NULL);

	// Libero recursos
	log_destroy(logger);
	config_destroy(config_general);
	config_destroy(config_test);
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
	char * bitacora;
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
			bitacora = recibir_payload(i_mongo_store_fd);
			log_info(logger, bitacora);
			/*char ** lista_bitacora = string_split(bitacora,";");
			for(int i =0 ;lista_bitacora[i]!=NULL ;i++){
				log_info(logger, lista_bitacora[i]);
			}*/
			free(bitacora);
			break;
		case COD_MANEJAR_SABOTAJE:;
			char * payload = recibir_payload(i_mongo_store_fd);
			//printf("payload:%s\n", payload);
			char ** posiciones = string_split(payload,"|");
			posicion_sabotaje.pos_x=atoi(posiciones[0]);
			posicion_sabotaje.pos_y=atoi(posiciones[1]);
			//printf("empiezo a planificar el sabotaje\n");
			tarea_sabotaje.pos_x=posicion_sabotaje.pos_x;
			tarea_sabotaje.pos_y=posicion_sabotaje.pos_y;
			//printf("HOLA\n");
			tarea_sabotaje.duracion=duracion_sabotaje;
			/*log_info(logger, "Pausando planificacion");
			if (estado_planificador == PLANIFICADOR_RUNNING){
				dispatcher_pausar();
				log_debug(logger, "Fue pausado exitosamente");
			}else 
				log_debug(logger, "Ya esta pausado");*/
			
			log_info(logger,"Bloqueando tripulantes por sabotaje");
			if (bloquear_tripulantes_por_sabotaje() != EXIT_SUCCESS){
                log_error(logger, "No se pudo ejecutar la funcion de bloqueo ante sabotajes");
				break;
			}
			
            tripulante_sabotaje= malloc(sizeof(t_tripulante));
			tripulante_sabotaje->posicion_X=1000;
			tripulante_sabotaje->posicion_Y=1000;
			tripulante_sabotaje->TID=1000;
			//printf("tripulante elegido tid %d\n",tripulante_sabotaje->TID);
			t_queue *listado_tripulantes_sabotaje;
			t_queue *temporal;
			listado_tripulantes_sabotaje = queue_create();
			temporal = queue_create();
			//printf("cree queues\n");
			temporal->elements = list_duplicate(cola[BLOCKED_EMERGENCY]->elements);
			//printf("duplico cola bloqued emergency\n");
			while(queue_size(temporal)>0){
				queue_push(listado_tripulantes_sabotaje,queue_pop(temporal));
			}
			//printf("iterar lista\n");
			list_iterate(listado_tripulantes_sabotaje->elements,tripulante_mas_cercano);
			//printf("tripulante %d es el indicado para el sabotaje\n",tripulante_sabotaje->TID);
			char * trip_elegido = string_from_format("El tripulante %d es el indicado para el sabotaje\n", tripulante_sabotaje->TID);
			log_info(logger,trip_elegido);
			free(trip_elegido);
			desencolar_tripulante_por_tid(cola[BLOCKED_EMERGENCY], tripulante_sabotaje->TID);
            transicion(tripulante_sabotaje, BLOCKED_EMERGENCY, EXEC);
			sem_post(tripulante_sabotaje->sem_planificacion_fue_reanudada);
			queue_destroy_and_destroy_elements(temporal,free);

			sem_wait(&sem_sabotaje_finalizado);
			desencolar_tripulante_por_tid(cola[EXEC], tripulante_sabotaje->TID);
			transicion(tripulante_sabotaje, EXEC, BLOCKED_EMERGENCY);
			log_info(logger,"Desbloqueando tripulantes al terminar el sabotaje");
			desbloquear_tripulantes_tras_sabotaje();
		
            log_debug(logger, "Termino sabotaje"); 
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

void tripulante_mas_cercano(void *data){
	//printf("entre en funcion\n");
    t_tripulante * tripulante_nuevo = data;
	//printf("tripula sabotaje pos x:%d, y:%d, TID:%d\n",tripulante_sabotaje->posicion_X, tripulante_sabotaje->posicion_Y, tripulante_sabotaje->TID);
	//printf("nuevo tripulante pos x:%d, y:%d, TID:%d\n",tripulante_nuevo->posicion_X, tripulante_nuevo->posicion_Y, tripulante_nuevo->TID);
	int dif_x_anterior, dif_y_anterior, dif_x_nueva, dif_y_nueva, dif_total_anterior, dif_total_nueva;
	dif_x_anterior = abs(posicion_sabotaje.pos_x - tripulante_sabotaje->posicion_X);
	dif_y_anterior = abs(posicion_sabotaje.pos_y - tripulante_sabotaje->posicion_Y);
	dif_x_nueva =abs(posicion_sabotaje.pos_x - tripulante_nuevo->posicion_X);
	dif_y_nueva =abs(posicion_sabotaje.pos_y - tripulante_nuevo->posicion_Y);
	dif_total_anterior = dif_x_anterior + dif_y_anterior;
	dif_total_nueva = dif_x_nueva + dif_y_nueva;
	if(dif_total_nueva<dif_total_anterior || ( dif_total_nueva==dif_total_anterior && tripulante_nuevo->TID < tripulante_sabotaje->TID)){
		tripulante_sabotaje = tripulante_nuevo;
	}
}


void leer_consola_y_procesar(int i_mongo_store_fd, int mi_ram_hq_fd) {
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
		case EXPULSAR_TRIPULANTE:
			// Verificamos la cantidad de argumentos
			if(cantidad_argumentos(argumentos) > 2){
				log_error(logger, "EXPULSAR_TRIPULANTE: Sobran argumentos");
				break;
			}

			if(cantidad_argumentos(argumentos) < 2){
				log_error(logger, "EXPULSAR_TRIPULANTE: Faltan argumentos");
				break;
			}

			expulsar_tripulante(atoi(argumentos[1]));
			break;
		case INICIAR_PLANIFICACION:
			
			// Verificamos la cantidad de argumentos
			if(cantidad_argumentos(argumentos) > 1){
				log_error(logger, "INICIAR_PLANIFICACION: Sobran argumentos");
				break;
			}

            log_info(logger, "Iniciando planificacion");

			// Habilito la planificacion para cada tripulante cuando se ejecuta por 1ra vez o si fue pausado:
			if (estado_planificador != PLANIFICADOR_RUNNING){		
				// Inicia la ejecucion del HILO Dispatcher: gestor de QUEUEs del Sistema
				iniciar_dispatcher();
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
		case FINALIZAR:
			log_info(logger, "Finalizando Discordiador");	
			if(estado_planificador != PLANIFICADOR_RUNNING){
				iniciar_dispatcher();
				finalizar_discordiador();
				reanudar_planificacion();
			}
			else
				finalizar_discordiador();
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

bool planificacion_pausada(){
	t_list_iterator* iterador = list_iterator_create(lista_tripulantes);
    t_tripulante* tripulante;
    int valor_semaforo;

	// Por cada tripulante, reviso su semaforo para confirmar que este pausado
    while(list_iterator_has_next(iterador)){
        tripulante = list_iterator_next(iterador);
		log_info(logger, "Confirmando pausa de %d", tripulante->TID);
        sem_getvalue(tripulante->sem_planificacion_fue_reanudada, &valor_semaforo);
	    if(valor_semaforo != 0){
			list_iterator_destroy(iterador); 
			return false;
		}
    }

    list_iterator_destroy(iterador); 
	return true;
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
	char* path_archivo = string_duplicate(path_tareas);
	string_append(&path_archivo, argumentos[2]);
	archivo_de_tareas = fopen(path_archivo, "r");
	if(archivo_de_tareas == NULL){
		log_error(logger, "INICIAR_PATOTA: %s: No existe el archivo",argumentos[2]);
		free(path_archivo);
		free(lista_de_tareas);
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
	free(path_archivo);
	free(lista_de_tareas);
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

	int ciclos_en_estado_actual = 0;

	int	ciclos_ejecutando_tarea = 0;

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
	tripulante = iniciador_tripulante(struct_iniciar_tripulante.TID, struct_iniciar_tripulante.PID,
									struct_iniciar_tripulante.posicion_X, struct_iniciar_tripulante.posicion_Y);
	sem_post(&sem_mutex_ingreso_tripulantes_new);
	
	// Habilitamos al siguiente tripulante a inicializar
	sem_post(&sem_struct_iniciar_tripulante);

	log_info(logger, "Tripulante inicializado");
	bool primer_ciclo = true;
	bool llego_a_el_sabotaje=false;
	int ciclos_ejecutando_sabotaje=0;
	
	char * indice_tripulante = string_itoa(tripulante->TID);

	enviar_op_recibir_estado_tripulante(mi_ram_hq_fd_tripulante, 'N');
		
	while(!tripulante_finalizado){
			
		// Si la planificacion fue pausada, se bloquea el tripulante
		if(estado_planificador != PLANIFICADOR_RUNNING){
			log_info(logger, "El tripulante %d pauso su planificacion", tripulante->TID);
			sem_wait(tripulante->sem_planificacion_fue_reanudada);
			log_debug(logger, "El tripulante %d inicia su planificacion",tripulante->TID);
		}

		// Si la planificacion no esta pausada
		
		switch(tripulante->estado){

			case NEW:	// El tripulante espera hasta que el planificador lo saque de la cola de new
				encolar(NEW, tripulante);
				sem_post(&sem_hay_evento_planificable);
				sem_wait(tripulante->sem_tripulante_dejo[NEW]);
				break;

			case READY:	// El tripulante espera hasta que el planificacor lo saque de la cola de 
				if(sabotaje_activo){
					desencolar_tripulante_por_tid(cola[READY],tripulante->TID);
					transicion(tripulante,READY,BLOCKED_EMERGENCY);
					//printf("entre en ready sabotaje trip: %d\n",tripulante->TID);
					break;
				}
				enviar_op_recibir_estado_tripulante(mi_ram_hq_fd_tripulante, 'R');	
				if(tarea == NULL){
					tarea = obtener_proxima_tarea(mi_ram_hq_fd_tripulante, tripulante, &ciclos_ejecutando_tarea);
				}
				sem_post(&sem_hay_evento_planificable);
				sem_wait(tripulante->sem_tripulante_dejo[READY]);
				break;
			case BLOCKED_IO:	// Solo hay que esperar los ciclos hasta que se libere
				
				// Si recien llega a BLOCKED_IO
				if(ciclos_en_estado_actual == 0){
					enviar_op_recibir_estado_tripulante(mi_ram_hq_fd_tripulante, 'B');
					// Espera hasta que se libere el dispositivo de entrada salida
					log_info(logger,"T%2d: Esperando disp. E/S", tripulante->TID);	
					sem_wait(tripulante->sem_puede_usar_dispositivo_io);
					log_info(logger,"T%2d: Ocupando disp. E/S", tripulante->TID);
				}

				if(sabotaje_activo){
					//solo pausuo su ejecucion
					sem_wait(tripulante->sem_planificacion_fue_reanudada);
					//implementacion sin pausar a los bloqueados y moviendolo a emergency cuando terminan
					//(NO SERIA LA FORMA CORRECTA)
					/*if(ciclos_en_estado_actual == tarea->duracion){
						// TODO: Le pide al planificador que lo agregue a la cola de ready
						encolar(BLOCKED_IO_TO_READY, tripulante);

						// El tripulante espera hasta que el planificacor lo saque de la cola de ready
						sem_wait(tripulante->sem_tripulante_dejo[BLOCKED_IO]);
						ciclos_en_estado_actual = 0;

						// TODO: Avisa que finalizo la tarea
						
						log_debug(logger, "El tripulante %d completo tarea de %d ciclos exitosamente, pasando a estado READY", tripulante->TID, tarea->duracion);	
						enviar_operacion(i_mongo_store_fd_tripulante, COD_TERMINAR_TAREA, tarea->string, strlen(tarea->string)+1);
						destruir_tarea(tarea);
						tarea = NULL;
						break;	// Salgo del switch
					}

					// Se crea un hilo aparte que termina despues de un ciclo
					ciclo = crear_ciclo_cpu();	

					ciclos_en_estado_actual++;
					log_error(logger,"EJECUTO UN CICLO DE LA TAREA EN BLOCKED IO");

					// Se espera que termine el ciclo
					esperar_fin_ciclo_de_cpu(ciclo);*/					
					break;
				}

				// Me fijo si cumplio los ciclos de la tarea
				if(ciclos_en_estado_actual == tarea->duracion){

					// TODO: Avisa que finalizo la tarea
					log_error(logger,"T%2d: Ciclo Tarea I/O: Tarea completada", tripulante->TID);
					enviar_operacion(i_mongo_store_fd_tripulante, COD_TERMINAR_TAREA, tarea->string, strlen(tarea->string)+1);
					destruir_tarea(tarea);

					// PEDIR TAREA A LA RAM
					tarea = obtener_proxima_tarea(mi_ram_hq_fd_tripulante, tripulante, &ciclos_ejecutando_tarea);

					// TODO: Le pide al planificador que lo agregue a la cola de ready
					encolar(BLOCKED_IO_TO_READY, tripulante);

					// El tripulante espera hasta que el planificacor lo saque de la cola de BLOCKED IO
					sem_post(&sem_hay_evento_planificable);
					sem_wait(tripulante->sem_tripulante_dejo[BLOCKED_IO]);
					ciclos_en_estado_actual = 0;

					break;	// Salgo del switch
				}

				// Se crea un hilo aparte que termina despues de un ciclo
				ciclo = crear_ciclo_cpu();	
				log_error(logger,"T%2d: Ciclo Tarea I/O: Quedan %d", tripulante->TID, tarea->duracion - ciclos_en_estado_actual);			

				ciclos_en_estado_actual++;
				
				// Se espera que termine el ciclo
				esperar_fin_ciclo_de_cpu(ciclo);		
				break;

			case EXEC:
				// printf("imprimo tid\n");
				// printf("soy trip %d y entre en exec\n",tripulante->TID);
				if(tarea == NULL){
					tarea = obtener_proxima_tarea(mi_ram_hq_fd_tripulante, tripulante, &ciclos_ejecutando_tarea);
				}

				if(sabotaje_activo){
					ciclo = crear_ciclo_cpu();
					if(primer_ciclo){
						//printf("soy trip %d y entre a resolver el sabotaje\n",tripulante->TID);
						
						llego_a_el_sabotaje=false;
						ciclos_ejecutando_sabotaje=0;
						primer_ciclo = false;
						
						
					}
					//printf("HOLA\n");
				
					//TO DO: mover tripulante de su posicion a la del 
					// muevo al tripulante y luego verifico si llego a la tarea
					if(!llego_a_el_sabotaje && tripulante_esta_en_posicion(tripulante, &tarea_sabotaje, mi_ram_hq_fd_tripulante, i_mongo_store_fd_tripulante)){
						
						llego_a_el_sabotaje = true;

						// Si llego a el sabotaje:
						//envio el id del tripulante a imongo para facilitar su reconocimiento				
						enviar_operacion(i_mongo_store_fd_tripulante,COD_MANEJAR_SABOTAJE, indice_tripulante,strlen(indice_tripulante)+1);

						// Espero que termine el ciclo de cpu
						esperar_fin_ciclo_de_cpu(ciclo);
						break;
					}

					// Si el tripulante llego la tarea 
					if(llego_a_el_sabotaje){
						//printf("aumento ciclo\n");
						ciclos_ejecutando_sabotaje++;
						log_error(logger,"EJECUTO UN CICLO DE RESOLVER SABOTAJE");
					
						if(ciclos_ejecutando_sabotaje == tarea_sabotaje.duracion){
							//printf("termine la resolver el sabotaje\n");
							//destruir_tarea(tarea_sabotaje);
							sem_post(&sem_sabotaje_finalizado);
							sabotaje_activo = 0;
							llego_a_el_sabotaje = false;
							ciclos_ejecutando_sabotaje = 0;
						}	
					}
					// Espero que termine el ciclo de cpu
					//printf("esperarciclo\n");
					esperar_fin_ciclo_de_cpu(ciclo);
					//revisar semaforos, talvez son innecesarios, no los uso en otros lados y no rompe.
					
					break;
				}
				// printf("soy trip %d y no entre en sabotaje exec\n",tripulante->TID);
				// PARA ROUND ROBIN
				// Si al tripulante se le acabo el quantum => pasa a READY
				if(algoritmo_planificacion == RR && ciclos_en_estado_actual >= quantum){
					// log_debug(logger, "T%2d: Expul. CPU: Fin de quantum", tripulante->TID);
					// TODO: Le pide al planificador que lo agregue a la cola de ready
					encolar(EXEC_TO_READY, tripulante);
					sem_post(&sem_hay_evento_planificable);
					sem_wait(tripulante->sem_tripulante_dejo[EXEC]);
					ciclos_en_estado_actual = 0;
					break;	// Salgo del switch
				}

				// Si recien llega a EXEC
				if(ciclos_en_estado_actual == 0)
					enviar_op_recibir_estado_tripulante(mi_ram_hq_fd_tripulante, 'E');

				/*
				// Si no inicie la tarea, la pido (NO consume ciclo de cpu)
				if(tarea == NULL){			
					// PIDO LA TAREA A MI RAM HQ
					estado_envio_mensaje = enviar_op_enviar_proxima_tarea(mi_ram_hq_fd_tripulante);
					if(estado_envio_mensaje != EXIT_SUCCESS)
						log_error(logger, "No se pudo mandar el mensaje a Mi-Ram HQ");

					tarea = leer_proxima_tarea_mi_ram_hq(mi_ram_hq_fd_tripulante);

					log_info(logger,"T%2d: Prox. tarea: %s", tripulante->TID, tarea->string);

					// Si la tarea leida es la tarea "FIN"
					if(strcmp(tarea->string, "FIN") == 0){
						expulsar_tripulante(tripulante->TID);
						sem_wait(tripulante->sem_tripulante_dejo[EXEC]);
						break;	// Salgo del switch
					}
											
					// Seteo variables
					ciclos_ejecutando_tarea = 0;	// Cuenta los ciclos en exec para la tarea
				}*/

				ciclos_en_estado_actual++;	// Cuento 1 ciclo de cpu (para ROUND ROBIN)

				// Inicio 1 ciclo de cpu
				ciclo = crear_ciclo_cpu();

				// Si el tripulante no esta en la posicion de la tarea,
				// Desplazo al tripulante hacia a la posicion de la tarea (consume 1 ciclo CPU)  
				if(!tripulante_esta_en_posicion(tripulante, tarea, mi_ram_hq_fd_tripulante, i_mongo_store_fd_tripulante)){

					// TODO: Desplazar al tripulante
					// desplazar_tripulante();
										
					// Espero que termine el ciclo de cpu
					esperar_fin_ciclo_de_cpu(ciclo);
					break;
				}

				// Si el tripulante llego a la tarea y no la inicio
				if(ciclos_ejecutando_tarea == 0){			
					estado_envio_mensaje = enviar_operacion(i_mongo_store_fd_tripulante,COD_EJECUTAR_TAREA, tarea->string, strlen(tarea->string)+1);
					log_info(logger,"T%2d: Iniciando tarea: %s", tripulante->TID, tarea->string);
				}

				// Si el tripulante esta en la posicion de la tarea y es bloqueante
				// El tripulante se bloquea, iniciar peticion de E/S (consume 1 ciclo de CPU)
				if (tarea->es_bloqueante){
					log_info(logger, "T%2d: Iniciando peticion de E/S", tripulante->TID);
					esperar_fin_ciclo_de_cpu(ciclo);
					encolar(EXEC_TO_BLOCKED_IO, tripulante);
					ciclos_en_estado_actual = 0;
					sem_post(&sem_hay_evento_planificable);
					sem_wait(tripulante->sem_tripulante_dejo[EXEC]);
					break;
				}

				// Si el tripulante llego la tarea y no es bloqueante
				log_error(logger,"T%2d: Ciclo Tarea CPU: Quedan %d", tripulante->TID, tarea->duracion - ciclos_ejecutando_tarea);
				ciclos_ejecutando_tarea++;	

				// Espero que termine el ciclo de cpu	
				esperar_fin_ciclo_de_cpu(ciclo);

				if(ciclos_ejecutando_tarea == tarea->duracion){
					log_error(logger,"T%2d: Ciclo Tarea CPU: Tarea finalizada", tripulante->TID, tarea->duracion - ciclos_ejecutando_tarea);
					enviar_operacion(i_mongo_store_fd_tripulante, COD_TERMINAR_TAREA, tarea->string, strlen(tarea->string)+1);
					destruir_tarea(tarea);
					// PEDIR TAREA A LA RAM
					tarea = obtener_proxima_tarea(mi_ram_hq_fd_tripulante, tripulante, &ciclos_ejecutando_tarea);
				}	

				break;

			case BLOCKED_EMERGENCY:;
				//printf("blocked emergency pausa del trip %d\n",tripulante->TID);
				int valor_semaforo;
				sem_getvalue(&sem_tripulante_disponible, &valor_semaforo);
				//printf("valor semaforo:%d \n",valor_semaforo);
				if(valor_semaforo == 0){
					sem_post(&sem_tripulante_disponible);
				}
				
				sem_wait(tripulante->sem_planificacion_fue_reanudada);
				
				// Creo que con pausarlos basta
				break;

			case EXIT:
				// El tripulante espera a que lo quiten de la cola de EXIT		
				sem_post(&sem_hay_evento_planificable);
				sem_wait(tripulante->sem_tripulante_dejo[EXIT]);
				tripulante_finalizado = true;								
				break;
			default:
				log_error(logger, "ERROR. submodulo_tripulante: estado erroneo");
				break;
		}// FIN SWITCH
	}// FIN WHILE

	// CUANDO SALE DE EXIT:

	// Se le pide a la RAM que elimine las estructuras del tripulante
	estado_envio_mensaje = enviar_op_expulsar_tripulante(mi_ram_hq_fd_tripulante);
	if(estado_envio_mensaje != EXIT_SUCCESS)
		log_error(logger, "No se pudo mandar el mensaje a Mi-Ram HQ");

	// Libero recursos
	if(tarea != NULL)
		destruir_tarea(tarea);
	liberar_conexion(i_mongo_store_fd_tripulante);
	liberar_conexion(mi_ram_hq_fd_tripulante);

	// Habilita al dispatcher a eliminar sus estructuras del Discordiador
	sem_post(&sem_hay_evento_planificable);
	sem_post(tripulante->sem_finalizo);
	free(indice_tripulante);

	return EXIT_SUCCESS;
}

int expulsar_tripulante(int numero_tid){
	// Creo un hilo que se encarga de buscar al tripulante y moverlo a la cola de EXIT
	int* TID = malloc(sizeof(int));
	*TID = numero_tid; 
    log_info(logger, "Creando rutina para expulsar tripulante %d", *TID);
	pthread_t hilo_expulsar_tripulante;
	pthread_create(&hilo_expulsar_tripulante, NULL, (void*) rutina_expulsar_tripulante, TID);
	pthread_detach(hilo_expulsar_tripulante);
	return EXIT_SUCCESS;
}


// Si no inicie la tarea, la pido (NO consume ciclo de cpu)
	
t_tarea* obtener_proxima_tarea(int mi_ram_hq_fd_tripulante, t_tripulante* tripulante, int *ciclos_ejecutando_tarea){
	// PIDO LA TAREA A MI RAM HQ
	int estado_envio_mensaje = enviar_op_enviar_proxima_tarea(mi_ram_hq_fd_tripulante);
	if(estado_envio_mensaje != EXIT_SUCCESS)
		log_error(logger, "No se pudo mandar el mensaje a Mi-Ram HQ");

	t_tarea* tarea = leer_proxima_tarea_mi_ram_hq(mi_ram_hq_fd_tripulante);

	log_info(logger,"T%2d: Prox. tarea: %s", tripulante->TID, tarea->string);

	// Si la tarea leida es la tarea "FIN"
	if(strcmp(tarea->string, "FIN") == 0){
		enum estado_tripulante estado_actual = tripulante->estado;
		expulsar_tripulante(tripulante->TID);
		sem_wait(tripulante->sem_tripulante_dejo[estado_actual]);
	}
							
	// Seteo variables
	*ciclos_ejecutando_tarea = 0;	// Cuenta los ciclos en exec para la tarea
	return tarea;
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

void leer_estado_tripulante_mi_ram_hq(int mi_ram_hq_fd_tripulante, t_tripulante* tripulante){
	// Le pido a mi ram hq que envie el estado del tripulante
	/*
	int estado_envio_mensaje = enviar_op_enviar_estado_tripulante(mi_ram_hq_fd_tripulante);
	if(estado_envio_mensaje != EXIT_SUCCESS)
		log_error(logger, "No se pudo mandar el mensaje a Mi-Ram HQ");

	char char_estado = leer_estado_mi_ram_hq(mi_ram_hq_fd_tripulante);

	log_info(logger,"T%2d: Estado actual: %c", tripulante->TID, char_estado);*/
	// Espero a que mi ram hq me envie el estado del tripulante
}

char leer_estado_mi_ram_hq(int mi_ram_hq_fd_tripulante){
	char estado;
	char* payload = NULL;
	int cod_op = recibir_operacion(mi_ram_hq_fd_tripulante);
	switch(cod_op)
	{
		case COD_ESTADO_TRIPULANTE:
			payload = recibir_payload(mi_ram_hq_fd_tripulante);
			memcpy(&estado, payload, sizeof(char));
			free(payload);
			break;
		case -1:
			log_error(logger, "Mi-RAM HQ se desconecto. Terminando discordiador");
			status_discordiador = END;
			break;
		default:
			log_warning(logger, "Operacion desconocida. No quieras meter la pata");
			break;
	}
	return estado;
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
			free(payload);
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
	if(strcmp(string,"FINALIZAR")==0)
		return FINALIZAR;
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

// Si el tripulante no esta en posicion, retorna false y lo mueve
// Si el tripulante esta en posicion, retorna true
bool tripulante_esta_en_posicion(t_tripulante* tripulante, t_tarea* tarea, int mi_ram_hq_fd_tripulante, int i_mongo_store_fd_tripulante){
	// PIDO LA POSICION A MI RAM HQ
	int estado_envio_mensaje = enviar_op_enviar_ubicacion_tripulante(mi_ram_hq_fd_tripulante);
	if(estado_envio_mensaje != EXIT_SUCCESS)
		log_error(logger, "No se pudo mandar el mensaje a Mi-Ram HQ");
	leer_ubicacion_tripulante_mi_ram_hq(mi_ram_hq_fd_tripulante, &(tripulante->posicion_X), &(tripulante->posicion_Y));
	
	int pos_X_vieja = tripulante->posicion_X;
	int pos_Y_vieja = tripulante->posicion_Y;
	
	// printf("Tripulante %d: Estoy en la posicion (%d,%d)\n", tripulante->TID, tripulante->posicion_X, tripulante->posicion_Y);
	// printf("Tripulante %d: La tarea esta en (%d,%d)\n", tripulante->TID, tarea->pos_x, tarea->pos_y);

	// Si ya esta en la posicion
	if((tripulante->posicion_X == tarea->pos_x) && (tripulante->posicion_Y == tarea->pos_y))
		return true;

	//MENSAJE PARA IMONGO
	char * mensaje_i_mongo = string_from_format("Se mueve de %d|%d a ", tripulante->posicion_X, tripulante->posicion_Y);

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


	//ENVIO MENSAJE A IMONGO
	char* mensaje_i_mongo_2 = string_from_format("%d|%d\n", tripulante->posicion_X, tripulante->posicion_Y);
	string_append(&mensaje_i_mongo, mensaje_i_mongo_2);
	// printf("El mensaje para el i-mongo es: %s", mensaje_i_mongo);
	enviar_operacion(i_mongo_store_fd_tripulante, COD_MOVIMIENTO_TRIP, mensaje_i_mongo, strlen(mensaje_i_mongo)+1);
	free(mensaje_i_mongo);
	free(mensaje_i_mongo_2);

	log_error(logger,"T%2d: Mov: (%d,%d) => (%d,%d)", tripulante->TID, pos_X_vieja, pos_Y_vieja,
													tripulante->posicion_X, tripulante->posicion_Y);	

	// Como al principio no estaba en la posicion, retorna false
	return false;
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