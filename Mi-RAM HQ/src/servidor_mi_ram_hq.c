#include "servidor_mi_ram_hq.h"

int loguear_mensaje(char* payload){
    log_info(logger, "Recibi el mensaje: %s", payload);
    return EXIT_SUCCESS;
}

int iniciar_patota(char* payload){
    uint32_t PID;
    uint32_t cantidad_tripulantes;
    uint32_t longitud_tareas;
    char* tareas;
    int offset = 0;

    log_info(logger, "Iniciando estructuras de la patota");
 
    // Leo el PID
    memcpy(&PID, payload + offset, sizeof(uint32_t));
    offset += sizeof(uint32_t);
    log_info(logger, "El PID de la patota es: %d",PID);

   // Leo la cantidad de tripulantes
    memcpy(&cantidad_tripulantes, payload + offset, sizeof(uint32_t));
    offset += sizeof(uint32_t);
    log_info(logger, "La cantidad de tripulantes de la patota es: %d",cantidad_tripulantes);

    // Leo la longitud de las tareas
    memcpy(&longitud_tareas, payload + offset, sizeof(uint32_t));
    offset += sizeof(uint32_t);
    log_info(logger, "La longitud de la tarea es: %d",longitud_tareas);

    // Leo la lista de tareas
    tareas = malloc(longitud_tareas);
    memcpy(tareas, payload + offset, longitud_tareas);
    offset += sizeof(longitud_tareas);
    // log_info(logger, "La lista de tareas del proceso es: %s",tareas);

    // Revisamos si hay espacio suficiente en memoria para la patota
    uint32_t espacio_necesario = TAMANIO_PCB + cantidad_tripulantes * TAMANIO_TCB + longitud_tareas;

    if(espacio_necesario > espacio_disponible()){
        log_error(logger, "ERROR: iniciar_patota. No hay espacio en memoria suficiente para crear la patota");
        free(tareas);
        return COD_INICIAR_PATOTA_ERROR;
    }
        
    // Creamos la patota
    if(crear_patota(PID, longitud_tareas, tareas) == EXIT_FAILURE){
        log_error(logger, "NO SE PUDO CREAR LA PATOTA.");
        free(tareas);
        return COD_INICIAR_PATOTA_ERROR;
    }
    free(tareas);
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

    dibujar_tripulante_mapa(TID, posicion_X, posicion_Y);

    if(crear_tripulante(tabla, dir_log_tcb, PID, TID, posicion_X, posicion_Y) == EXIT_FAILURE){
        log_error(logger, "NO SE PUDO CREAR AL TRIPULANTE.");
        borrar_tripulante_mapa(TID);
        return COD_INICIAR_TRIPULANTE_ERROR;
    }

    return COD_INICIAR_TRIPULANTE_OK; 
}

int recibir_ubicacion_tripulante(char* payload, void** tabla, uint32_t* direccion_logica_TCB){
    uint32_t nueva_posicion_X, nueva_posicion_Y;
    uint32_t actual_posicion_X, actual_posicion_Y;
    uint32_t TID;
    int offset = 0;

    // Recibo la nueva posicion segun el submodulo tripulante    
    log_info(logger, "Recibiendo ubicacion de un tripulante");

    memcpy(&nueva_posicion_X, payload + offset, sizeof(uint32_t));
	offset += sizeof(uint32_t);

    memcpy(&nueva_posicion_Y, payload + offset, sizeof(uint32_t));
	offset += sizeof(uint32_t);
 
    // Leo la posicion actual y el TID en memoria RAM
    leer_memoria_principal(*tabla, *direccion_logica_TCB, DESPL_POS_X, &actual_posicion_X, sizeof(uint32_t));
    leer_memoria_principal(*tabla, *direccion_logica_TCB, DESPL_POS_Y, &actual_posicion_Y, sizeof(uint32_t));
    leer_memoria_principal(*tabla, *direccion_logica_TCB, DESPL_TID, &TID, sizeof(uint32_t));
    
    // Intento desplazar al tripulante en el mapa
    int desplazamiento_X = nueva_posicion_X - actual_posicion_X;
    int desplazamiento_Y = nueva_posicion_Y - actual_posicion_Y;

    if(desplazar_tripulante_mapa(TID, desplazamiento_X, desplazamiento_Y) != EXIT_SUCCESS)
        return EXIT_FAILURE;

    // Si se desplazo con exito, escribo la nueva ubicacion del tripulante en memoria RAM
    log_info(logger, "Nueva ubicacion del tripulante: (%d,%d)",nueva_posicion_X,nueva_posicion_Y);

    escribir_memoria_principal(*tabla, *direccion_logica_TCB, DESPL_POS_X, &nueva_posicion_X, sizeof(uint32_t));
    escribir_memoria_principal(*tabla, *direccion_logica_TCB, DESPL_POS_Y, &nueva_posicion_Y, sizeof(uint32_t));

    return EXIT_SUCCESS;
}

int enviar_ubicacion_tripulante(int tripulante_fd, char* payload, void** tabla, uint32_t* direccion_logica_TCB){
    uint32_t posicion_X, posicion_Y;

    log_info(logger, "Leyendo ubicacion");

    // Leemos la ubicacion
    leer_memoria_principal(*tabla, *direccion_logica_TCB, DESPL_POS_X, &posicion_X, sizeof(uint32_t));
    leer_memoria_principal(*tabla, *direccion_logica_TCB, DESPL_POS_Y, &posicion_Y, sizeof(uint32_t));

    log_info(logger, "Enviando posicion: (%d,%d)", posicion_X, posicion_Y);

    // enviamos la ubicacion al submodulo tripulante
    int exit_status;
	uint32_t longitud_stream = sizeof(uint32_t) * 2;
	char* stream = malloc(longitud_stream);
	int offset = 0;

	memcpy(stream + offset, &posicion_X, sizeof(uint32_t));
	offset += sizeof(uint32_t);

	memcpy(stream + offset, &posicion_Y, sizeof(uint32_t));
	offset += sizeof(uint32_t);

	exit_status = enviar_operacion(tripulante_fd, COD_UBICACION_TRIPULANTE, stream, longitud_stream);
	free(stream);

    if(exit_status != EXIT_SUCCESS)
        log_error(logger, "ERROR. No se pudo enviar la ubicacion al submodulo tripulante");

    return exit_status;
}


int recibir_estado_tripulante(char* payload, void** tabla, uint32_t* direccion_logica_TCB){
    uint32_t estado_tripulante;
    int offset = 0;
    
    log_info(logger, "Recibiendo estado de un tripulante");

    memcpy(&estado_tripulante, payload + offset, sizeof(char));
	offset += sizeof(char);

    log_info(logger, "Nuevo estado del tripulante: %c", estado_tripulante);

    escribir_memoria_principal(*tabla, *direccion_logica_TCB, DESPL_ESTADO, &estado_tripulante, sizeof(char));

    return EXIT_SUCCESS;
}

int enviar_estado_tripulante(int tripulante_fd, char* payload, void** tabla, uint32_t* direccion_logica_TCB){
    char estado_tripulante;

    // Leemos el estado
    log_info(logger, "Leyendo estado");
    leer_memoria_principal(*tabla, *direccion_logica_TCB, DESPL_ESTADO, &estado_tripulante, sizeof(char));

    log_info(logger, "Enviando estado: %c", estado_tripulante);

    // Enviamos el estado al submodulo tripulante
    int exit_status;
	uint32_t longitud_stream = sizeof(char);
	char* stream = malloc(longitud_stream);
	int offset = 0;

	memcpy(stream + offset, &estado_tripulante, sizeof(char));
	offset += sizeof(char);

	exit_status = enviar_operacion(tripulante_fd, COD_ESTADO_TRIPULANTE, stream, longitud_stream);
	free(stream);

    if(exit_status != EXIT_SUCCESS)
        log_error(logger, "ERROR. No se pudo enviar el estado al submodulo tripulante");

    return exit_status;
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

    // Leemos el TID del tripulante en memoria principal para poder borrarlo del mapa
    uint32_t TID;
    leer_memoria_principal(*tabla, *direccion_logica_TCB, DESPL_TID, &TID, sizeof(uint32_t));

    if(borrar_tripulante_mapa(TID) != EXIT_SUCCESS)
        return EXIT_FAILURE;

    eliminar_tripulante(*tabla, *direccion_logica_TCB);

    *tabla = NULL;

    return EXIT_SUCCESS;
}