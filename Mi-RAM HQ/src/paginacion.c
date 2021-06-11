#include "paginacion.h"

// PATOTAS Y TRIPULANTES
 
int crear_patota_paginacion(uint32_t PID, uint32_t longitud_tareas, char* tareas){

    // Creamos la tabla de paginas de la patota
    tabla_paginas_t* tabla_patota = malloc(sizeof(tabla_paginas_t));
    tabla_patota->paginas = list_create();
    tabla_patota->PID = PID;
    tabla_patota->proximo_numero_pagina = 0;
    tabla_patota->fragmentacion_interna = 0;
    tabla_patota->cantidad_tripulantes = 0;

    // Buscamos un espacio en memoria para el PCB y lo reservamos
    uint32_t* direccion_logica_PCB;
    if(reservar_memoria(tabla_patota, TAMANIO_PCB, direccion_logica_PCB) == EXIT_FAILURE){
        log_error(logger, "ERROR. No hay espacio para guardar el PCB de la patota. %d",TAMANIO_PCB);
        destruir_tabla_paginas(tabla_patota);
        return EXIT_FAILURE;
    }

    //log_info(logger,"La cantidad de paginas es: %d",cantidad_paginas(tabla_patota));
    //log_info(logger,"El nro de segmento es: %d",fila_PCB->numero_segmento);

    // Buscamos un espacio en memoria para las tareas y lo reservamos
    uint32_t* direccion_logica_tareas;
    if(reservar_memoria(tabla_patota, longitud_tareas, direccion_logica_tareas) == EXIT_FAILURE){
        log_error(logger, "ERROR. No hay espacio para guardar las tareas de la patota.");
        destruir_tabla_paginas(tabla_patota);
        return EXIT_FAILURE;
    }

    //log_info(logger,"La cantidad de paginas es: %d",cantidad_paginas(tabla_patota));
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

int crear_tripulante_paginacion(void** tabla, uint32_t* dir_log_tcb, 
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

    *tabla->cantidad_tripulantes++;

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

// Reservar/Leer/Escribir memoria

int reservar_memoria(tabla_paginas_t* tabla_patota, int tamanio, uint32_t* direccion_logica){   
    // Armamos la direccion logica que apunta al inicio de la memoria a reservar
    *direccion_logica = ultima_pagina(tabla_patota)->numero_pagina << 16;     
    *direccion_logica += tamanio_pagina - tabla_patota->fragmentacion_interna;

    // Descontamos del tamanio la fragmentacion interna actual
    tamanio -= tabla_patota->fragmentacion_interna;

    // Si necesitamos mas memoria, reservamos paginas hasta que podamos cubrir el tamanio pedido
    while(tamanio > 0){
        ultima_pagina(tabla_patota)->fragmentacion_interna = 0;     // La ultima pagina no tiene fragm. pues se va a llenar
        if(crear_pagina(tabla_patota) == EXIT_FAILURE)
            return EXIT_FAILURE;
        tamanio -= tamanio_pagina;
    }
    // Contamos la fragmentacion interna (espacio que sobro)
    tabla_patota->fragmentacion_interna = -tamanio;         
    ultima_pagina(tabla_patota)->fragmentacion_interna = -tamanio;
    return EXIT_SUCCESS;
}

int escribir_memoria_principal_paginacion(void* args, uint32_t inicio_logico, uint32_t desplazamiento_logico, void* dato, int tamanio_total){
    log_info(logger,"Escribiendo en memoria principal");
    tabla_paginas_t* tabla = (tabla_paginas_t*) args;
    uint32_t direccion_logica = direccion_logica_paginacion(inicio_logico, desplazamiento_logico);

    int direccion_fisica_dato; 
    if(direccion_fisica_paginacion(tabla, direccion_logica, &direccion_fisica_dato) == EXIT_FAILURE){
        log_error(logger,"ERROR: escribir_memoria_principal. Direccionamiento invalido.");
        return EXIT_FAILURE;
    }
    int desplazamiento = desplazamiento_paginacion(direccion_logica);

    /* Voy a empezar a escribir el dato en la pagina actual:
        1) Hasta que recorra el dato completo
        2) Hasta que llegue al final de la pagina: en ese caso, 
            debo continuar escribriendo en la pagina siguiente
    */

    // Calculo el tamanio de la escritura en la pagina actual
    int tamanio_escritura_pagina_actual = tamanio_pagina - desplazamiento;
    if(tamanio_escritura_pagina_actual > tamanio_total)
        tamanio_escritura_pagina_actual = tamanio_total;

    memcpy(memoria_principal + direccion_fisica_dato, dato, tamanio_escritura_pagina_actual);

    // Calculo el tamanio de lo que me falto escribir
    tamanio_total -= tamanio_escritura_pagina_actual;
    
    // Si no termine de escribir, continuo al principio de la siguiente pagina
    if(tamanio_total != 0){
        int numero_proxima_pagina = numero_pagina(direccion_logica) + 1;
        int inicio_logico_proxima_pagina = (numero_proxima_pagina << 16) & 0xFFFF0000;
        void* dato_proxima_pagina = dato + tamanio_escritura_pagina_actual;
        escribir_memoria_principal_paginacion(args, inicio_logico_proxima_pagina, 0, dato_proxima_pagina, tamanio_total);
    }

    return EXIT_SUCCESS;
}

int leer_memoria_principal_paginacion(void* args, uint32_t inicio_logico, uint32_t desplazamiento_logico, void* dato, int tamanio){
    
    log_info(logger,"Leyendo en memoria principal");
    tabla_paginas_t* tabla = (tabla_paginas_t*) args;
    uint32_t direccion_logica = direccion_logica_paginacion(inicio_logico, desplazamiento_logico);

    int direccion_fisica_dato; 
    if(direccion_fisica_paginacion(tabla, direccion_logica, &direccion_fisica_dato) == EXIT_FAILURE){
        log_error(logger,"ERROR: leer_memoria_principal. Direccionamiento invalido.");
        return EXIT_FAILURE;
    }
    int desplazamiento = desplazamiento_paginacion(direccion_logica);

    /* Voy a empezar a leer el dato en la pagina actual:
        1) Hasta que recorra el dato completo
        2) Hasta que llegue al final de la pagina: en ese caso, 
            debo continuar leyendo en la pagina siguiente
    */

    // Calculo el tamanio de la lectura en la pagina actual
    int tamanio_lectura_pagina_actual = tamanio_pagina - desplazamiento;
    if(tamanio_lectura_pagina_actual > tamanio_total)
        tamanio_lectura_pagina_actual = tamanio_total;

    memcpy(dato, memoria_principal + direccion_fisica_dato, tamanio_lectura_pagina_actual);

    // Calculo el tamanio de lo que me falto leer
    tamanio_total -= tamanio_lectura_pagina_actual;
    
    // Si no termine de leer, continuo al principio de la siguiente pagina
    if(tamanio_total != 0){
        int numero_proxima_pagina = numero_pagina(direccion_logica) + 1;
        int inicio_logico_proxima_pagina = (numero_proxima_pagina << 16) & 0xFFFF0000;
        void* dato_proxima_pagina = dato + tamanio_lectura_pagina_actual;
        escribir_memoria_principal_paginacion(args, inicio_logico_proxima_pagina, 0, dato_proxima_pagina, tamanio_total);
    }

    return EXIT_SUCCESS;
}

// TABLAS DE PAGINACION

tabla_paginas_t* obtener_tabla_patota(int PID_buscado){
    bool tienePID(void* args){
        tabla_paginas_t* tabla = (tabla_paginas_t*) args;
        int PID_tabla;
        leer_memoria_principal(tabla, DIR_LOG_PCB, DESPL_PID, &PID_tabla, sizeof(uint32_t));
        return PID_tabla == PID_buscado;
    }
    return list_find(tablas_de_patotas, tienePID);
}

marco_t* ultima_pagina(tabla_paginas_t* tabla_patota){
    int cant_paginas = list_size(tabla_patota->paginas);
    return list_get(tabla_patota->paginas, cant_paginas - 1);
}

marco_t* buscar_marco_libre(){
    bool es_marco_libre(void* args){
        marco_t* marco = (marco_t*) args;
        return marco->estado == LIBRE;
    }
    return  list_find(lista_de_marcos, es_marco_libre);
}

int crear_pagina(tabla_paginas_t* tabla_patota){

    // 1er Paso: Reservamos en el mapa de memoria disponbile un marco
    int n_marco = 0;
    
    // Buscamos un marco que este libre
    marco_t* pagina = buscar_marco_libre()
    if(pagina == NULL){
        log_error(logger, "ERROR: crear_pagina. No hay marcos lbres.");
        return EXIT_FAILURE;
    }

    // 2do paso: Si hay un marco disponible, lo reservamos y lo agregamos a la tabla de la patota como pagina
    
    pagina->PID = tabla_patota->PID;
    pagina->estado = OCUPADO;
    pagina->bit_presencia = true;
    pagina->bit_modificado = false;
    pagina->numero_pagina = tabla_patota->proximo_numero_pagina;

    tabla_patota->proximo_numero_pagina++;
    list_add(tabla_patota->paginas,pagina);

    return EXIT_SUCCESS;
}

fila_tabla_segmentos_t* get_pagina(tabla_paginas_t* tabla_patota, int numero_pagina_buscada){
    bool tiene_numero_de_pagina(void* args){
        marco_t* pagina = (marco_t*) args;
        return pagina->numero_pagina == numero_pagina_buscada; 
    }
    return list_find(tabla_patota->paginas, tiene_numero_de_pagina);
}

int cantidad_paginas(tabla_paginas_t* tabla_patota){
    return list_size(tabla->paginas);
}

// DIRECCIONAMIENTO

uint32_t direccion_logica_paginacion(uint32_t inicio_logico, uint32_t desplazamiento_logico){
    uint32_t direccion_logica;
    int numero_pagina = numero_pagina(inicio_logico);               // Obtenemos el numero pagina inicial     
    int desplazamiento_inicial = desplazamiento_paginacion(inicio_logico);     // Obtenemos el desplazamiento inicial
    int desplazamiento_total = desplazamiento_inicial + desplazamiento_logico;  // Obtenemos el desplazamiento total
    // Si el desplazamiento es mas grande que el tamanio de pagina, pasamos a la siguiente
    while(desplazamiento_total > tamanio_pagina){
        numero_pagina++;                            // Pasamos a la siguiente pagina
        desplazamiento_total -= tamanio_pagina;     // Descontamos el tamanio de pagina al desplazamiento
    }
    // Armamos la direccion logica final
    direccion_logica = numero_pagina << 16;
    direccion_logica += desplazamiento_total;
    return direccion_logica;
}

int numero_pagina(uint32_t direccion_logica){
    return (direccion_logica >> 16) & 0x0000FFFF;
}

int desplazamiento_paginacion(uint32_t direccion_logica){
    return direccion_logica & 0x0000FFFF;
}

int direccion_fisica_paginacion(tabla_paginas_t* tabla_patota, uint32_t direccion_logica, int* direccion_fisica){
    fila_tabla_segmentos_t* pagina = get_pagina(tabla_patota, numero_pagina(direccion_logica));
    if(pagina == NULL)
        return EXIT_FAILURE;
    *direccion_fisica = (pagina->numero_marco) << 16;
    *direccion_fisica += desplazamiento_paginacion(direccion_logica);
    return EXIT_SUCCESS;
}

// DUMP MEMORIA

// ES MUY PARECIDO A LA VERSION DE SEGMENTACION
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

    // Obtenemos la fecha
    fecha = temporal_get_string_time("%d/%m/%y %H:%M:%S");

    // Escribimos info general del dump
    char* info_dump = string_from_format("Dump: %s", fecha);
    fwrite(info_dump, sizeof(char), strlen(info_dump), archivo_dump);
    free(info_dump);
    free(fecha);    // Liberamos el string fecha

    // Por cada marco, escribimos la informacion correspondiente
    t_list_iterator* iterador = list_iterator_create(lista_de_marcos);    // Creamos el iterador
    marco_t* marco;

    log_info(logger,"LA CANTIDAD DE MARCOS ES: %d", list_size(lista_de_marcos));

    while(list_iterator_has_next(iterador)){
        marco = list_iterator_next(iterador);
        dump_marco(marco, archivo_dump);
    }

    list_iterator_destroy(iterador);    // Liberamos el iterador

    // Cerramos el archivo
    fclose(archivo_dump);
}

void dump_marco(marco_t* marco, FILE* archivo_dump){
    // Marco:0	Estado:Ocupado	Proceso: 1	Pagina: 2
    char* info_marco = string_new();                                                         
    string_append(&info_marco, "\n");                                                        
    string_append(&info_marco, string_from_format("Proceso: %3d ", marco->numero_marco));    
    if(marco->estado == OCUPADO){
        string_append(&info_marco, "Estado: Ocupado ");
        string_append(&info_marco, string_from_format("Proceso: %3d ", marco->PID));         
        string_append(&info_marco, string_from_format("Pagina: %3d", marco->numero_pagina)); 
    }
    else{
        string_append(&info_marco, "Estado: Libre   ");
        string_append(&iinfo_marco, "Proceso:   - ");         
        string_append(&info_marco, "Pagina:   -"); 
    }         
    
    fwrite(info_marco, sizeof(char), strlen(info_marco), archivo_dump);
    
    free(info_marco);
}