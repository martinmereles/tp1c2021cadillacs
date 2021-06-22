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
    tabla_patota->semaforo = malloc(sizeof(sem_t));
    sem_init(tabla_patota->semaforo, 0, 1);

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

    //ejecutar_rutina(dump_memoria);

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

    //ejecutar_rutina(dump_memoria);

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

    //ejecutar_rutina(dump_memoria);

    return EXIT_SUCCESS;
}

int reservar_memoria(tabla_paginas_t* tabla_patota, int tamanio, uint32_t* direccion_logica){   
    int memoria_reservada = tamanio;

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
        if(crear_pagina(tabla_patota) == EXIT_FAILURE){
            // Si no alcanzo la memoria, liberamos lo que se reservo
            memoria_reservada -= tamanio;
            liberar_memoria(tabla_patota, memoria_reservada,*direccion_logica);
            return EXIT_FAILURE;
        }
        tamanio -= tamanio_pagina;
    }
    
    // Contamos la fragmentacion interna (espacio que sobro)
    tabla_patota->fragmentacion_interna = -tamanio;         
    ultima_pagina(tabla_patota)->fragmentacion_interna = -tamanio;

    // PARA TESTEAR
    //ejecutar_rutina(dump_memoria);

    return EXIT_SUCCESS;
}

int escribir_memoria_principal_paginacion(void* args, uint32_t inicio_logico, uint32_t desplazamiento_logico, void* dato, int tamanio_total){

    tabla_paginas_t* tabla = (tabla_paginas_t*) args;
    uint32_t direccion_logica = direccion_logica_paginacion(inicio_logico, desplazamiento_logico);
  
    // Si la pagina esta en memoria virtual, debo traerla
    marco_t* pagina = get_pagina(tabla, numero_pagina(direccion_logica)); 

    if(!pagina->bit_presencia)
        proceso_swap(pagina);
   
    sem_wait(pagina->semaforo);

    // Calculo la direccion fisica
    uint32_t direccion_fisica_dato; 
    if(direccion_fisica_paginacion(tabla, direccion_logica, &direccion_fisica_dato) == EXIT_FAILURE){
        log_error(logger,"ERROR: escribir_memoria_principal. Direccionamiento invalido.");
        sem_post(pagina->semaforo);
        return EXIT_FAILURE;
    }   
    
    // log_info(logger,"DIRECCION FISICA A ESCRIBIR: %x",direccion_fisica_dato);

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

    // Escribo el dato en memoria principal
    memcpy(memoria_principal + direccion_fisica_dato, dato, tamanio_escritura_pagina_actual);
   
    // Actualizo informacion de la pagina
    actualizar_timestamp(pagina);   // LRU
    pagina->bit_uso = true;         // Clock
  
    // Calculo el tamanio de lo que me falto escribir
    tamanio_total -= tamanio_escritura_pagina_actual;
    
    // Si no termine de escribir, continuo al principio de la siguiente pagina
    if(tamanio_total != 0){
        int numero_proxima_pagina = numero_pagina(direccion_logica) + 1;
        sem_post(pagina->semaforo);
        int inicio_logico_proxima_pagina = (numero_proxima_pagina << 16) & 0xFFFF0000;
        void* dato_proxima_pagina = dato + tamanio_escritura_pagina_actual;
        escribir_memoria_principal_paginacion(args, inicio_logico_proxima_pagina, 0, dato_proxima_pagina, tamanio_total);
    }

    sem_post(pagina->semaforo);

    return EXIT_SUCCESS;
}

int leer_memoria_principal_paginacion(void* args, uint32_t inicio_logico, uint32_t desplazamiento_logico, void* dato, int tamanio_total){

    tabla_paginas_t* tabla = (tabla_paginas_t*) args;
    uint32_t direccion_logica = direccion_logica_paginacion(inicio_logico, desplazamiento_logico);

    // Si la pagina esta en memoria virtual, debo traerla
    marco_t* pagina = get_pagina(tabla, numero_pagina(direccion_logica)); 

    if(!pagina->bit_presencia)
        proceso_swap(pagina);

    sem_wait(pagina->semaforo);

    // Calculo la direccion fisica
    uint32_t direccion_fisica_dato; 
    if(direccion_fisica_paginacion(tabla, direccion_logica, &direccion_fisica_dato) == EXIT_FAILURE){
        log_error(logger,"ERROR: leer_memoria_principal. Direccionamiento invalido.");
        sem_post(pagina->semaforo);
        return EXIT_FAILURE;
    }

    //log_info(logger,"DIRECCION FISICA A LEER: %x",direccion_fisica_dato);

    int desplazamiento = get_desplazamiento(direccion_logica);
    
    /* Voy a empezar a leer el dato en la pagina actual:
        1) Hasta que recorra el dato completo
        2) Hasta que llegue al final de la pagina: en ese caso, 
            debo continuar leyendo en la pagina siguiente
    */

    // Calculo el tamanio de la lectura en la pagina actual
    int tamanio_lectura_pagina_actual = tamanio_pagina - desplazamiento;
    if(tamanio_lectura_pagina_actual > tamanio_total)
        tamanio_lectura_pagina_actual = tamanio_total;

    // Leo el dato de memoria principal
    memcpy(dato, memoria_principal + direccion_fisica_dato, tamanio_lectura_pagina_actual);

    // Actualizo informacion de la pagina
    actualizar_timestamp(pagina);   // LRU
    pagina->bit_uso = true;         // Clock

    // Calculo el tamanio de lo que me falto leer
    tamanio_total -= tamanio_lectura_pagina_actual;
    
    // Si no termine de leer, continuo al principio de la siguiente pagina
    if(tamanio_total != 0){
        int numero_proxima_pagina = numero_pagina(direccion_logica) + 1;
        sem_post(pagina->semaforo);
        int inicio_logico_proxima_pagina = (numero_proxima_pagina << 16) & 0xFFFF0000;
        void* dato_proxima_pagina = dato + tamanio_lectura_pagina_actual;
        leer_memoria_principal_paginacion(args, inicio_logico_proxima_pagina, 0, dato_proxima_pagina, tamanio_total);
    }

    sem_post(pagina->semaforo);

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
    
    sem_wait(pagina->semaforo);

    pagina->PID = tabla_patota->PID;
    pagina->estado = MARCO_OCUPADO;
    pagina->bit_uso = true;
    actualizar_timestamp(pagina);
    actualizar_reloj(pagina);

    sem_wait(tabla_patota->semaforo);
    pagina->numero_pagina = tabla_patota->proximo_numero_pagina;
    tabla_patota->proximo_numero_pagina++;
    list_add(tabla_patota->paginas,pagina);
    sem_post(tabla_patota->semaforo);

    sem_post(pagina->semaforo);

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
    // Bloquear todas las paginas con semaforos??
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

// MEMORIA VIRTUAL

// LRU
void actualizar_timestamp(marco_t* pagina){
    free(pagina->timestamp);
    pagina->timestamp = temporal_get_string_time("%y%m%d%H%M%S");
}

// ALGORITMOS DE REEMPLAZO
int inicializar_algoritmo_de_reemplazo(t_config* config){
    char* string_algoritmo_reemplazo = config_get_string_value(config, "ALGORITMO_REEMPLAZO");
    if(strcmp(string_algoritmo_reemplazo,"LRU")==0){
        log_info(logger,"El algoritmo de reemplazo es: LRU");
        algoritmo_de_reemplazo = &algoritmo_lru;
        return EXIT_SUCCESS;
    }
    if(strcmp(string_algoritmo_reemplazo,"CLOCK")==0){
        log_info(logger,"El algoritmo de reemplazo es: Clock");
        algoritmo_de_reemplazo = &algoritmo_clock;
        return EXIT_SUCCESS;
    }
    log_error(logger,"%s: Algoritmo de reemplazo invalido",string_algoritmo_reemplazo);
    return EXIT_FAILURE;
}

marco_t* algoritmo_lru(){
    marco_t* pagina_victima;

    void* pagina_menos_recientemente_usada(void* args_1, void* args_2){
        marco_t* pagina_1 = (marco_t*) args_1;
        marco_t* pagina_2 = (marco_t*) args_2;
        char* timestamp_1 = pagina_1->timestamp;
        char* timestamp_2 = pagina_2->timestamp;
        int i = 0;

        for(;i < 11 && atoi(timestamp_1 + i) == atoi(timestamp_2 + i);i++);
        if(atoi(timestamp_1 + i) < atoi(timestamp_2 + i))
            return (void*) pagina_1;
        return (void*) pagina_2;
    }

    bool es_pagina_presente(void* args){
        marco_t* pagina = (marco_t*) args;
        return pagina->bit_presencia;
    }

    t_list* paginas_presentes = list_filter(lista_de_marcos, es_pagina_presente);

    pagina_victima = (marco_t*) list_get_minimum(paginas_presentes, pagina_menos_recientemente_usada);
    list_destroy(paginas_presentes);
    return pagina_victima;
}

marco_t* algoritmo_clock(){

    // Recorro el reloj hasta que encuentro un marco con bit de uso en 0
    while(aguja_reloj->marco->bit_uso == true){
        aguja_reloj->marco->bit_uso = false;
        aguja_reloj = aguja_reloj->siguiente;
    }

    return aguja_reloj->marco;  // El primer marco con bit de uso en 0 es la "PAGINA VICTIMA"
}

// Se llama al crear una pagina nueva
int actualizar_reloj(marco_t* pagina){

    // Si no es un marco en memoria principal, la aguja del reloj no se mueve    
    if(pagina->bit_presencia == false)
        return EXIT_SUCCESS;

    // La aguja del reloj avanza hasta encontrar el marco de la pagina creada
    while(aguja_reloj->marco->numero_marco != pagina->numero_marco){
        aguja_reloj = aguja_reloj->siguiente;
    }
    aguja_reloj = aguja_reloj->siguiente;   // El reloj apunta al marco siguiente de la pagina creada
    return EXIT_SUCCESS;
}

void proceso_swap(marco_t* pagina_necesitada){

    log_info(logger, "INICIANDO PROCESO SWAP");
    ejecutar_rutina(dump_memoria);

    // Bloquear con semaforos las paginas
    sem_wait(pagina_necesitada->semaforo);
    log_info(logger, "BLOQUEO MARCO NUMERO %d", pagina_necesitada->numero_marco);

    // Copiamos en un buffer el contenido de la pagina necesitada
    char* buffer_necesitado = leer_marco(pagina_necesitada);

    // Buscamos si hay marcos libres en memoria principal
    bool es_marco_libre_en_memoria_principal(void* args){
        marco_t* marco = (marco_t*) args;
        return marco->bit_presencia && (marco->estado == MARCO_LIBRE);
    }

    marco_t* pagina_victima = list_find(lista_de_marcos, es_marco_libre_en_memoria_principal);
    
    // Si no hay marcos libres
    if(pagina_victima == NULL){

        // Buscamos una pagina victima con algoritmo de reemplazo
        pagina_victima = algoritmo_de_reemplazo();
        sem_wait(pagina_victima->semaforo);
        log_info(logger, "BLOQUEO MARCO NUMERO %d", pagina_victima->numero_marco);
        // Movemos la informacion de la pagina victima a donde estaba la pagina necesitada en memoria virtual 
        char* buffer_victima = leer_marco(pagina_victima);
        escribir_marco(pagina_necesitada, buffer_victima);
        free(buffer_victima);
    }
    else{
        sem_wait(pagina_victima->semaforo);
        log_info(logger, "BLOQUEO MARCO NUMERO %d", pagina_victima->numero_marco);
    }

    // Movemos el contenido de la pagina necesitada a memoria principal
    escribir_marco(pagina_victima, buffer_necesitado);
    free(buffer_necesitado);
        
    /*
        typedef struct{
            int numero_pagina;          // Se mantiene igual
            int numero_marco;           // Se intercambian
            bool bit_presencia;         // Se intercambian
            char* timestamp;            // No hace falta (lo actualiza la funcion de lectura/escritura)
            bool bit_uso;               // No hace falta (lo actualiza la funcion de lectura/escritura)
            int fragmentacion_interna;  // Se mantiene igual (la pagina conserva su frag interna)
            enum estado_marco estado;   // Se mantiene igual (la pagina conserva su estado)
            int PID;                    // Se mantiene igual (la pagina conserva su PID)
        } marco_t;
    */
    
    // Intercambiamos numeros de marco (pasan del marco de RAM al marco de M.VIRTUAL y viceversa)
    int n_marco_auxiliar = pagina_victima->numero_marco;
    pagina_victima->numero_marco = pagina_necesitada->numero_marco;
    pagina_necesitada->numero_marco = n_marco_auxiliar;

    pagina_necesitada->bit_presencia = true;    // Se mueve a RAM
    pagina_victima->bit_presencia = false;       // Se mueve a Memoria Virtual

    // Actualizamos el reloj de clock (la aguja quedo apuntando a la hora de la pagina victima)
    aguja_reloj->marco = pagina_necesitada; // La pagina traida a memoria ocupa el lugar de la pagina victima
    aguja_reloj = aguja_reloj->siguiente;   // El reloj queda apuntando al siguiente marco

    // Reordenamos la lista de marcos por numero de marco
    bool tiene_menor_numero_marco(void* args_1, void* args_2){
        marco_t* marco_1 = args_1;
        marco_t* marco_2 = args_2;
        return marco_1->numero_marco < marco_2->numero_marco;
    }

    list_sort(lista_de_marcos, tiene_menor_numero_marco);    

    // Liberar los semaforos de las paginas
    sem_post(pagina_necesitada->semaforo);
    sem_post(pagina_victima->semaforo);

    log_info(logger, "FINALIZANDO SWAP");
    log_info(logger, "DESBLOQUEO MARCOs NUMERO %d %d", pagina_necesitada->numero_marco, pagina_victima->numero_marco);
    ejecutar_rutina(dump_memoria);
}

char* leer_marco(marco_t* marco){
    char* informacion = malloc(tamanio_pagina);
    uint32_t direccion_fisica;

    // Si el marco esta en memoria principal
    if(marco->numero_marco < cantidad_marcos_memoria_principal){
        direccion_fisica = (marco->numero_marco) * tamanio_pagina;
        memcpy(informacion, memoria_principal + direccion_fisica, tamanio_pagina);
    }

    /// Si el marco esta en memoria virtual
    else{
        direccion_fisica = marco->numero_marco - cantidad_marcos_memoria_principal;
        direccion_fisica *= tamanio_pagina;
        fseek(memoria_virtual, direccion_fisica, SEEK_SET);
        fread(informacion, sizeof(char), tamanio_pagina, memoria_virtual);
    }
        
    return informacion;
}

void escribir_marco(marco_t* marco, char* informacion){
    uint32_t direccion_fisica;

    // Si el marco esta en memoria principal
    if(marco->numero_marco < cantidad_marcos_memoria_principal){
        direccion_fisica = (marco->numero_marco) * tamanio_pagina;
        memcpy(memoria_principal + direccion_fisica, informacion, tamanio_pagina);
    }

    // Si el marco esta en memoria virtual
    else{
        direccion_fisica = marco->numero_marco - cantidad_marcos_memoria_principal;
        direccion_fisica *= tamanio_pagina;
        fseek(memoria_virtual, direccion_fisica, SEEK_SET);
        fwrite(informacion, sizeof(char), tamanio_pagina, memoria_virtual);
    }
}

uint32_t espacio_disponible_paginacion(){
    uint32_t cantidad_marcos_libres = 0;

    // Por cada marco, nos fijamos si esta libre o no
    t_list_iterator* iterador = list_iterator_create(lista_de_marcos);    // Creamos el iterador
    marco_t* marco;

    while(list_iterator_has_next(iterador)){
        marco = list_iterator_next(iterador);
        if(marco->estado == MARCO_LIBRE)
            cantidad_marcos_libres++;
    }

    list_iterator_destroy(iterador);    // Liberamos el iterador
    return cantidad_marcos_libres * tamanio_pagina;
}