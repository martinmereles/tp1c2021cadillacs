#include "segmentacion.h"

// ALGORITMOS DE UBICACION

int inicializar_algoritmo_de_ubicacion(t_config* config){
    char* string_algoritmo_ubicacion = config_get_string_value(config, "CRITERIO_SELECCION");
    if(strcmp(string_algoritmo_ubicacion,"FF")==0){
        log_info(logger,"El algoritmo de ubicacion es: First Fit");
        algoritmo_de_ubicacion = &first_fit;
        return EXIT_SUCCESS;
    }
    if(strcmp(string_algoritmo_ubicacion,"BF")==0){
        log_info(logger,"El algoritmo de ubicacion es: Best Fit");
        algoritmo_de_ubicacion = &best_fit;
        return EXIT_SUCCESS;
    }
    log_error(logger,"%s: Algoritmo de ubicacion invalido",string_algoritmo_ubicacion);
    return EXIT_FAILURE;
}

segmento_t* first_fit(int memoria_pedida) {
    // Me fijo segmento por segmento de la memoria principal cual tiene suficiente espacio disponible
    t_list_iterator* iterador = list_iterator_create(lista_segmentos);    // Creamos el iterador
    segmento_t* segmento_first_fit = NULL;
    segmento_t* segmento;

    while(list_iterator_has_next(iterador)){
        segmento = list_iterator_next(iterador);
        if(segmento->estado == SEGMENTO_LIBRE && segmento->tamanio >= memoria_pedida){
            segmento_first_fit = segmento;
            break;
        }
    }

    list_iterator_destroy(iterador);    // Liberamos el iterador

    // Si no se encontro un segmento libre mayor o igual al tamanio pedido, retorna NULL
    return segmento_first_fit;
}

segmento_t* best_fit(int memoria_pedida) {
    // Me fijo segmento por segmento de la memoria principal cual tiene el mejor ajuste
    t_list_iterator* iterador = list_iterator_create(lista_segmentos);    // Creamos el iterador
    segmento_t* segmento_best_fit = NULL;
    segmento_t* segmento;
    log_info(logger,"ARRANCA EL BEST FIT");

    while(list_iterator_has_next(iterador)){
        segmento = list_iterator_next(iterador);
        // Si el segmento esta libre y tiene espacio suficiente
        if(segmento->estado == SEGMENTO_LIBRE && segmento->tamanio >= memoria_pedida){
            
            // Comparo el segmento hallado con el actual segmento con mejor ajuste encontrado
            if( segmento_best_fit == NULL || (segmento_best_fit->tamanio > segmento->tamanio ) )
                segmento_best_fit = segmento;
        }
    }

    list_iterator_destroy(iterador);    // Liberamos el iterador

    // Si no se encontro un segmento libre mayor o igual al tamanio pedido, retorna NULL
    return segmento_best_fit;
}

// LECTURA/ESCRITURA EN MEMORIA PRINCIPAL

uint32_t direccion_logica_segmentacion(uint32_t inicio_logico, uint32_t desplazamiento_logico){
    return inicio_logico + desplazamiento_logico;
}

int escribir_memoria_principal_segmentacion(void* args, uint32_t inicio_logico, uint32_t desplazamiento_logico, void* dato, int tamanio){
    tabla_segmentos_t* tabla = (tabla_segmentos_t*) args;
    uint32_t direccion_logica = direccion_logica_segmentacion(inicio_logico, desplazamiento_logico);
    int numero_seg = numero_de_segmento(direccion_logica);
    segmento_t* segmento = obtener_fila(tabla, numero_seg);

    // Agregar semaforo para el segmento
    sem_wait(&(segmento->semaforo));

    int direccion_fisica_dato = calcular_direccion_fisica(tabla, direccion_logica);
    if(segmento->inicio + segmento->tamanio < direccion_fisica_dato + tamanio){
        log_error(logger,"ERROR. Segmentation fault. Direccionamiento invalido en escritura");
        // Liberar semaforo del segmento
        sem_post(&(segmento->semaforo));
        return EXIT_FAILURE;
    }
    memcpy(memoria_principal+direccion_fisica_dato, dato, tamanio);
    // Liberar semaforo del segmento
    sem_post(&(segmento->semaforo));
    
    return EXIT_SUCCESS;
}

int leer_memoria_principal_segmentacion(void* args, uint32_t inicio_logico, uint32_t desplazamiento_logico, void* dato, int tamanio){
    tabla_segmentos_t* tabla = (tabla_segmentos_t*) args;
    uint32_t direccion_logica = direccion_logica_segmentacion(inicio_logico, desplazamiento_logico);
    int numero_seg = numero_de_segmento(direccion_logica);
    segmento_t* segmento = obtener_fila(tabla,numero_seg);
    
    // Agregar semaforo para el segmento
    sem_wait(&(segmento->semaforo));

    int direccion_fisica_dato = calcular_direccion_fisica(tabla, direccion_logica);
    if(segmento->inicio + segmento->tamanio < direccion_fisica_dato + tamanio){
        log_error(logger,"ERROR. Segmentation fault. Direccionamiento invalido en lectura");
        // Liberar semaforo del segmento
        sem_post(&(segmento->semaforo));
        return EXIT_FAILURE;
    }
    memcpy(dato, memoria_principal+direccion_fisica_dato, tamanio);
    // Liberar semaforo del segmento
    sem_post(&(segmento->semaforo));
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
}

void destruir_tabla_segmentos(void* args){
    tabla_segmentos_t* tabla = (tabla_segmentos_t*) args;
    // Destruir todas las filas y liberar segmentos;
    list_destroy_and_destroy_elements(tabla->filas, liberar_segmento);
    free(tabla->semaforo);
    free(tabla);
    // ejecutar_rutina(dump_memoria);  // Para test
}

void quitar_y_liberar_segmento(tabla_segmentos_t* tabla, int numero_seg){
    bool tiene_numero_de_segmento(void* args){
        segmento_t* fila = (segmento_t*) args;
        return fila->numero_segmento == numero_seg; 
    }
    list_remove_and_destroy_by_condition(tabla->filas, tiene_numero_de_segmento, liberar_segmento);
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
    tabla_patota->semaforo = malloc(sizeof(sem_t));
    tabla_patota->PID = PID;
    sem_init(tabla_patota->semaforo, 0, 1);

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

void destruir_segmento(void* args){
    segmento_t* segmento = (segmento_t*) args;
    sem_destroy(&(segmento->semaforo));
    free(segmento);
}

void liberar_segmento(void* args){
    segmento_t* segmento = (segmento_t*) args;
    
    // Liberamos el segmento
    sem_wait(&reservar_liberar_memoria_mutex);
    liberar_memoria_segmentacion(segmento);
    sem_post(&reservar_liberar_memoria_mutex);
}

segmento_t* crear_fila(tabla_segmentos_t* tabla, int tamanio){

    sem_wait(&reservar_liberar_memoria_mutex);

    // Buscamos un espacio de memoria por algoritmo
    segmento_t* segmento = algoritmo_de_ubicacion(tamanio);

    // Si no se encontro espacio
    if(segmento == NULL){
        // Ejecutamos la compactacion
        sem_post(&reservar_liberar_memoria_mutex);
        compactacion();
        sem_wait(&reservar_liberar_memoria_mutex);

        // Probamos una segunda vez 
        segmento = algoritmo_de_ubicacion(tamanio);
        
        // Si tampoco se encontro espacio
        if(segmento == NULL){
            log_error(logger,"ERROR: crear_fila. No hay espacio en memoria disponible.");
            sem_post(&reservar_liberar_memoria_mutex);
            return NULL;
        }
    }

    // Si se encontro espacio
    // Reservamos el espacio en el mapa de memoria disponible
    reservar_memoria_segmentacion(segmento, tamanio);

    sem_post(&reservar_liberar_memoria_mutex);

    // Agregamos el segmento a la tabla
    list_add(tabla->filas,segmento);
    segmento->PID = tabla->PID;
    segmento->numero_segmento = generar_nuevo_numero_segmento(tabla);
    return segmento;
}

void reservar_memoria_segmentacion(segmento_t* segmento, uint32_t tamanio_pedido){
    // Calculamos el espacio que sobraria
    uint32_t espacio_sobrante = segmento->tamanio - tamanio_pedido;

    // Actualizamos el segmento:
    segmento->estado = SEGMENTO_OCUPADO;
    segmento->tamanio = tamanio_pedido;

    // Si sobro espacio, creamos a continuacion un nuevo segmento libre
    uint32_t inicio_nuevo_segmento = segmento->inicio + segmento->tamanio;
    crear_segmento_libre(inicio_nuevo_segmento, espacio_sobrante);
}

void crear_segmento_libre(uint32_t inicio, uint32_t tamanio){
    
    // Si el tamanio es mayor a 0, creamos a partir de inicio un nuevo segmento libre
    if(tamanio > 0){
        // Creamos el segmento
        segmento_t* nuevo_segmento = malloc(sizeof(segmento_t));
        nuevo_segmento->inicio = inicio;
        nuevo_segmento->tamanio = tamanio;
        nuevo_segmento->estado = SEGMENTO_LIBRE;
        sem_init(&(nuevo_segmento->semaforo), 0, 1);

        // Agregamos el segmento a la lista global de segmentos, 
        // ordenada por su direccion fisica inicial

        bool segmento_inicia_antes(void* arg1, void* arg2){
            segmento_t* segmento_menor = (segmento_t*) arg1;
            segmento_t* segmento_mayor = (segmento_t*) arg2;
            return segmento_menor->inicio < segmento_mayor->inicio;
        }

        list_add_sorted(lista_segmentos, nuevo_segmento, segmento_inicia_antes);
    }    
}

void liberar_memoria_segmentacion(segmento_t* segmento){
    segmento->estado = SEGMENTO_LIBRE;
}

int generar_nuevo_numero_segmento(tabla_segmentos_t* tabla){
    sem_wait(tabla->semaforo);
    int nuevo = tabla->proximo_numero_segmento;
    tabla->proximo_numero_segmento++;
    sem_post(tabla->semaforo);
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
    quitar_y_liberar_segmento(tabla, numero_de_segmento(direccion_logica_TCB));

    // Si no quedan tripulantes en la patota, destruimos su tabla y liberamos sus recursos
    if(cantidad_filas(tabla) <= 2){
        quitar_y_destruir_tabla(tabla);
    }
}

// DUMP MEMORIA

void dump_memoria_segmentacion(){
    crear_archivo_dump(lista_segmentos, dump_segmento);
    // Para pruebas
    /*
    log_info(logger,"Dump: %s",temporal_get_string_time("%d/%m/%y %H:%M:%S"));
    list_iterate(tablas_de_patotas, dump_patota_segmentacion_pruebas);
    */
}

void dump_segmento(void* args, FILE* archivo_dump){
    segmento_t* segmento = (segmento_t*) args;
    char* info_segmento;

    // Proceso: 1	Segmento: 1	Inicio: 0x0000	Tam: 20b
    if(segmento->estado == SEGMENTO_OCUPADO){
        info_segmento = string_from_format("\nEstado: OCUPADO Proceso: %3d Segmento: %3d Inicio: %8d Tam: %db", 
                        segmento->PID, segmento->numero_segmento, segmento->inicio, segmento->tamanio);
    }
    else{
        info_segmento = string_from_format("\nEstado: LIBRE   Proceso:  -  Segmento:  -  Inicio: %8d Tam: %db", 
                        segmento->inicio, segmento->tamanio);
    }
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

    sem_wait(&reservar_liberar_memoria_mutex);
    log_info(logger,"INICIANDO COMPACTACION");
    
    int espacio_disponible = espacio_disponible_segmentacion();

    // Elimino todos los segmentos libres de la lista global de segmentos:
    bool es_segmento_libre(void* args){
        segmento_t* segmento = (segmento_t*) args;
        return segmento->estado == SEGMENTO_LIBRE;
    }

    while( list_any_satisfy(lista_segmentos, es_segmento_libre) )
        list_remove_and_destroy_by_condition(lista_segmentos, es_segmento_libre, destruir_segmento);
        
    // Compacto todos los segmentos ocupados por orden

    // Por cada segmento, lo movemos al primer lugar de la memoria disponible
    t_list_iterator* iterador = list_iterator_create(lista_segmentos);    // Creamos el iterador
    uint32_t inicio_proximo_espacio_disponible = 0;
    segmento_t* segmento;

    while(list_iterator_has_next(iterador)){
        segmento = list_iterator_next(iterador);
        compactar_segmento(segmento, inicio_proximo_espacio_disponible);
        inicio_proximo_espacio_disponible += segmento->tamanio;
    }

    list_iterator_destroy(iterador);    // Liberamos el iterador

    // Si sobra espacio, creo un segmento libre al final de la memoria principal:
    crear_segmento_libre(tamanio_memoria - espacio_disponible, espacio_disponible);

    log_info(logger,"COMPACTACION COMPLETADA");
    sem_post(&reservar_liberar_memoria_mutex);
}

void compactar_segmento(segmento_t* segmento, uint32_t inicio){

    sem_wait(&(segmento->semaforo));

    // Copiar contenido de la RAM en un buffer
    void* buffer = malloc(segmento->tamanio);
    memcpy(buffer, memoria_principal + segmento->inicio, segmento->tamanio);

    // Actualizo informacion segmento
    segmento->inicio = inicio;

    // Guardar la informacion del buffer en la RAM
    memcpy(memoria_principal + segmento->inicio, buffer, segmento->tamanio);
    free(buffer);

    sem_post(&(segmento->semaforo));
}

uint32_t espacio_disponible_segmentacion(){

    uint32_t espacio_disponible = 0;

    // Por cada segmento, nos fijamos si esta libre o no
    t_list_iterator* iterador = list_iterator_create(lista_segmentos);    // Creamos el iterador
    segmento_t* segmento;

    while(list_iterator_has_next(iterador)){
        segmento = list_iterator_next(iterador);
        if(segmento->estado == SEGMENTO_LIBRE)
            espacio_disponible += segmento->tamanio;
    }

    list_iterator_destroy(iterador);    // Liberamos el iterador
    return espacio_disponible;
}