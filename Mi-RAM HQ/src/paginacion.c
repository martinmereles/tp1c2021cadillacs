#include "paginacion.h"

// PATOTAS Y TRIPULANTES
 
void eliminar_patota(tabla_paginas_t* tabla_a_eliminar){
    
    bool tiene_PID(void* args){
        tabla_paginas_t* tabla = (tabla_paginas_t*) args;
        uint32_t PID_tabla;
        leer_memoria_principal(tabla, DIR_LOG_PCB, DESPL_PID, &PID_tabla, sizeof(uint32_t));
        return PID_tabla == tabla_a_eliminar->PID;
    }

    // DESTRUIMOS LA TABLA DE PAGINAS Y LIBERAMOS LOS MARCOS (Los marcos no se destruyen)
    void destruir_tabla_patota(void* args){
        tabla_paginas_t* tabla = (tabla_paginas_t*) args;
        list_destroy_and_destroy_elements(tabla->paginas, liberar_marco);
        free(tabla);
    }

    // QUITAMOS LA PATOTA DE LA LISTA DE PATOTAS Y LA DESTRUIMOS
    list_remove_and_destroy_by_condition(tablas_de_patotas, tiene_PID, destruir_tabla_patota);
}



void liberar_marco(void* args){
    marco_t* marco = (marco_t*) args;
    marco->estado = MARCO_LIBRE;
}

int crear_patota_paginacion(uint32_t PID, uint32_t longitud_tareas, char* tareas){

    // Creamos la tabla de paginas de la patota
    tabla_paginas_t* tabla_patota = malloc(sizeof(tabla_paginas_t));
    tabla_patota->paginas = list_create();
    tabla_patota->PID = PID;
    tabla_patota->proximo_numero_pagina = 0;
    tabla_patota->fragmentacion_interna = 0;
    tabla_patota->cantidad_tripulantes = 0;

    // Buscamos un espacio en memoria para el PCB y lo reservamos
    uint32_t direccion_logica_PCB;
    if(reservar_memoria(tabla_patota, TAMANIO_PCB, &direccion_logica_PCB) == EXIT_FAILURE){
        log_error(logger, "ERROR. No hay espacio para guardar el PCB de la patota. %d",TAMANIO_PCB);
        eliminar_patota(tabla_patota);
        return EXIT_FAILURE;
    }

    //log_info(logger,"La direccion logica del PCB es: %x",direccion_logica_PCB);

    // Buscamos un espacio en memoria para las tareas y lo reservamos
    uint32_t direccion_logica_tareas;
    if(reservar_memoria(tabla_patota, longitud_tareas, &direccion_logica_tareas) == EXIT_FAILURE){
        log_error(logger, "ERROR. No hay espacio para guardar las tareas de la patota.");
        eliminar_patota(tabla_patota);
        return EXIT_FAILURE;
    }

    //log_info(logger,"La direccion logica de las tareas es: %x",direccion_logica_tareas);

    // Guardo en la tabla el tamanio de la lista de tareas
    tabla_patota->tamanio_tareas = longitud_tareas;

    // Agregar la tabla a la lista de tablas
    list_add(tablas_de_patotas, tabla_patota);

    // Guardo el PID y la direccion logica de las tareas en el PCB
    escribir_memoria_principal(tabla_patota, direccion_logica_PCB, DESPL_PID, &PID, sizeof(uint32_t));
    escribir_memoria_principal(tabla_patota, direccion_logica_PCB, DESPL_TAREAS, &direccion_logica_tareas, sizeof(uint32_t));  

    // Guardo las tareas
    escribir_memoria_principal(tabla_patota, direccion_logica_tareas, 0, tareas, longitud_tareas);

    log_info(logger, "Estructuras de la patota inicializadas exitosamente");

    dump_memoria();

    return EXIT_SUCCESS;
}

int crear_tripulante_paginacion(void** args, uint32_t* direccion_logica_TCB, 
    uint32_t PID, uint32_t TID, uint32_t posicion_X, uint32_t posicion_Y){

    tabla_paginas_t** tabla_patota = (tabla_paginas_t**) args;

    log_info(logger, "CREANDO TRIPULANTE");

    // Buscar la tabla que coincide con el PID
    *tabla_patota = obtener_tabla_patota(PID);

    if(*tabla_patota == NULL){
        log_error(logger,"ERROR. No se encontro la tabla de paginas de la patota");
        return EXIT_FAILURE;
    }

    // Buscamos un espacio en memoria para el TCB y lo reservamos
    if(reservar_memoria(*tabla_patota, TAMANIO_TCB, direccion_logica_TCB) == EXIT_FAILURE){
        log_error(logger, "ERROR. No hay espacio para guardar el TCB del tripulante. %d",TAMANIO_TCB);
        eliminar_patota(*tabla_patota);
        return EXIT_FAILURE;
    }

    (*tabla_patota)->cantidad_tripulantes++;

    // Guardo el TID, el estado, la posicion, el identificador de la proxima instruccion y la direccion logica del PCB
    char estado = 'N';
    uint32_t id_proxima_instruccion = 0;
    uint32_t dir_log_pcb = DIR_LOG_PCB;
    escribir_memoria_principal(*tabla_patota, *direccion_logica_TCB, DESPL_TID, &TID, sizeof(uint32_t));
    escribir_memoria_principal(*tabla_patota, *direccion_logica_TCB, DESPL_ESTADO, &estado, sizeof(char));
    escribir_memoria_principal(*tabla_patota, *direccion_logica_TCB, DESPL_POS_X, &posicion_X, sizeof(uint32_t));
    escribir_memoria_principal(*tabla_patota, *direccion_logica_TCB, DESPL_POS_Y, &posicion_Y, sizeof(uint32_t));
    escribir_memoria_principal(*tabla_patota, *direccion_logica_TCB, DESPL_PROX_INSTR, &id_proxima_instruccion, sizeof(uint32_t));
    escribir_memoria_principal(*tabla_patota, *direccion_logica_TCB, DESPL_DIR_PCB, &dir_log_pcb, sizeof(uint32_t));

    dump_memoria();

    return EXIT_SUCCESS;
}

void eliminar_tripulante_paginacion(void* args, uint32_t direccion_logica_TCB){
    tabla_paginas_t* tabla_patota = (tabla_paginas_t*) args; 

    // Liberamos memoria a partir de la direccion logica del TCB
    if(liberar_memoria(tabla_patota, TAMANIO_TCB, direccion_logica_TCB) == EXIT_FAILURE)
        log_error(logger, "ERROR: eliminar_tripulante_paginacion. Direccionamiento invalido");

    // Descontamos el tripulante expulsado
    tabla_patota->cantidad_tripulantes--;

    // Si no quedan tripulantes en la patota, la destruimos y liberamos sus recursos
    if(tabla_patota->cantidad_tripulantes <= 0)
        eliminar_patota(tabla_patota);
}

// Liberar/Reservar/Leer/Escribir memoria

int liberar_memoria(tabla_paginas_t* tabla_patota, int tamanio_total, uint32_t direccion_logica){

    int desplazamiento = get_desplazamiento(direccion_logica);

    /* Voy a empezar a liberar memoria en la pagina actual:
        1) Hasta que libere la cantidad de memoria pedida
        2) Hasta que llegue al final de la pagina: en ese caso, 
            debo continuar en la pagina siguiente
    */

    // Calculo el tamanio de memoria a liberar en la pagina actual
    int tamanio_memoria_pagina_actual = tamanio_pagina - desplazamiento;
    if(tamanio_memoria_pagina_actual > tamanio_total)
        tamanio_memoria_pagina_actual = tamanio_total;

    // Libero memoria
    marco_t* pagina_actual = get_pagina(tabla_patota, numero_pagina(direccion_logica));
    if(pagina_actual == NULL)
        return EXIT_FAILURE;
    pagina_actual->fragmentacion_interna += tamanio_memoria_pagina_actual;

    // Si el marco ya no guarda nada, lo liberamos para que lo pueda usar otra patota
    if(pagina_actual->fragmentacion_interna == tamanio_pagina){

        // Quitamos el marco de la lista de paginas de la tabla de la patota y lo liberamos
        bool tiene_numero_de_pagina(void* args){
            marco_t* pagina_encontrada = (marco_t*) args;
            return pagina_encontrada->numero_pagina == pagina_actual->numero_pagina; 
        }
        list_remove_and_destroy_by_condition(tabla_patota->paginas, tiene_numero_de_pagina, liberar_marco);
    }

    // Calculo el tamanio de lo que me falto liberar
    tamanio_total -= tamanio_memoria_pagina_actual;
    
    // Si no termine de liberar memoria, continuo al principio de la siguiente pagina
    if(tamanio_total != 0){
        int numero_proxima_pagina = numero_pagina(direccion_logica) + 1;
        int direccion_logica_proxima_pagina = (numero_proxima_pagina << 16) & 0xFFFF0000;
        liberar_memoria(tabla_patota, tamanio_total, direccion_logica_proxima_pagina);
    }

    return EXIT_SUCCESS;
}

int reservar_memoria(tabla_paginas_t* tabla_patota, int tamanio, uint32_t* direccion_logica){   

    // Revisamos si no tiene fragmentacion interna
    if(tabla_patota->fragmentacion_interna == 0){
        if(crear_pagina(tabla_patota) == EXIT_FAILURE)
            return EXIT_FAILURE;
        tabla_patota->fragmentacion_interna += tamanio_pagina;
    }

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

    // PARA TESTEAR
    dump_memoria();

    return EXIT_SUCCESS;
}

int escribir_memoria_principal_paginacion(void* args, uint32_t inicio_logico, uint32_t desplazamiento_logico, void* dato, int tamanio_total){
    
    tabla_paginas_t* tabla = (tabla_paginas_t*) args;
    uint32_t direccion_logica = direccion_logica_paginacion(inicio_logico, desplazamiento_logico);

    uint32_t direccion_fisica_dato; 
    if(direccion_fisica_paginacion(tabla, direccion_logica, &direccion_fisica_dato) == EXIT_FAILURE){
        log_error(logger,"ERROR: escribir_memoria_principal. Direccionamiento invalido.");
        return EXIT_FAILURE;
    }

    log_info(logger,"DIRECCION FISICA A ESCRIBIR: %x",direccion_fisica_dato);

    int desplazamiento = get_desplazamiento(direccion_logica);

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

int leer_memoria_principal_paginacion(void* args, uint32_t inicio_logico, uint32_t desplazamiento_logico, void* dato, int tamanio_total){

    tabla_paginas_t* tabla = (tabla_paginas_t*) args;
    uint32_t direccion_logica = direccion_logica_paginacion(inicio_logico, desplazamiento_logico);

    uint32_t direccion_fisica_dato; 
    if(direccion_fisica_paginacion(tabla, direccion_logica, &direccion_fisica_dato) == EXIT_FAILURE){
        log_error(logger,"ERROR: leer_memoria_principal. Direccionamiento invalido.");
        return EXIT_FAILURE;
    }
    int desplazamiento = get_desplazamiento(direccion_logica);

    log_info(logger,"DIRECCION FISICA A LEER: %x",direccion_fisica_dato);
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
        leer_memoria_principal_paginacion(args, inicio_logico_proxima_pagina, 0, dato_proxima_pagina, tamanio_total);
    }

    return EXIT_SUCCESS;
}

// TABLAS DE PAGINACION

tabla_paginas_t* obtener_tabla_patota_paginacion(int PID_buscado){
    bool tiene_PID(void* args){
        tabla_paginas_t* tabla = (tabla_paginas_t*) args;
        uint32_t PID_tabla;
        leer_memoria_principal(tabla, DIR_LOG_PCB, DESPL_PID, &PID_tabla, sizeof(uint32_t));
        log_info(logger,"EL PID ENCONTRADO ES: %d",PID_tabla);
        return PID_tabla == PID_buscado;
    }
    return list_find(tablas_de_patotas, tiene_PID);
}

marco_t* ultima_pagina(tabla_paginas_t* tabla_patota){
    int cant_paginas = list_size(tabla_patota->paginas);
    return list_get(tabla_patota->paginas, cant_paginas - 1);
}

marco_t* buscar_marco_libre(){
    bool es_marco_libre(void* args){
        marco_t* marco = (marco_t*) args;
        return marco->estado == MARCO_LIBRE;
    }
    return  list_find(lista_de_marcos, es_marco_libre);
}

int crear_pagina(tabla_paginas_t* tabla_patota){

    // 1er Paso: Buscamos un marco que este libre
    marco_t* pagina = buscar_marco_libre();
    if(pagina == NULL){
        log_error(logger, "ERROR: crear_pagina. No hay marcos libres.");
        return EXIT_FAILURE;
    }

    // 2do paso: Si hay un marco disponible, lo reservamos y lo agregamos a la tabla de la patota como pagina
    
    pagina->PID = tabla_patota->PID;
    pagina->estado = MARCO_OCUPADO;
    pagina->bit_presencia = true;
    pagina->bit_uso = true;
    pagina->numero_pagina = tabla_patota->proximo_numero_pagina;
    pagina->timestamp = temporal_get_string_time("%y_%m_%d_%H_%M_%S");

    tabla_patota->proximo_numero_pagina++;
    list_add(tabla_patota->paginas,pagina);

    return EXIT_SUCCESS;
}

marco_t* get_pagina(tabla_paginas_t* tabla_patota, int numero_pagina_buscada){
    bool tiene_numero_de_pagina(void* args){
        marco_t* pagina = (marco_t*) args;
        return pagina->numero_pagina == numero_pagina_buscada; 
    }
    return list_find(tabla_patota->paginas, tiene_numero_de_pagina);
}

int cantidad_paginas(tabla_paginas_t* tabla_patota){
    return list_size(tabla_patota->paginas);
}

int tamanio_tareas_paginacion(void* args){
    tabla_paginas_t* tabla = (tabla_paginas_t*) args;
    return tabla->tamanio_tareas;
}

// DIRECCIONAMIENTO

uint32_t direccion_logica_paginacion(uint32_t inicio_logico, uint32_t desplazamiento_logico){
    uint32_t direccion_logica;
    int nro_pagina = numero_pagina(inicio_logico);               // Obtenemos el numero pagina inicial     
    int desplazamiento_inicial = get_desplazamiento(inicio_logico);     // Obtenemos el desplazamiento inicial
    int desplazamiento_total = desplazamiento_inicial + desplazamiento_logico;  // Obtenemos el desplazamiento total
    // Si el desplazamiento es mas grande que el tamanio de pagina, pasamos a la siguiente
    while(desplazamiento_total >= tamanio_pagina){
        nro_pagina++;                            // Pasamos a la siguiente pagina
        desplazamiento_total -= tamanio_pagina;     // Descontamos el tamanio de pagina al desplazamiento
    }
    // Armamos la direccion logica final
    direccion_logica = (nro_pagina << 16) & 0xFFFF0000;
    direccion_logica += desplazamiento_total;
    return direccion_logica;
}

int numero_pagina(uint32_t direccion_logica){
    return (direccion_logica >> 16) & 0x0000FFFF;
}

int direccion_fisica_paginacion(tabla_paginas_t* tabla_patota, uint32_t direccion_logica, uint32_t* direccion_fisica){
    marco_t* pagina = get_pagina(tabla_patota, numero_pagina(direccion_logica));
    if(pagina == NULL)
        return EXIT_FAILURE;
    *direccion_fisica = (pagina->numero_marco) * tamanio_pagina;
    *direccion_fisica += get_desplazamiento(direccion_logica);
    return EXIT_SUCCESS;
}

// DUMP MEMORIA
void dump_memoria_paginacion(){
    crear_archivo_dump(lista_de_marcos, dump_marco);
}

void dump_marco(void* args, FILE* archivo_dump){
    marco_t* marco = (marco_t*) args;

    // Marco:0	Estado:Ocupado	Proceso: 1	Pagina: 2
    char* info_marco;                                    
    if(marco->estado == MARCO_OCUPADO)
        info_marco = string_from_format("\nMarco: %3d Estado: Ocupado Proceso: %3d Pagina: %3d",
                                         marco->numero_marco,marco->PID,marco->numero_pagina);
    else
        info_marco = string_from_format("\nMarco: %3d Estado: Libre   Proceso:  -  Pagina:  - ",
                                         marco->numero_marco);       
    
    fwrite(info_marco, sizeof(char), strlen(info_marco), archivo_dump);

    free(info_marco);
}

// LRU
void actualizar_timestamp(marco_t* pagina){
    free(pagina->timestamp);
    pagina->timestamp = temporal_get_string_time("%y_%m_%d_%H_%M_%S");
}