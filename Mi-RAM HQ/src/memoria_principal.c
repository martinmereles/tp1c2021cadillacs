#include "memoria_principal.h"

int inicializar_estructuras_memoria(t_config* config){
	// Inicializo las estructuras para administrar la RAM
	tamanio_memoria = atoi(config_get_string_value(config, "TAMANIO_MEMORIA"));
	memoria_principal = malloc(tamanio_memoria);

    log_info(logger, "El tamanio de la RAM es: %d",tamanio_memoria);

    // Inicializar esquema de memoria
    if(inicializar_esquema_memoria(config) == EXIT_FAILURE)
        return EXIT_FAILURE;
    
    // Inicializo la lista de tablas de patotas
    tablas_de_patotas = list_create();

    // Inicializo semaforos
    sem_init(&reservar_liberar_memoria_mutex, 0, 1);
    sem_init(&mutex_proceso_swap, 0, 1);
    sem_init(&mutex_temporal, 0, 1);

	// Configuramos signal de dump
	signal(SIGUSR1, signal_handler);

    // Inicializo mapa
    inicializar_mapa();

    return EXIT_SUCCESS;
}

int inicializar_esquema_memoria(t_config* config){
    char* string_algoritmo_ubicacion = config_get_string_value(config, "ESQUEMA_MEMORIA");

    if(strcmp(string_algoritmo_ubicacion,"SEGMENTACION")==0){
        log_info(logger,"El esquema de memoria es: SEGMENTACION");

        // Cada byte del mapa representa 8 bytes en RAM
        int tamanio_mapa_memoria_disponible = tamanio_memoria/8;
        // Le sumo 1 byte en el caso que el tamanio en bytes de la RAM no sea multiplo de 8
        if(tamanio_memoria%8 != 0)
            tamanio_mapa_memoria_disponible++;
        bitarray_mapa_memoria_disponible = malloc(tamanio_mapa_memoria_disponible);
        mapa_memoria_disponible = bitarray_create_with_mode(bitarray_mapa_memoria_disponible,
                                                            tamanio_mapa_memoria_disponible,
                                                            LSB_FIRST);

        // Inicializo todos los bits del mapa de memoria disponible en 0
        for(int i = 0;i < tamanio_memoria;i++){
            bitarray_clean_bit(mapa_memoria_disponible, i);
        }

        // Inicializo ALGORITMO UBICACION
        if(inicializar_algoritmo_de_ubicacion(config) == EXIT_FAILURE)
            return EXIT_FAILURE;

        // Inicializo vectores a funciones
        dump_memoria = &dump_memoria_segmentacion;
        crear_patota = &crear_patota_segmentacion;
        crear_tripulante = &crear_tripulante_segmentacion;
        escribir_memoria_principal = &escribir_memoria_principal_segmentacion;
        leer_memoria_principal = &leer_memoria_principal_segmentacion;
        obtener_tabla_patota = &obtener_tabla_patota_segmentacion;
        tamanio_tareas = &tamanio_tareas_segmentacion;
        eliminar_tripulante = &eliminar_tripulante_segmentacion;
        espacio_disponible = &espacio_disponible_segmentacion;

        // Inicializo varialbes globales
        memoria_virtual = NULL;

        // Configuramos signal de compactacion
	    signal(SIGINT, signal_handler);

        log_info(logger, "El tamanio del mapa de memoria disponible es: %d",bitarray_get_max_bit(mapa_memoria_disponible));

        return EXIT_SUCCESS;
    }

    if(strcmp(string_algoritmo_ubicacion,"PAGINACION")==0){
        log_info(logger,"El esquema de memoria es: PAGINACION");

        // Inicializo TAMANIO PAGINA
	    tamanio_pagina = atoi(config_get_string_value(config, "TAMANIO_PAGINA"));

        log_info(logger,"El tamanio de pagina es: %d", tamanio_pagina);

        if(tamanio_memoria % tamanio_pagina != 0){
            log_error(logger,"ERROR. El tamanio de la memoria no es multiplo del tamanio de pagina");
            return EXIT_FAILURE;
        }

        // Inicializo TAMANIO SWAP
        tamanio_swap = atoi(config_get_string_value(config, "TAMANIO_SWAP"));
        log_info(logger,"El tamanio de swap es: %d", tamanio_swap);

        if(tamanio_swap % tamanio_pagina != 0){
            log_error(logger,"ERROR. El tamanio de swap no es multiplo del tamanio de pagina.");
            return EXIT_FAILURE;
        }

        // Inicializo PATH SWAP
        path_swap = config_get_string_value(config, "PATH_SWAP");
        log_info(logger,"El path de swap es: %s", path_swap);

    
        // Abrimos el archivo
        memoria_virtual = fopen(path_swap,"w+");
        truncate(path_swap, tamanio_swap);
        for(int i=0;i<tamanio_swap;i++){
            fwrite("HOLA PAPA\n", sizeof(char), strlen("HOLA PAPA\n"), memoria_virtual);
        }


        if(memoria_virtual == NULL){
            log_error(logger,"ERROR. No se pudo crear el archivo de memoria virtual swap.");
            return EXIT_FAILURE;
        }

        // Inicializo ALGORITMO
        if(inicializar_algoritmo_de_reemplazo(config) == EXIT_FAILURE)
            return EXIT_FAILURE;


        cantidad_marcos_memoria_principal = tamanio_memoria/tamanio_pagina;
        log_info(logger,"La cantidad de marcos en memoria principal es: %d", cantidad_marcos_memoria_principal);

        cantidad_marcos_memoria_virtual = tamanio_swap/tamanio_pagina;
        log_info(logger,"La cantidad de marcos en memoria virtual es: %d", cantidad_marcos_memoria_virtual);

        cantidad_marcos_total = cantidad_marcos_memoria_principal + cantidad_marcos_memoria_virtual;

        //cantidad_marcos_total = cantidad_marcos_memoria_principal;  // Sin memoria virtual

        // Creamos la lista global de marcos
        lista_de_marcos = list_create();
        marco_t* marco;

        // Inicializo los marcos de memoria principal 
        // Al mismo tiempo, inicializo el reloj para algoritmo clock
        aguja_reloj = NULL;
        hora_t *nueva_hora, *primera_hora;

        for(int i = 0;i < cantidad_marcos_memoria_principal;i++){
            marco = malloc(sizeof(marco_t));
            marco->numero_marco = i;
            marco->estado = MARCO_LIBRE;
            marco->bit_presencia = 1;
            marco->timestamp = temporal_get_string_time("%y_%m_%d_%H_%M_%S");
            marco->semaforo = malloc(sizeof(sem_t));
            sem_init(marco->semaforo, 0, 1);
            list_add(lista_de_marcos, marco);

            // Voy agregando al reloj el marco de memoria principal
            nueva_hora = malloc(sizeof(hora_t));
            nueva_hora->marco = marco;
            if(aguja_reloj == NULL)
                primera_hora = nueva_hora;  // La hora creada es la primera
            else
                aguja_reloj->siguiente = nueva_hora;    // Si no es la primera, la hora actual apunta a la nueva hora creada
            aguja_reloj = nueva_hora;                   // La nueva hora pasa a ser la hora actual
        }

        // "Cerramos" el reloj
        aguja_reloj->siguiente = primera_hora;  // La hora actual (la ultima) apunta a la primera hora
        aguja_reloj = primera_hora;             // La aguja queda apuntando a la primera hora (la del primer marco)

        // Inicializo los marcos de memoria virtual
        for(int i = cantidad_marcos_memoria_principal;i < cantidad_marcos_total;i++){
            marco = malloc(sizeof(marco_t));
            marco->numero_marco = i;
            marco->estado = MARCO_LIBRE;
            marco->bit_presencia = 0;
            marco->timestamp = temporal_get_string_time("%y_%m_%d_%H_%M_%S");
            marco->semaforo = malloc(sizeof(sem_t));
            sem_init(marco->semaforo, 0, 1);
            list_add(lista_de_marcos, marco); 
        }

        // Inicializo vectores a funciones
        dump_memoria = &dump_memoria_paginacion;
        crear_patota = &crear_patota_paginacion;
        crear_tripulante = &crear_tripulante_paginacion;
        escribir_memoria_principal = &escribir_memoria_principal_paginacion;
        leer_memoria_principal = &leer_memoria_principal_paginacion;
        obtener_tabla_patota = &obtener_tabla_patota_paginacion;
        tamanio_tareas = &tamanio_tareas_paginacion;
        eliminar_tripulante = &eliminar_tripulante_paginacion;
        espacio_disponible = &espacio_disponible_paginacion;

        return EXIT_SUCCESS;
    }

    log_error(logger,"%s: Esquema de memoria invalido",string_algoritmo_ubicacion);
    return EXIT_FAILURE;
}
   
void liberar_estructuras_memoria(t_config* config){
    log_info(logger, "Liberando estructuras administrativas de la memoria principal");

    if(strcmp(config_get_string_value(config, "ESQUEMA_MEMORIA"),"SEGMENTACION")==0){
        list_destroy_and_destroy_elements(tablas_de_patotas, destruir_tabla_segmentos);
        free(bitarray_mapa_memoria_disponible);
        bitarray_destroy(mapa_memoria_disponible);
    } 

    if(strcmp(config_get_string_value(config, "ESQUEMA_MEMORIA"),"PAGINACION")==0){
        list_destroy_and_destroy_elements(tablas_de_patotas, destruir_tabla_paginas);
        list_destroy_and_destroy_elements(lista_de_marcos, destruir_marco);
        destruir_reloj();
        fclose(memoria_virtual);
        remove(path_swap);
    } 

    free(memoria_principal);
    finalizar_mapa();   // Finalizo mapa
}

void leer_tarea_memoria_principal(void* tabla, char** tarea, uint32_t id_prox_tarea){    
    
    // Obtenemos el tamanio de la lista de tareas
    int tamanio = tamanio_tareas(tabla);

     // Leemos la direccion logica de las tareas
    uint32_t dir_log_tareas;
    leer_memoria_principal(tabla, DIR_LOG_PCB, DESPL_TAREAS, &dir_log_tareas, sizeof(uint32_t));
    
    log_info(logger,"La direccion logica de las tareas es: %x", dir_log_tareas);

    // Leemos la lista de tareas completa
    char* tareas = malloc(tamanio + 1);
    leer_memoria_principal(tabla, dir_log_tareas, 0, tareas, tamanio);
    tareas[tamanio] = '\0';
    log_info(logger,"La lista de tareas es:\n %s",tareas);
    log_info(logger,"El ID de la proxima tarea es: %d",id_prox_tarea);

    // Obtenemos el array de tareas
    char** array_tareas = (char**) string_split(tareas, "\n");
    int cant_tareas = cantidad_tareas(array_tareas);
    if(id_prox_tarea < cant_tareas)
        *tarea = string_duplicate(array_tareas[id_prox_tarea]);
    else{
        if(id_prox_tarea == cant_tareas)
            *tarea = string_duplicate("FIN");
        else{
            *tarea = NULL;
            log_error(logger,"ERROR. No existe la instruccion con el identificador solicitado");
        }
    }

    // Libero el string tareas
    free(tareas);
	
    // Libero el array de tareas
	for(int i = 0;array_tareas[i]!=NULL;i++){
		free(array_tareas[i]);
	}
	free(array_tareas);
}

void signal_handler(int senial){
    log_info(logger,"Signal atendida por el hilo %d",syscall(__NR_gettid));

	switch(senial){
		case SIGUSR1:
            ejecutar_rutina(dump_memoria);
			break;
		case SIGINT:
            ejecutar_rutina(compactacion);
			break;
	}
}