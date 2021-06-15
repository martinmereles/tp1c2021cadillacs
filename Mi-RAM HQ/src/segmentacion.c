#include "segmentacion.h"

// ALGORITMOS DE UBICACION

int inicializar_algoritmo_de_ubicacion(t_config* config){
    char* string_algoritmo_ubicacion = config_get_string_value(config, "ALGORITMO_UBICACION");
    if(strcmp(string_algoritmo_ubicacion,"FIRST_FIT")==0){
        log_info(logger,"El algoritmo de ubicacion es: First Fit");
        algoritmo_de_ubicacion = &first_fit;
        return EXIT_SUCCESS;
    }
    if(strcmp(string_algoritmo_ubicacion,"BEST_FIT")==0){
        log_info(logger,"El algoritmo de ubicacion es: Best Fit");
        algoritmo_de_ubicacion = &best_fit;
        return EXIT_SUCCESS;
    }
    log_error(logger,"%s: Algoritmo de ubicacion invalido",string_algoritmo_ubicacion);
    return EXIT_FAILURE;
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
    if(memoria_disponible != memoria_pedida)
        return -1;
    return posicion_memoria_disponible;
}

int best_fit(int memoria_pedida) {
    int memoria_disponible = 0;
    int posicion_memoria_disponible = 0;
    int mejor_memoria_disponible = 0;
    int mejor_posicion_memoria_disponible = -1;
    //log_info(logger,"ARRANCA EL BEST FIT");
    // Me fijo byte por byte de la memoria principal cuales estan disponibles
    for(int nro_byte = 0;nro_byte < tamanio_memoria;nro_byte++){
        // Si el byte esta ocupado
        if(bitarray_test_bit(mapa_memoria_disponible, nro_byte)){

            // Me fijo si la memoria pedida entra en la memoria hallada
            if(memoria_disponible >= memoria_pedida){
                // Si entra, la comparo con la mejor que habia encontrado
                if(mejor_memoria_disponible > memoria_disponible || mejor_posicion_memoria_disponible == -1){
                    // Si es mejor, se convierte en la nueva mejor
                    mejor_memoria_disponible = memoria_disponible;
                    mejor_posicion_memoria_disponible = posicion_memoria_disponible;
                }
            }

            memoria_disponible = 0;
            posicion_memoria_disponible = nro_byte + 1;
        }
        // Si el byte esta disponible
        else{
            memoria_disponible++;
        }
    }
    // Verifico si hay memoria disponible al final de la memoria principal
    // Me fijo si la memoria pedida entra en la memoria hallada
    if(memoria_disponible >= memoria_pedida){
        // Si entra, la comparo con la mejor que habia encontrado
        if(mejor_memoria_disponible > memoria_disponible || mejor_posicion_memoria_disponible == -1){
            // Si es mejor, se convierte en la nueva mejor
            mejor_memoria_disponible = memoria_disponible;
            mejor_posicion_memoria_disponible = posicion_memoria_disponible;
        }
    }
    //log_info(logger,"El mejor ajuste es de tamanio %d", mejor_memoria_disponible);
    //log_info(logger,"La direccion del mejor ajuste es %d", mejor_posicion_memoria_disponible);
    // Retorno la mejor posicion de memoria encontrada
    // Si no se encontro un espacio de memoria del tamanio pedido, retorna -1
    return mejor_posicion_memoria_disponible;
}

// LECTURA/ESCRITURA EN MEMORIA PRINCIPAL

uint32_t direccion_logica_segmentacion(uint32_t inicio_logico, uint32_t desplazamiento_logico){
    return inicio_logico + desplazamiento_logico;
}

int escribir_memoria_principal_segmentacion(void* args, uint32_t inicio_logico, uint32_t desplazamiento_logico, void* dato, int tamanio){
    tabla_segmentos_t* tabla = (tabla_segmentos_t*) args;
    uint32_t direccion_logica = direccion_logica_segmentacion(inicio_logico, desplazamiento_logico);
    int numero_seg = numero_de_segmento(direccion_logica);
    int direccion_fisica_dato = calcular_direccion_fisica(tabla, direccion_logica);
    segmento_t* fila = obtener_fila(tabla, numero_seg);
    if(fila->inicio + fila->tamanio < direccion_fisica_dato + tamanio){
        log_error(logger,"ERROR. Segmentation fault. Direccionamiento invalido en escritura");
        return EXIT_FAILURE;
    }
    memcpy(memoria_principal+direccion_fisica_dato, dato, tamanio);
    return EXIT_SUCCESS;
}

int leer_memoria_principal_segmentacion(void* args, uint32_t inicio_logico, uint32_t desplazamiento_logico, void* dato, int tamanio){
    tabla_segmentos_t* tabla = (tabla_segmentos_t*) args;
    uint32_t direccion_logica = direccion_logica_segmentacion(inicio_logico, desplazamiento_logico);
    int numero_seg = numero_de_segmento(direccion_logica);
    int direccion_fisica_dato = calcular_direccion_fisica(tabla, direccion_logica);
    segmento_t* fila = obtener_fila(tabla,numero_seg);
    if(fila->inicio + fila->tamanio < direccion_fisica_dato + tamanio){
        log_error(logger,"ERROR. Segmentation fault. Direccionamiento invalido en lectura");
        return EXIT_FAILURE;
    }
    memcpy(dato, memoria_principal+direccion_fisica_dato, tamanio);
    return EXIT_SUCCESS;
}

// TABLAS DE SEGMENTOS

void quitar_y_destruir_tabla(tabla_segmentos_t* tabla_a_destruir){
    int PID_tabla_a_destruir;
    leer_memoria_principal(tabla_a_destruir, DIR_LOG_PCB, DESPL_PID, &PID_tabla_a_destruir, sizeof(uint32_t));

    bool tienePID(void* args){
        tabla_segmentos_t* una_tabla = (tabla_segmentos_t*) args;
        int PID_una_tabla;
        leer_memoria_principal(una_tabla, DIR_LOG_PCB, DESPL_PID, &PID_una_tabla, sizeof(uint32_t));
        return PID_una_tabla == PID_tabla_a_destruir;
    }

    list_remove_and_destroy_by_condition(tablas_de_patotas, tienePID, destruir_tabla_segmentos); 
    // PARA PRUEBAS
    dump_memoria();
}

void destruir_tabla_segmentos(void* args){
    tabla_segmentos_t* tabla = (tabla_segmentos_t*) args;
    // Destruir todas las filas y liberar segmentos;
    list_destroy_and_destroy_elements(tabla->filas, destruir_fila);
    free(tabla);
}

void quitar_y_destruir_fila(tabla_segmentos_t* tabla, int numero_seg){
    bool tiene_numero_de_segmento(void* args){
        segmento_t* fila = (segmento_t*) args;
        return fila->numero_segmento == numero_seg; 
    }
    list_remove_and_destroy_by_condition(tabla->filas, tiene_numero_de_segmento, destruir_fila);
}

tabla_segmentos_t* obtener_tabla_patota_segmentacion(int PID_buscado){
    bool tienePID(void* args){
        tabla_segmentos_t* tabla = (tabla_segmentos_t*) args;
        int PID_tabla;
        leer_memoria_principal(tabla, DIR_LOG_PCB, DESPL_PID, &PID_tabla, sizeof(uint32_t));
        return PID_tabla == PID_buscado;
    }
    return list_find(tablas_de_patotas, tienePID);
}

segmento_t* obtener_fila(tabla_segmentos_t* tabla, int numero_seg){
    bool tiene_numero_de_segmento(void* args){
        segmento_t* fila = (segmento_t*) args;
        return fila->numero_segmento == numero_seg; 
    }

    return list_find(tabla->filas, tiene_numero_de_segmento);
}

int tamanio_tareas_segmentacion(void* args){
    tabla_segmentos_t* tabla = (tabla_segmentos_t*) args;
    return tabla->tamanio_tareas;
}

int crear_patota_segmentacion(uint32_t PID, uint32_t longitud_tareas, char* tareas){

    // Creamos la tabla de segmentos de la patota
    tabla_segmentos_t* tabla_patota = malloc(sizeof(tabla_segmentos_t));
    tabla_patota->filas = list_create();
    tabla_patota->proximo_numero_segmento = 0;

    // Buscamos un espacio en memoria para el PCB y creamos su fila en la tabla de segmentos
    segmento_t* fila_PCB = crear_fila(tabla_patota, TAMANIO_PCB);
    if(fila_PCB == NULL) {
        log_error(logger, "ERROR. No hay espacio para guardar el PCB de la patota. %d",TAMANIO_PCB);
        destruir_tabla_segmentos(tabla_patota);
        return EXIT_FAILURE;
    }
    
    // Buscamos un espacio en memoria para las tareas y creamos su fila en la tabla de segmentos

    segmento_t* fila_tareas = crear_fila(tabla_patota, longitud_tareas);
    if(fila_tareas == NULL) {
        log_error(logger, "ERROR. No hay espacio para guardar las tareas de la patota.");
        destruir_tabla_segmentos(tabla_patota);
        return EXIT_FAILURE;
    }

    // Guardo en la tabla el tamanio de la lista de tareas
    tabla_patota->tamanio_tareas = longitud_tareas;

    int direccion_logica_PCB = direccion_logica_segmento(fila_PCB);
    int direccion_logica_tareas = direccion_logica_segmento(fila_tareas);

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

    // Buscamos un espacio en memoria para el TCB
    // Agregamos la fila para el segmento del TCB a la tabla de la patota
    segmento_t* fila_TCB = crear_fila(*tabla, TAMANIO_TCB);

    if(fila_TCB == NULL) {
        log_error(logger, "ERROR. No hay espacio para guardar el TCB del tripulante. %d",TAMANIO_TCB);
        quitar_y_destruir_fila(*tabla, fila_TCB->numero_segmento);
        return EXIT_FAILURE;
    }

    *dir_log_tcb = direccion_logica_segmento(fila_TCB);

    // Guardo el TID, el estado, la posicion, el identificador de la proxima instruccion y la direccion logica del PCB
    char estado = 'N';
    uint32_t id_proxima_instruccion = 0;
    uint32_t dir_log_pcb = DIR_LOG_PCB;
    escribir_memoria_principal(*tabla, *dir_log_tcb, DESPL_TID, &TID, sizeof(uint32_t));
    escribir_memoria_principal(*tabla, *dir_log_tcb, DESPL_ESTADO, &estado, sizeof(char));
    escribir_memoria_principal(*tabla, *dir_log_tcb, DESPL_POS_X, &posicion_X, sizeof(uint32_t));
    escribir_memoria_principal(*tabla, *dir_log_tcb, DESPL_POS_Y, &posicion_Y, sizeof(uint32_t));
    escribir_memoria_principal(*tabla, *dir_log_tcb, DESPL_PROX_INSTR, &id_proxima_instruccion, sizeof(uint32_t));
    escribir_memoria_principal(*tabla, *dir_log_tcb, DESPL_DIR_PCB, &dir_log_pcb, sizeof(uint32_t));

    return EXIT_SUCCESS;
}

int cantidad_filas(tabla_segmentos_t* tabla){
    return list_size(tabla->filas);
}

void destruir_fila(void* args){
    segmento_t* segmento = (segmento_t*) args;

    // Liberamos el espacio en el mapa de memoria disponible
    liberar_memoria_segmentacion(segmento->inicio, segmento->tamanio);

    free(segmento);
}

segmento_t* crear_fila(tabla_segmentos_t* tabla, int tamanio){
    
    // Buscamos un espacio de memoria por algoritmo
    int inicio = algoritmo_de_ubicacion(tamanio);

    // Si no se encontro espacio
    if(inicio < 0){
        // Ejecutamos la compactacion
        compactacion();

        // Probamos una segunda vez 
        inicio = algoritmo_de_ubicacion(tamanio);
        
        // Si tampoco se encontro espacio
        if(inicio < 0){
            log_error(logger,"ERROR: crear_fila. No hay espacio en memoria disponible.");
            return NULL;
        }
    }

    // Si se encontro espacio
    // Creamos el segmento
    segmento_t* segmento = malloc(sizeof(segmento_t));
    segmento->inicio = inicio;
    segmento->tamanio = tamanio;

    // Reservamos el espacio en el mapa de memoria disponible
    reservar_memoria_segmentacion(inicio, tamanio);    

    // Agregamos el segmento a la tabla
    list_add(tabla->filas,segmento);
    segmento->numero_segmento = generar_nuevo_numero_segmento(tabla);
    return segmento;
}

void reservar_memoria_segmentacion(uint32_t inicio, uint32_t tamanio){
    for(int pos = inicio;pos < inicio + tamanio;pos++){
        bitarray_set_bit(mapa_memoria_disponible, pos);
    }
}

void liberar_memoria_segmentacion(uint32_t inicio, uint32_t tamanio){
    for(int pos = inicio;pos < inicio + tamanio;pos++){
        bitarray_clean_bit(mapa_memoria_disponible, pos);
    }
}

int generar_nuevo_numero_segmento(tabla_segmentos_t* tabla){
    // AGREGAR SEMAFORO?
    int nuevo = tabla->proximo_numero_segmento;
    tabla->proximo_numero_segmento++;
    return nuevo;
}

// DIRECCIONAMIENTO

uint32_t direccion_logica_segmento(segmento_t* fila){
    return fila->numero_segmento << 16;
}

int calcular_direccion_fisica(tabla_segmentos_t* tabla, uint32_t direccion_logica){
    segmento_t* fila = obtener_fila(tabla, numero_de_segmento(direccion_logica));
    if(fila == NULL){
        log_error(logger,"NO EXISTE LA FILA");
    }
    return fila->inicio + get_desplazamiento(direccion_logica);
}

uint32_t numero_de_segmento(uint32_t direccion_logica){
    return (direccion_logica & 0xFFFF0000) >> 16;
}

void eliminar_tripulante_segmentacion(void* tabla, uint32_t direccion_logica_TCB){
    // Quitamos la fila de la tabla de segmentos (tambien se encarga de liberar la memoria)
    quitar_y_destruir_fila(tabla, numero_de_segmento(direccion_logica_TCB));

    // Si no quedan tripulantes en la patota, destruimos su tabla y liberamos sus recursos
    if(cantidad_filas(tabla) <= 2){
        quitar_y_destruir_tabla(tabla);
    }
}

// DUMP MEMORIA

void dump_memoria_segmentacion(){
    crear_archivo_dump(tablas_de_patotas, dump_patota_segmentacion);

    // Para pruebas
    /*
    log_info(logger,"Dump: %s",temporal_get_string_time("%d/%m/%y %H:%M:%S"));
    list_iterate(tablas_de_patotas, dump_patota_segmentacion_pruebas);
    */
}

void dump_patota_segmentacion(void* args, FILE* archivo_dump){
    tabla_segmentos_t* tabla_patota = (tabla_segmentos_t*) args;

    // Obtenemos el PID
    uint32_t PID;
    leer_memoria_principal(tabla_patota, DIR_LOG_PCB, DESPL_PID, &PID, sizeof(uint32_t));

    // Por cada segmento, hacemos un dump de su informacion
    t_list_iterator* iterador = list_iterator_create(tabla_patota->filas);    // Creamos el iterador
    segmento_t* segmento;

    while(list_iterator_has_next(iterador)){
        segmento = list_iterator_next(iterador);
        dump_segmento(segmento, PID, archivo_dump);
    }

    list_iterator_destroy(iterador);    // Liberamos el iterador
}

void dump_segmento(segmento_t* segmento, int PID, FILE* archivo_dump){
    // Proceso: 1	Segmento: 1	Inicio: 0x0000	Tam: 20b
    char* info_segmento = string_from_format("\nProceso: %3d Segmento: %3d Inicio: %8d Tam: %db", 
                                                PID, segmento->numero_segmento, segmento->inicio, segmento->tamanio);
    
    // Escribimos la info en el archivo
    fwrite(info_segmento, sizeof(char), strlen(info_segmento), archivo_dump);   
    
    free(info_segmento);
}

// PSEUDODUMP PARA PRUEBAS

void dump_patota_segmentacion_pruebas(void* args){
    tabla_segmentos_t* tabla_patota = (tabla_segmentos_t*) args;

    // Proceso: 1	Segmento: 1	Inicio: 0x0000	Tam: 20b  
    uint32_t inicio = obtener_fila(tabla_patota, 0)->inicio;
    uint32_t tamanio = obtener_fila(tabla_patota, 0)->tamanio;
    uint32_t PID;
    char* tareas;
    uint32_t direccion_tareas;

    log_info(logger,"PATOTA");

    // Mostramos informacion del PCB
    leer_memoria_principal(tabla_patota, DIR_LOG_PCB, DESPL_PID, &PID, sizeof(uint32_t));
    log_info(logger, "Proceso: %d   Segmento: %d    Inicio: %d  Tam: %db",PID,1,inicio,tamanio);

    // Mostramos informacion del segmento de tareas
    inicio = obtener_fila(tabla_patota, 1)->inicio;
    tamanio = obtener_fila(tabla_patota, 1)->tamanio;
    log_info(logger, "Proceso: %d   Segmento: %d    Inicio: %d  Tam: %db",PID,2,inicio,tamanio);

    tareas = malloc(tamanio);
    leer_memoria_principal(tabla_patota, DIR_LOG_PCB, DESPL_TAREAS, &direccion_tareas, sizeof(uint32_t));
    leer_memoria_principal(tabla_patota, direccion_tareas, 0, tareas, tamanio);
    log_info(logger, "Tareas: \n%s",tareas);
    free(tareas);

    // Mostramos informacion de los segmentos de TCBs
    log_info(logger,"TRIPULANTES");
    int cant_filas = cantidad_filas(tabla_patota);
    for(int nro_fila = 2; nro_fila < cant_filas; nro_fila++){
        dump_tripulante_segmentacion_pruebas(tabla_patota, nro_fila);
    }
}

void dump_tripulante_segmentacion_pruebas(tabla_segmentos_t* tabla, int nro_fila){
    // Proceso: 1	Segmento: 1	Inicio: 0x0000	Tam: 20b
    uint32_t inicio = obtener_fila(tabla,nro_fila)->inicio;
    uint32_t tamanio = obtener_fila(tabla,nro_fila)->tamanio;
    uint32_t TID, posicion_X, posicion_Y, id_proxima_instruccion, dir_log_pcb, PID;
    uint32_t dir_log_tcb = nro_fila << 16;
    char estado;
    leer_memoria_principal(tabla, dir_log_tcb, DESPL_TID, &TID, sizeof(uint32_t));    
    leer_memoria_principal(tabla, dir_log_tcb, DESPL_ESTADO, &estado, sizeof(char));
    leer_memoria_principal(tabla, dir_log_tcb, DESPL_POS_X, &posicion_X, sizeof(uint32_t));
    leer_memoria_principal(tabla, dir_log_tcb, DESPL_POS_Y, &posicion_Y, sizeof(uint32_t));
    leer_memoria_principal(tabla, dir_log_tcb, DESPL_PROX_INSTR, &id_proxima_instruccion, sizeof(uint32_t));
    leer_memoria_principal(tabla, dir_log_tcb, DESPL_DIR_PCB, &dir_log_pcb, sizeof(uint32_t));
    leer_memoria_principal(tabla, dir_log_pcb, DESPL_PID, &PID, sizeof(uint32_t));   
 
    log_info(logger, "Tripulante: %d Proceso: %d Inicio: %d Tam: %db",TID,PID,inicio,tamanio);
    log_info(logger, "Posicion: (%d,%d) Proxima instruccion: %d",posicion_X,posicion_Y,id_proxima_instruccion);
}

void compactacion(){

    log_info(logger,"INICIANDO COMPACTACION");
    
    // Generamos la lista de segmentos ordenada
    t_list* lista_segmentos_memoria = lista_segmentos_en_memoria();
    // Compactamos los segmentos por orden
    list_iterate(lista_segmentos_memoria, compactar_segmento);
    // Destruimos la lista (sin destruir los segmentos)    
    list_destroy(lista_segmentos_memoria);

    log_info(logger,"COMPACTACION COMPLETADA");

    // PARA PRUEBAS
    dump_memoria();
}



void compactar_segmento(void* args){
    segmento_t* segmento = (segmento_t*) args;

    // Copiar contenido de la RAM en un buffer
    void* buffer = malloc(segmento->tamanio);
    memcpy(buffer, memoria_principal + segmento->inicio, segmento->tamanio);

    // Liberar memoria en el mapa
    liberar_memoria_segmentacion(segmento->inicio, segmento->tamanio);
    
    // Buscar el primer espacio disponible en RAM para ocupar (usar criterio first fit)
    int ini = first_fit(segmento->tamanio);
    if(ini < 0){
        log_error(logger,"ERROR: compactar_segmento. Error de compactacion. Programame bien prro");
    }

    // Actualizo informacion segmento
    segmento->inicio = ini;

    // Reservar el espacio de memoria encontrado en el mapa 
    reservar_memoria_segmentacion(segmento->inicio, segmento->tamanio);

    // Guardar la informacion del buffer en la RAM
    memcpy(memoria_principal + segmento->inicio, buffer, segmento->tamanio);
    free(buffer);
}
    

t_list* lista_segmentos_en_memoria(){
    t_list* lista_segmentos_memoria = list_create();

    bool segmento_inicia_antes(void* arg1, void* arg2){
        segmento_t* segmento_menor = (segmento_t*) arg1;
        segmento_t* segmento_mayor = (segmento_t*) arg2;
        return segmento_menor->inicio < segmento_mayor->inicio;
    }

    // Agrega al segmento a la lista, ordenandolos por su direccion fisica inicial
    void agregar_segmento(void* args){
        segmento_t* segmento = (segmento_t*) args;
        list_add_sorted(lista_segmentos_memoria, segmento, segmento_inicia_antes);
    }

    // Agregamos cada segmento de la tabla a la lista de segmentos
    void agregar_segmentos_de_tabla(void* args){
        tabla_segmentos_t* tabla_patota = (tabla_segmentos_t*) args;
        list_iterate(tabla_patota->filas, agregar_segmento);
    }

    // Por cada tabla de patota, agregamos sus segmentos a la lista de segmentos
    list_iterate(tablas_de_patotas, agregar_segmentos_de_tabla);

    return lista_segmentos_memoria;
}