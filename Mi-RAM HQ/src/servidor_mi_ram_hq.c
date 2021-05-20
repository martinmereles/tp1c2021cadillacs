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

    // Crear una tabla de segmentos para la patota
    tabla_segmentos_t* tabla_patota;
    tabla_patota = crear_tabla_segmentos();

    // Buscamos un espacio en memoria para el PCB y las tareas
    fila_tabla_segmentos_t* fila_PCB = crear_fila(tabla_patota, TAMANIO_PCB);
    if(fila_PCB == NULL) {
        log_error(logger, "ERROR. No hay espacio para guardar el PCB de la patota. %d",TAMANIO_PCB);
        destruir_tabla_segmentos(tabla_patota);
        return EXIT_FAILURE;
    }

    log_info(logger,"La cantidad de filas es: %d",cantidad_filas(tabla_patota));
    log_info(logger,"El nro de segmento es: %d",fila_PCB->numero_segmento);

    fila_tabla_segmentos_t* fila_tareas = crear_fila(tabla_patota, longitud_tareas);
    if(fila_tareas == NULL) {
        log_error(logger, "ERROR. No hay espacio para guardar las tareas de la patota.");
        destruir_tabla_segmentos(tabla_patota);
        return EXIT_FAILURE;
    }

    log_info(logger,"La cantidad de filas es: %d",cantidad_filas(tabla_patota));
    log_info(logger,"El nro de segmento es: %d",fila_tareas->numero_segmento);

    int direccion_logica_PCB = direccion_logica(fila_PCB);
    int direccion_logica_tareas = direccion_logica(fila_tareas);

    log_info(logger, "Direccion logica PCB: %x",direccion_logica_PCB);
    log_info(logger, "Direccion logica tareas: %x",direccion_logica_tareas);

    // Agregar la tabla a la lista de tablas
    list_add(tablas_de_segmentos, tabla_patota);

    // Guardo el PID y la direccion logica de las tareas en el PCB
    escribir_memoria_principal(tabla_patota, direccion_logica_PCB + DESPL_PID, &PID, sizeof(uint32_t));
    escribir_memoria_principal(tabla_patota, direccion_logica_PCB + DESPL_TAREAS, &direccion_logica_tareas, sizeof(uint32_t));  

    // Guardo las tareas en el segmento de tareas
    escribir_memoria_principal(tabla_patota, direccion_logica_tareas, tareas, longitud_tareas);
    log_info(logger, "Estructuras de la patota inicializadas exitosamente");
    return EXIT_SUCCESS;  
}   

int iniciar_tripulante(char* payload, tabla_segmentos_t** tabla, uint32_t* dir_log_tcb){
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

    // Buscar la tabla que coincide con el PID
    *tabla = obtener_tabla_patota(PID);
    if(*tabla == NULL){
        log_error(logger,"ERROR. No se encontro la tabla de segmentos de la patota");
        return EXIT_FAILURE;
    }

    // Buscamos un espacio en memoria para el TCB
    // Agregamos la fila para el segmento del TCB a la tabla de la patota
    fila_tabla_segmentos_t* fila_TCB = crear_fila(*tabla, TAMANIO_TCB);

    if(fila_TCB == NULL) {
        log_error(logger, "ERROR. No hay espacio para guardar el TCB del tripulante. %d",TAMANIO_TCB);
        quitar_y_destruir_fila(*tabla, fila_TCB->numero_segmento);
        return EXIT_FAILURE;
    }

    *dir_log_tcb = direccion_logica(fila_TCB);

    // Guardo el TID, el estado, la posicion, el identificador de la proxima instruccion y la direccion logica del PCB
    char estado = 'N';
    uint32_t id_proxima_instruccion = 0;
    uint32_t dir_log_pcb = DIR_LOG_PCB;
    escribir_memoria_principal(*tabla, *dir_log_tcb + DESPL_TID, &TID, sizeof(uint32_t));
    escribir_memoria_principal(*tabla, *dir_log_tcb + DESPL_ESTADO, &estado, sizeof(char));
    escribir_memoria_principal(*tabla, *dir_log_tcb + DESPL_POS_X, &posicion_X, sizeof(uint32_t));
    escribir_memoria_principal(*tabla, *dir_log_tcb + DESPL_POS_Y, &posicion_Y, sizeof(uint32_t));
    escribir_memoria_principal(*tabla, *dir_log_tcb + DESPL_PROX_INSTR, &id_proxima_instruccion, sizeof(uint32_t));
    escribir_memoria_principal(*tabla, *dir_log_tcb + DESPL_DIR_PCB, &dir_log_pcb, sizeof(uint32_t));
 
    return EXIT_SUCCESS;
}

int recibir_ubicacion_tripulante(char* payload, tabla_segmentos_t** tabla, uint32_t* direccion_logica_TCB){
    uint32_t posicion_X, posicion_Y;
    int offset = 0;
    
    log_info(logger, "Recibiendo ubicacion de un tripulante");

    memcpy(&posicion_X, payload + offset, sizeof(uint32_t));
	offset += sizeof(uint32_t);

    memcpy(&posicion_Y, payload + offset, sizeof(uint32_t));
	offset += sizeof(uint32_t);

    log_info(logger, "Nueva ubicacion del tripulante: (%d,%d)",posicion_X,posicion_Y);

    escribir_memoria_principal(*tabla, *direccion_logica_TCB + DESPL_POS_X, &posicion_X, sizeof(uint32_t));
    escribir_memoria_principal(*tabla, *direccion_logica_TCB + DESPL_POS_Y, &posicion_Y, sizeof(uint32_t));

    return EXIT_SUCCESS;
}

int enviar_proxima_tarea(int tripulante_fd, char* payload, tabla_segmentos_t** tabla, uint32_t* dir_log_tcb){
    uint32_t id_prox_tarea, dir_log_pcb, dir_log_tareas;
    char* tarea;

    log_info(logger, "Enviando proxima tarea");
    log_info(logger, "Leyendo tareas");
    // Leemos la lista de tareas
    leer_memoria_principal(*tabla, *dir_log_tcb + DESPL_PROX_INSTR, &id_prox_tarea, sizeof(uint32_t));
    leer_memoria_principal(*tabla, *dir_log_tcb + DESPL_DIR_PCB, &dir_log_pcb, sizeof(uint32_t));
    leer_memoria_principal(*tabla, dir_log_pcb + DESPL_TAREAS, &dir_log_tareas, sizeof(uint32_t));
    leer_tarea_memoria_principal(*tabla, dir_log_tareas, &tarea, id_prox_tarea);

    if(tarea == NULL){
        log_error(logger, "ERROR. No se encontro la proxima instruccion");
        return EXIT_FAILURE;
    }

    log_info(logger, "Enviando proxima tarea: %s", tarea);
    // enviamos la proxima tarea al submodulo tripulante
    enviar_operacion(tripulante_fd, COD_PROXIMA_TAREA, tarea, strlen(tarea) + 1);
    free(tarea);

    log_info(logger, "Incrementamos proxima tarea");
    // Sumamos 1 al identificador de la proxima instruccion
    id_prox_tarea++;
    escribir_memoria_principal(*tabla, *dir_log_tcb + DESPL_PROX_INSTR, &id_prox_tarea, sizeof(uint32_t));

    return EXIT_SUCCESS;
}

int expulsar_tripulante(char* payload, tabla_segmentos_t** tabla, uint32_t* direccion_logica_TCB){
    
    log_info(logger,"Expulsando tripulante");

    // Quitamos la fila de la tabla de segmentos (tambien se encarga de liberar la memoria)
    quitar_y_destruir_fila(*tabla, numero_de_segmento(*direccion_logica_TCB));
 
    // Si no quedan tripulantes en la patota, destruimos su tabla y liberamos sus recursos
    if(cantidad_filas(*tabla) <= 2){
        quitar_y_destruir_tabla(*tabla);
    }

    return EXIT_SUCCESS;
}
