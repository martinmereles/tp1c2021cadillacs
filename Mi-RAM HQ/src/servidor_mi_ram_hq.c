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

    // Creamos la patota
    if(crear_patota(PID, longitud_tareas, tareas) == EXIT_FAILURE){
        log_error(logger, "NO SE PUDO CREAR LA PATOTA.");
        return COD_INICIAR_PATOTA_ERROR;
    }

    return COD_INICIAR_PATOTA_OK; 
}   

int iniciar_tripulante(char* payload, void** tabla, uint32_t* dir_log_tcb){
    uint32_t PID, posicion_X, posicion_Y, TID;
    int offset = 0;
    
    log_info(logger, "Iniciando estructuras del tripulante");

    memcpy(&PID, payload + offset, sizeof(uint32_t));
	offset += sizeof(uint32_t);
    log_info(logger, "El PID del tripulante es: %d",PID);

    memcpy(&TID, payload + offset, sizeof(uint32_t));
	offset += sizeof(uint32_t);
    log_info(logger, "El TID del tripulante es: %d",TID);

    memcpy(&posicion_X, payload + offset, sizeof(uint32_t));
	offset += sizeof(uint32_t);

    memcpy(&posicion_Y, payload + offset, sizeof(uint32_t));
	offset += sizeof(uint32_t);
    log_info(logger, "La posicion inicial del tripulante es: (%d,%d)",posicion_X,posicion_Y);

    if(crear_tripulante(tabla, dir_log_tcb, PID, TID, posicion_X, posicion_Y) == EXIT_FAILURE){
        log_error(logger,"ERROR. NO SE PUDO CREAR AL TRIPULANTE. AVISAR AL DISCORDIADOR???");
    }

    return EXIT_SUCCESS;
}

int recibir_ubicacion_tripulante(char* payload, void** tabla, uint32_t* direccion_logica_TCB){
    uint32_t posicion_X, posicion_Y;
    int offset = 0;
    
    log_info(logger, "Recibiendo ubicacion de un tripulante");

    memcpy(&posicion_X, payload + offset, sizeof(uint32_t));
	offset += sizeof(uint32_t);

    memcpy(&posicion_Y, payload + offset, sizeof(uint32_t));
	offset += sizeof(uint32_t);

    log_info(logger, "Nueva ubicacion del tripulante: (%d,%d)",posicion_X,posicion_Y);

    escribir_memoria_principal(*tabla, *direccion_logica_TCB, DESPL_POS_X, &posicion_X, sizeof(uint32_t));
    escribir_memoria_principal(*tabla, *direccion_logica_TCB, DESPL_POS_Y, &posicion_Y, sizeof(uint32_t));

    return EXIT_SUCCESS;
}

int enviar_proxima_tarea(int tripulante_fd, char* payload, void** tabla, uint32_t* dir_log_tcb){
    uint32_t id_prox_tarea;
    char* tarea;

    log_info(logger, "Enviando proxima tarea");
    log_info(logger, "Leyendo tareas");

    // Leemos la lista de tareas
    leer_memoria_principal(*tabla, *dir_log_tcb, DESPL_PROX_INSTR, &id_prox_tarea, sizeof(uint32_t));
    leer_tarea_memoria_principal(*tabla, &tarea, id_prox_tarea);

    if(tarea == NULL){
        log_error(logger, "ERROR. No se encontro la proxima instruccion");
        return EXIT_FAILURE;
    }

    log_info(logger, "Enviando proxima tarea: %s", tarea);
    // enviamos la proxima tarea al submodulo tripulante
    enviar_operacion(tripulante_fd, COD_PROXIMA_TAREA, tarea, strlen(tarea) + 1);
    free(tarea);

    // Escribimos el identificador de la proxima instruccion
    id_prox_tarea++;
    escribir_memoria_principal(*tabla, *dir_log_tcb, DESPL_PROX_INSTR, &id_prox_tarea, sizeof(uint32_t));

    return EXIT_SUCCESS;
}

int expulsar_tripulante(char* payload, void** tabla, uint32_t* direccion_logica_TCB){
    
    log_info(logger,"Expulsando tripulante");

    eliminar_tripulante(*tabla, *direccion_logica_TCB);

    return EXIT_SUCCESS;
}