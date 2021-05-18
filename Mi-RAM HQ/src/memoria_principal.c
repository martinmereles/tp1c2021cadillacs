#include "memoria_principal.h"

void inicializar_estructuras_memoria(t_config* config){
	// Inicializo las estructuras para administrar la RAM
	tamanio_memoria = atoi(config_get_string_value(config, "TAMANIO_MEMORIA"));
	memoria_principal = malloc(tamanio_memoria);

    // Inicializo las listas de tablas de segmentos
    tablas_de_segmentos_de_patotas = list_create();
    tablas_de_segmentos_de_tripulantes = list_create();

	// Cada byte del mapa representa 8 bytes en RAM
	int tamanio_mapa_memoria_disponible = tamanio_memoria/8;
	// Le sumo 1 byte en el caso que el tamanio en bytes de la RAM no sea multiplo de 8
	if(tamanio_memoria%8 != 0)
		tamanio_mapa_memoria_disponible++;
	bitarray_mapa_memoria_disponible = malloc(tamanio_mapa_memoria_disponible);
	mapa_memoria_disponible = bitarray_create_with_mode(bitarray_mapa_memoria_disponible,
														tamanio_mapa_memoria_disponible,
														LSB_FIRST);
	// Inicializo todos los bits del mapa en 0
	for(int i = 0;i < tamanio_memoria;i++){
		bitarray_clean_bit(mapa_memoria_disponible, i);
	}

	log_info(logger, "El tamanio de la RAM es: %d",tamanio_memoria);
	log_info(logger, "El tamanio del mapa de memoria disponible es: %d",bitarray_get_max_bit(mapa_memoria_disponible));
}

void liberar_estructuras_memoria(){
    log_info(logger, "Liberando estructuras administrativas de la memoria principal");
    free(memoria_principal);
    // Destruir listas de tablas
    //      tablas_de_segmentos_de_patotas;
    //      tablas_de_segmentos_de_tripulantes;
    free(bitarray_mapa_memoria_disponible);
    bitarray_destroy(mapa_memoria_disponible);
}

fila_tabla_segmentos_t* reservar_segmento(int tamanio){
    int inicio = first_fit(tamanio);
    fila_tabla_segmentos_t* fila = NULL;
    if(inicio >= 0){
        fila = malloc(sizeof(fila_tabla_segmentos_t));
        fila->inicio = inicio;
        fila->tamanio = tamanio;
        for(int pos = inicio;pos < inicio + tamanio;pos++){
            bitarray_set_bit(mapa_memoria_disponible, pos);
        }
    }
    return fila;
}

void liberar_segmento(fila_tabla_segmentos_t* fila){
    int inicio = fila->inicio;
    int tamanio = fila->tamanio;
    for(int pos = inicio;pos < inicio + tamanio;pos++){
        bitarray_clean_bit(mapa_memoria_disponible, pos);
    }
    free(fila);
}

int first_fit(int memoria_pedida) {
    int memoria_disponible = 0;
    int posicion_memoria_disponible = 0;
    
    // Me fijo byte por byte de la memoria principal cuales estan disponibles
    for(int nro_byte = 0;nro_byte < tamanio_memoria && memoria_disponible < memoria_pedida;nro_byte++){
        // Si el byte esta ocupado
        if(bitarray_test_bit(mapa_memoria_disponible, nro_byte)){
            memoria_disponible = 0;
            posicion_memoria_disponible = nro_byte + 1;
        }
        // Si el byte esta disponible
        else{
            memoria_disponible++;
        }
	}

    // Si no se encontro un espacio de memoria del tamanio pedido, retorna -1
    if(memoria_disponible != memoria_pedida) {
        return -1;
    }   

    return posicion_memoria_disponible;
}

int escribir_memoria_principal(tabla_segmentos_t* tabla, uint32_t direccion_logica, void* dato, int tamanio){
    int numero_fila = (direccion_logica & 0x80000000) >> 31;
    int direccion_fisica_dato = calcular_direccion_fisica(tabla, direccion_logica);
    if(tabla->filas[numero_fila]->inicio + tabla->filas[numero_fila]->tamanio < direccion_fisica_dato + tamanio){
        log_error(logger,"ERROR. Segmentation fault. Esta intentando escribir en una posicion de memoria no reservada");
        return EXIT_FAILURE;
    }
    memcpy(memoria_principal+direccion_fisica_dato, dato, tamanio);
    return EXIT_SUCCESS;
}

int leer_memoria_principal(tabla_segmentos_t* tabla, uint32_t direccion_logica, void* dato, int tamanio){
    int numero_fila = (direccion_logica & 0x80000000) >> 31;
    int direccion_fisica_dato = calcular_direccion_fisica(tabla, direccion_logica);
    if(tabla->filas[numero_fila]->inicio + tabla->filas[numero_fila]->tamanio < direccion_fisica_dato + tamanio){
        log_error(logger,"ERROR. Esta intentando leer en una posicion de memoria no reservada");
        return EXIT_FAILURE;
    }
    memcpy(dato, memoria_principal+direccion_fisica_dato, tamanio);
    return EXIT_SUCCESS;
}

int calcular_direccion_fisica(tabla_segmentos_t* tabla, uint32_t direccion_logica){
    int numero_fila = (direccion_logica & 0x80000000) >> 31;
    return tabla->filas[numero_fila]->inicio + (direccion_logica & 0x7FFFFFFF);
}

void dump_patota(tabla_segmentos_t* tabla_patota){
    // Proceso: 1	Segmento: 1	Inicio: 0x0000	Tam: 20b  
    uint32_t inicio = tabla_patota->filas[0]->inicio;
    uint32_t tamanio = tabla_patota->filas[0]->tamanio;
    uint32_t PID;
    char* tareas;
    uint32_t direccion_tareas;
    leer_memoria_principal(tabla_patota, DIR_LOG_PCB + DESPL_PID, &PID, sizeof(uint32_t));

    log_info(logger, "Proceso: %d   Segmento: %d    Inicio: %d  Tam: %db",PID,1,inicio,tamanio);
    
    inicio = tabla_patota->filas[1]->inicio;
    tamanio = tabla_patota->filas[1]->tamanio;
    log_info(logger, "Proceso: %d   Segmento: %d    Inicio: %d  Tam: %db",PID,2,inicio,tamanio);

    tareas = malloc(tamanio);
    leer_memoria_principal(tabla_patota, DIR_LOG_PCB + DESPL_TAREAS, &direccion_tareas, sizeof(uint32_t));
    leer_memoria_principal(tabla_patota, direccion_tareas, tareas, tamanio);
    //memcpy(tareas, memoria_principal+inicio, tamanio);
    //tareas[tamanio] = '\0';
    log_info(logger, "Tareas: \n%s",tareas);
    free(tareas);
}

void dump_tripulante(tabla_segmentos_t* tabla_tripulante){
    // Proceso: 1	Segmento: 1	Inicio: 0x0000	Tam: 20b  
    uint32_t inicio = tabla_tripulante->filas[0]->inicio;
    uint32_t tamanio = tabla_tripulante->filas[0]->tamanio;
    uint32_t TID, posicion_X, posicion_Y, id_proxima_instruccion, PID;
    char estado;
    leer_memoria_principal(tabla_tripulante, DIR_LOG_TCB + DESPL_TID, &TID, sizeof(uint32_t));
    
    leer_memoria_principal(tabla_tripulante, DIR_LOG_TCB + DESPL_ESTADO, &estado, sizeof(char));
    leer_memoria_principal(tabla_tripulante, DIR_LOG_TCB + DESPL_POS_X, &posicion_X, sizeof(uint32_t));
    leer_memoria_principal(tabla_tripulante, DIR_LOG_TCB + DESPL_POS_Y, &posicion_Y, sizeof(uint32_t));
    leer_memoria_principal(tabla_tripulante, DIR_LOG_TCB + DESPL_PROX_INSTR, &id_proxima_instruccion, sizeof(uint32_t));
    leer_memoria_principal(tabla_tripulante, DIR_LOG_TCB + DESPL_DIR_PCB, &PID, sizeof(uint32_t));
 
    log_info(logger, "Tripulane: %d Proceso: %d Inicio: %d Tam: %db",TID,PID,inicio,tamanio);
    log_info(logger, "Posicion: (%d,%d) Proxima instruccion: %d",posicion_X,posicion_Y,id_proxima_instruccion);
}