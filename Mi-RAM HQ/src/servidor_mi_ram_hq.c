#include "servidor_mi_ram_hq.h"

int loguear_mensaje(char* payload){
    log_info(logger, "Recibi el mensaje: %s", payload);
    return EXIT_SUCCESS;
}

int iniciar_patota(char* payload){
    uint32_t PID;
    uint32_t longitud_tareas;
    char* tareas;
    int offset = 0;

    log_info(logger, "Iniciando estructuras de la patota");

    // Leo el PID
    memcpy(&PID, payload + offset, sizeof(uint32_t));
    offset += sizeof(uint32_t);
    log_info(logger, "El PID del proceso es: %d",PID);
    
    // Leo la longitud de las tareas
    memcpy(&longitud_tareas, payload + offset, sizeof(uint32_t));
    offset += sizeof(uint32_t);
    log_info(logger, "La longitud de la tarea es: %d",longitud_tareas);

    // Leo la lista de tareas
    tareas = malloc(longitud_tareas);
    memcpy(tareas, payload + offset, longitud_tareas);
    offset += sizeof(longitud_tareas);
    log_info(logger, "La lista de tareas del proceso es: %s",tareas);

    // Buscamos un espacio en memoria para el PCB y las tareas
    fila_tabla_segmentos_t* fila_PCB = reservar_segmento(TAMANIO_PCB);
    if(fila_PCB == NULL) {
        log_error(logger, "ERROR. No hay espacio para guardar el PCB de la patota. %d",TAMANIO_PCB);
        return EXIT_FAILURE;
    }

    log_info(logger, "HOLA MAINIGI");

    fila_tabla_segmentos_t* fila_tareas = reservar_segmento(longitud_tareas);
    if(fila_tareas == NULL) {
        log_error(logger, "ERROR. No hay espacio para guardar las tareas de la patota.");
        liberar_segmento(fila_PCB);
        return EXIT_FAILURE;
    }

    log_info(logger, "HOLA MAINIGI");

    // Crear una tabla de segmentos para la patota
    tabla_segmentos_t* tabla_patota;
    log_info(logger, "Chau perro");
    tabla_patota = crear_tabla_segmentos();

    log_info(logger, "Chau perro");
    int direccion_logica_PCB = agregar_fila(tabla_patota, fila_PCB);
    log_info(logger, "Chau perro");
    int direccion_logica_tareas = agregar_fila(tabla_patota, fila_tareas);
    log_info(logger, "Chau perro");

    log_info(logger, "HOLA MAINIGI");

    log_info(logger, "Direccion logica PCB: %x",direccion_logica_PCB);
    log_info(logger, "Direccion logica tareas: %x",direccion_logica_tareas);

    // Agregar la tabla a la lista de tablas
    list_add(tablas_de_segmentos, tabla_patota);

        log_info(logger, "HOLA MAINIGI");

    // Guardo el PID y la direccion logica de las tareas en el PCB
    escribir_memoria_principal(tabla_patota, direccion_logica_PCB + DESPL_PID, &PID, sizeof(uint32_t));
    escribir_memoria_principal(tabla_patota, direccion_logica_PCB + DESPL_TAREAS, &direccion_logica_tareas, sizeof(uint32_t));  

    // Guardo las tareas en el segmento de tareas
    escribir_memoria_principal(tabla_patota, direccion_logica_tareas, tareas, longitud_tareas);
    log_info(logger, "Estructuras de la patota inicializadas exitosamente");
    return EXIT_SUCCESS;  
}   

int iniciar_tripulante(char* payload){
    int PID, posicion_X, posicion_Y, TID;
    int offset = 0;
    
    log_info(logger, "Iniciando estructuras del tripulante");

    memcpy(&PID, payload + offset, sizeof(int));
	offset += sizeof(int);
    log_info(logger, "El PID del tripulante es: %d",PID);

    memcpy(&TID, payload + offset, sizeof(int));
	offset += sizeof(int);
    log_info(logger, "El TID del tripulante es: %d",TID);

    memcpy(&posicion_X, payload + offset, sizeof(int));
	offset += sizeof(int);

    memcpy(&posicion_Y, payload + offset, sizeof(int));
	offset += sizeof(int);
    log_info(logger, "La posicion inicial del tripulante es: (%d,%d)",posicion_X,posicion_Y);

    // Buscamos un espacio en memoria para el TCB
    fila_tabla_segmentos_t* fila_TCB = reservar_segmento(TAMANIO_TCB);

    log_info(logger, "wacha piola");
    if(fila_TCB == NULL) {
        log_error(logger, "ERROR. No hay espacio para guardar el TCB del tripulante. %d",TAMANIO_TCB);
        return EXIT_FAILURE;
    }
    log_info(logger, "wacha piola");
    // Buscar la tabla que coincide con el PID y agregar la fila para el segmento del TCB
    tabla_segmentos_t* tabla = obtener_tabla_patota(PID);
    if(tabla == NULL){
        log_error(logger,"No se encontro la tabla de segmentos de la patota");
    }
    log_info(logger, "wacha piola");
    int dir_log_tcb = agregar_fila(tabla, fila_TCB);
    log_info(logger, "wacha piola");
    // Guardo el TID, el estado, la posicion, el identificador de la proxima instruccion y la direccion logica del PCB
    char estado = 'N';
    uint32_t id_proxima_instruccion = 1;
    uint32_t dir_log_pcb = DIR_LOG_PCB;
    log_info(logger, "wacha piola");
    escribir_memoria_principal(tabla, dir_log_tcb + DESPL_TID, &TID, sizeof(uint32_t));
    escribir_memoria_principal(tabla, dir_log_tcb + DESPL_ESTADO, &estado, sizeof(char));
    escribir_memoria_principal(tabla, dir_log_tcb + DESPL_POS_X, &posicion_X, sizeof(uint32_t));
    escribir_memoria_principal(tabla, dir_log_tcb + DESPL_POS_Y, &posicion_Y, sizeof(uint32_t));
    escribir_memoria_principal(tabla, dir_log_tcb + DESPL_PROX_INSTR, &id_proxima_instruccion, sizeof(uint32_t));
    escribir_memoria_principal(tabla, dir_log_tcb + DESPL_DIR_PCB, &dir_log_pcb, sizeof(uint32_t));
 
    return EXIT_SUCCESS;
}

int recibir_ubicacion_tripulante(char* payload){
    return EXIT_SUCCESS;
}

int enviar_proxima_tarea(char* payload){
    return EXIT_SUCCESS;
}

int expulsar_tripulante(char* payload){
    return EXIT_SUCCESS;
}
