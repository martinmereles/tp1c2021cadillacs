#include "paginacion.h"

// TABLAS DE PAGINAS

int crear_patota_paginacion(uint32_t PID, uint32_t longitud_tareas, char* tareas){

    // Creamos la tabla de paginas de la patota
    tabla_paginas_t* tabla_patota = malloc(sizeof(tabla_paginas_t));
    tabla_patota->filas = list_create();
    tabla_patota->proximo_numero_pagina = 0;
    tabla_patota->fragmentacion_interna = 0;

    // Buscamos un espacio en memoria para el PCB y lo reservamos
    uint32_t* direccion_logica_PCB;
    if(reservar_memoria(tabla_patota, TAMANIO_PCB, direccion_logica_PCB) == EXIT_FAILURE){
        log_error(logger, "ERROR. No hay espacio para guardar el PCB de la patota. %d",TAMANIO_PCB);
        destruir_tabla_paginas(tabla_patota);
        return EXIT_FAILURE;
    }

    //log_info(logger,"La cantidad de filas es: %d",cantidad_filas(tabla_patota));
    //log_info(logger,"El nro de segmento es: %d",fila_PCB->numero_segmento);

    // Buscamos un espacio en memoria para las tareas y lo reservamos
    uint32_t* direccion_logica_tareas;
    if(reservar_memoria(tabla_patota, longitud_tareas, direccion_logica_tareas) == EXIT_FAILURE){
        log_error(logger, "ERROR. No hay espacio para guardar las tareas de la patota.");
        destruir_tabla_paginas(tabla_patota);
        return EXIT_FAILURE;
    }

    //log_info(logger,"La cantidad de filas es: %d",cantidad_filas(tabla_patota));
    //log_info(logger,"El nro de segmento es: %d",fila_tareas->numero_segmento);

    log_info(logger, "Direccion logica PCB: %x",direccion_logica_PCB);
    log_info(logger, "Direccion logica tareas: %x",direccion_logica_tareas);

    // Agregar la tabla a la lista de tablas
    list_add(tablas_de_patotas, tabla_patota);

    // Guardo el PID y la direccion logica de las tareas en el PCB
    escribir_memoria_principal(tabla_patota, direccion_logica_PCB, DESPL_PID, &PID, sizeof(uint32_t));
    escribir_memoria_principal(tabla_patota, direccion_logica_PCB, DESPL_TAREAS, &direccion_logica_tareas, sizeof(uint32_t));  

    // Guardo las tareas en el segmento de tareas
    escribir_memoria_principal(tabla_patota, direccion_logica_tareas, 0, tareas, longitud_tareas);
    log_info(logger, "Estructuras de la patota inicializadas exitosamente");

    return EXIT_SUCCESS;
}

int crear_tripulante_segmentacion(void** tabla, uint32_t* dir_log_tcb, 
    uint32_t PID, uint32_t TID, uint32_t posicion_X, uint32_t posicion_Y){

    // Buscar la tabla que coincide con el PID
    *tabla = obtener_tabla_patota(PID);
    if(*tabla == NULL){
        log_error(logger,"ERROR. No se encontro la tabla de segmentos de la patota");
        return EXIT_FAILURE;
    }

    // Buscamos un espacio en memoria para el TCB y lo reservamos
    uint32_t* direccion_logica_TCB;
    if(reservar_memoria(tabla_patota, TAMANIO_TCB, direccion_logica_TCB) == EXIT_FAILURE){
        log_error(logger, "ERROR. No hay espacio para guardar el TCB del tripulante. %d",TAMANIO_TCB);
        destruir_tabla_paginas(tabla_patota);
        return EXIT_FAILURE;
    }

    // Guardo el TID, el estado, la posicion, el identificador de la proxima instruccion y la direccion logica del PCB
    char estado = 'N';
    uint32_t id_proxima_instruccion = 0;
    uint32_t dir_log_pcb = DIR_LOG_PCB;
    escribir_memoria_principal(*tabla, *direccion_logica_TCB, DESPL_TID, &TID, sizeof(uint32_t));
    escribir_memoria_principal(*tabla, *direccion_logica_TCB, DESPL_ESTADO, &estado, sizeof(char));
    escribir_memoria_principal(*tabla, *direccion_logica_TCB, DESPL_POS_X, &posicion_X, sizeof(uint32_t));
    escribir_memoria_principal(*tabla, *direccion_logica_TCB, DESPL_POS_Y, &posicion_Y, sizeof(uint32_t));
    escribir_memoria_principal(*tabla, *direccion_logica_TCB, DESPL_PROX_INSTR, &id_proxima_instruccion, sizeof(uint32_t));
    escribir_memoria_principal(*tabla, *direccion_logica_TCB, DESPL_DIR_PCB, &dir_log_pcb, sizeof(uint32_t));

    return EXIT_SUCCESS;
}

int reservar_memoria(tabla_paginas_t* tabla_patota, int tamanio, uint32_t* direccion_logica){
    if(tamanio > 0 && tabla_patota->fragmentacion_interna == 0){
        crear_pagina(tabla_patota);
    }
}

int crear_pagina(tabla_paginas_t* tabla_patota){
    // Reservamos en el mapa de memoria disponbile un marco
    int n_marco = 0;
    
    // Marco por marco, busco el primero que este disponible
    while(bitarray_test_bit(mapa_memoria_disponible, n_marco) && n_marco < cantidad_marcos)
        n_marco++;
    
    // Si no hay marcos disponibles
    if(n_marco == cantidad_marcos){
        log_error(logger, "ERROR: crear_pagina. No hay marcos disponibles.");
        return EXIT_FAILURE;
    }

    // Si hay un marco disponible, lo reservamos
    bitarray_set_bit(mapa_memoria_disponible, n_marco);

    // Creamos una fila en la tabla de la patota
    fila_tabla_paginas_t* fila = malloc(sizeof(fila_tabla_paginas_t));
    fila->numero_marco = n_marco;
    fila->bit_presencia = true;
    fila->bit_modificado = false;

    //tabla_patota->proximo_numero_pagina++;
    tabla_patota->fragmentacion_interna = tamanio_pagina;

    typedef struct{
        uint32_t numero_marco;
        bool bit_de_presencia;
        bool bit_de_modificado;
    } fila_tabla_paginas_t;

    typedef struct{
        t_list *filas;
        uint32_t proximo_numero_pagina; // Hace falta??
        int fragmentacion_interna;
    } tabla_paginas_t;
    

    return EXIT_SUCCESS;
}

/*
// DUMP MEMORIA

void dump_memoria_paginacion(){
    // Dump real
    log_info(logger,"INICIANDO DUMP");

    // Obtenemos la fecha
    char* fecha = temporal_get_string_time("%d_%m_%y_%H_%M_%S");
    
    // Creamos el archivo
    char* nombre_archivo = string_from_format("./dump/Dump_%s.dmp", fecha);
    FILE* archivo_dump = fopen(nombre_archivo, "w");
    free(nombre_archivo);
    free(fecha);

    fecha = temporal_get_string_time("%d/%m/%y %H:%M:%S");
    // Escribimos info general del dump
    char* info_dump = string_from_format("Dump: %s", fecha);
    fwrite(info_dump, sizeof(char), strlen(info_dump), archivo_dump);
    free(info_dump);

    free(fecha);    // Liberamos el string fecha

    // Por cada patota, escribimos la informacion correspondiente
    t_list_iterator* iterador = list_iterator_create(tablas_de_patotas);    // Creamos el iterador
    tabla_segmentos_t* tabla_patota;

    //log_info(logger,"LA CANTIDAD DE PATOTAS ES: %d", list_size(tablas_de_patotas));

    while(list_iterator_has_next(iterador)){
        tabla_patota = list_iterator_next(iterador);
        dump_patota_segmentacion(tabla_patota, archivo_dump);
    }

    list_iterator_destroy(iterador);    // Liberamos el iterador

    // Cerramos el archivo
    fclose(archivo_dump);
}

void dump_patota_paginacion(tabla_segmentos_t* tabla_patota, FILE* archivo_dump){

    // Obtenemos el PID
    uint32_t PID;
    leer_memoria_principal(tabla_patota, DIR_LOG_PCB + DESPL_PID, &PID, sizeof(uint32_t));
    //log_info(logger,"Dumpeamos patota %d",PID);

    // Por cada segmento, hacemos un dump de su informacion
    int cant_segmentos = cantidad_filas(tabla_patota);
    for(int nro_segmento = 0; nro_segmento < cant_segmentos; nro_segmento++){
        dump_segmento(archivo_dump, tabla_patota, PID, nro_segmento);
    }
}

void dump_pagina(FILE* archivo_dump, tabla_segmentos_t* tabla, int PID, int nro_segmento){
    uint32_t inicio = obtener_fila(tabla,nro_segmento)->inicio;
    uint32_t tamanio = obtener_fila(tabla,nro_segmento)->tamanio;

    // Proceso: 1	Segmento: 1	Inicio: 0x0000	Tam: 20b
    char* info_segmento = string_new();                                               // Creamos el string vacio
    string_append(&info_segmento, "\n");                                              // Agregamos salto de linea
    string_append(&info_segmento, string_from_format("Proceso: %d ", PID));           // Agregamos Proceso
    string_append(&info_segmento, string_from_format("Segmento: %d ", nro_segmento)); // Agregamos Segmento
    string_append(&info_segmento, string_from_format("Inicio: %d ", inicio));         // Agregamos Inicio
    string_append(&info_segmento, string_from_format("Tam: %db", tamanio));           // Agregamos Fin
    
    fwrite(info_segmento, sizeof(char), strlen(info_segmento), archivo_dump);   // Escribimos la info en el archivo
    
    free(info_segmento);
}*/