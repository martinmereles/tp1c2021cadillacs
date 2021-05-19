#include "memoria_principal.h"

void inicializar_estructuras_memoria(t_config* config){
	// Inicializo las estructuras para administrar la RAM
	tamanio_memoria = atoi(config_get_string_value(config, "TAMANIO_MEMORIA"));
	memoria_principal = malloc(tamanio_memoria);

    // Inicializo la lista de tablas de segmentos
    tablas_de_segmentos = list_create();

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

tabla_segmentos_t* obtener_tabla_patota(int PID_buscado){
    bool tienePID(void* args){
        tabla_segmentos_t* tabla = (tabla_segmentos_t*) args;
        int PID_tabla;
        leer_memoria_principal(tabla, DIR_LOG_PCB + DESPL_PID, &PID_tabla, sizeof(uint32_t));
        return PID_tabla == PID_buscado;
    }
    return list_find(tablas_de_segmentos, tienePID);
}

fila_tabla_segmentos_t* obtener_fila(tabla_segmentos_t* tabla, int numero_seg){
    return list_get(tabla->filas, numero_seg);
}

// REVISAR
/*
int obtener_tabla_y_direccion_tripulante(uint32_t TID_buscado, tabla_segmentos_t* tabla, uint32_t* dir_log_TCB){
    t_list_iterator* iterador_tablas = list_iterator_create(tablas_de_segmentos);
    fila_tabla_segmentos_t* fila;
    uint32_t TID;

    while(list_iterator_has_next(iterador_tablas)){
        tabla = list_iterator_next(iterador);
        for(int i = 2;i < cantidad_filas(tabla),i++){
            fila = obtener_fila(tabla, i);
            leer_memoria_principal(tabla, fila->inicio + DESPL_TID, &TID, sizeof(uint32_t))
            if(TID == TID_buscado){
                list_iterator_destroy(iterador_tablas);
                return EXIT_SUCCESS;
            }
        }
    }
    tabla = NULL;
    *dir_log_TCB = - 1;
    list_iterator_destroy(iterador_tablas);
    return EXIT_FAILURE;
}*/

tabla_segmentos_t* crear_tabla_segmentos(){
    tabla_segmentos_t* tabla = malloc(sizeof(tabla_segmentos_t));
    tabla->filas = list_create();
    return tabla;
}

void destruir_tabla_segmentos(tabla_segmentos_t* tabla){
    // FALTA LIBERAR LAS FILAS DE SEGMENTOS
    free(tabla->filas);
    free(tabla);
}

int cantidad_filas(tabla_segmentos_t* tabla){
    return list_size(tabla->filas);
}

void quitar_fila(tabla_segmentos_t* tabla, int numero_seg){
    // TODO
}

uint32_t agregar_fila(tabla_segmentos_t* tabla, fila_tabla_segmentos_t* fila){
    list_add(tabla->filas,fila);
    uint32_t numero_fila_nueva = cantidad_filas(tabla);
    uint32_t direccion_logica = (numero_fila_nueva - 1) << 16;
    return direccion_logica;
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
    log_info(logger,"Escribiendo en memoria principal");
    int numero_seg = numero_de_segmento(direccion_logica);
    int direccion_fisica_dato = calcular_direccion_fisica(tabla, direccion_logica);
    fila_tabla_segmentos_t* fila = obtener_fila(tabla, numero_seg);
    if(fila->inicio + fila->tamanio < direccion_fisica_dato + tamanio){
        log_error(logger,"ERROR. Segmentation fault. Direccionamiento invalido en escritura");
        return EXIT_FAILURE;
    }
    memcpy(memoria_principal+direccion_fisica_dato, dato, tamanio);
    return EXIT_SUCCESS;
}

int leer_memoria_principal(tabla_segmentos_t* tabla, uint32_t direccion_logica, void* dato, int tamanio){
    int numero_seg = numero_de_segmento(direccion_logica);
    int direccion_fisica_dato = calcular_direccion_fisica(tabla, direccion_logica);
    fila_tabla_segmentos_t* fila = obtener_fila(tabla,numero_seg);
    if(fila->inicio + fila->tamanio < direccion_fisica_dato + tamanio){
        log_error(logger,"ERROR. Segmentation fault. Direccionamiento invalido en lectura");
        return EXIT_FAILURE;
    }
    memcpy(dato, memoria_principal+direccion_fisica_dato, tamanio);
    return EXIT_SUCCESS;
}

int calcular_direccion_fisica(tabla_segmentos_t* tabla, uint32_t direccion_logica){
    fila_tabla_segmentos_t* fila = obtener_fila(tabla, numero_de_segmento(direccion_logica));
    return fila->inicio + desplazamiento(direccion_logica);
}

uint32_t numero_de_segmento(uint32_t direccion_logica){
    return (direccion_logica & 0xFFFF0000) >> 16;
}

uint32_t desplazamiento(uint32_t direccion_logica){
    return direccion_logica & 0x0000FFFF;
}

void dump_patota(tabla_segmentos_t* tabla_patota){
    // Proceso: 1	Segmento: 1	Inicio: 0x0000	Tam: 20b  
    uint32_t inicio = obtener_fila(tabla_patota, 0)->inicio;
    uint32_t tamanio = obtener_fila(tabla_patota, 0)->tamanio;
    uint32_t PID;
    char* tareas;
    uint32_t direccion_tareas;

    // Mostramos informacion del PCB
    leer_memoria_principal(tabla_patota, DIR_LOG_PCB + DESPL_PID, &PID, sizeof(uint32_t));
    log_info(logger, "Proceso: %d   Segmento: %d    Inicio: %d  Tam: %db",PID,1,inicio,tamanio);

    // Mostramos informacion del segmento de tareas
    inicio = obtener_fila(tabla_patota, 1)->inicio;
    tamanio = obtener_fila(tabla_patota, 1)->tamanio;
    log_info(logger, "Proceso: %d   Segmento: %d    Inicio: %d  Tam: %db",PID,2,inicio,tamanio);

    tareas = malloc(tamanio);
    leer_memoria_principal(tabla_patota, DIR_LOG_PCB + DESPL_TAREAS, &direccion_tareas, sizeof(uint32_t));
    leer_memoria_principal(tabla_patota, direccion_tareas, tareas, tamanio);
    log_info(logger, "Tareas: \n%s",tareas);
    free(tareas);

    // Mostramos informacion de los segmentos de TCBs
    log_info(logger,"TRIPULANTES");
    int cant_filas = cantidad_filas(tabla_patota);
    for(int nro_fila = 2; nro_fila < cant_filas; nro_fila++){
        dump_tripulante(tabla_patota, nro_fila);
    }
}

void dump_tripulante(tabla_segmentos_t* tabla, int nro_fila){
    // Proceso: 1	Segmento: 1	Inicio: 0x0000	Tam: 20b
    log_info(logger,"PATOTA");
    uint32_t inicio = obtener_fila(tabla,nro_fila)->inicio;
    uint32_t tamanio = obtener_fila(tabla,nro_fila)->tamanio;
    uint32_t TID, posicion_X, posicion_Y, id_proxima_instruccion, dir_log_pcb, PID;
    uint32_t dir_log_tcb = nro_fila << 16;
    char estado;
    leer_memoria_principal(tabla, dir_log_tcb + DESPL_TID, &TID, sizeof(uint32_t));    
    leer_memoria_principal(tabla, dir_log_tcb + DESPL_ESTADO, &estado, sizeof(char));
    leer_memoria_principal(tabla, dir_log_tcb + DESPL_POS_X, &posicion_X, sizeof(uint32_t));
    leer_memoria_principal(tabla, dir_log_tcb + DESPL_POS_Y, &posicion_Y, sizeof(uint32_t));
    leer_memoria_principal(tabla, dir_log_tcb + DESPL_PROX_INSTR, &id_proxima_instruccion, sizeof(uint32_t));
    leer_memoria_principal(tabla, dir_log_tcb + DESPL_DIR_PCB, &dir_log_pcb, sizeof(uint32_t));
    leer_memoria_principal(tabla, dir_log_pcb + DESPL_PID, &PID, sizeof(uint32_t));   
 
    log_info(logger, "Tripulane: %d Proceso: %d Inicio: %d Tam: %db",TID,PID,inicio,tamanio);
    log_info(logger, "Posicion: (%d,%d) Proxima instruccion: %d",posicion_X,posicion_Y,id_proxima_instruccion);
}