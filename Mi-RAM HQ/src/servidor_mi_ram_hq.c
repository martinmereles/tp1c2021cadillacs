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
        log_error(logger, "ERROR. No hay espacio para guardar la patota.");
        return EXIT_FAILURE;
    }

    fila_tabla_segmentos_t* fila_tareas = reservar_segmento(longitud_tareas);
    if(fila_tareas == NULL) {
        log_error(logger, "ERROR. No hay espacio para guardar la patota.");
        liberar_segmento(fila_PCB);
        return EXIT_FAILURE;
    }

    // Crear una tabla de segmentos para la patota
    tabla_segmentos_t* tabla_patota;
    tabla_patota = malloc(sizeof(tabla_segmentos_t));
    tabla_patota->filas = malloc(2 * sizeof(fila_tabla_segmentos_t*));
    (tabla_patota->filas)[0] = fila_PCB;
    (tabla_patota->filas)[1] = fila_tareas;

    // Agregar la tabla a la lista de tablas??
    // tablas_de_segmentos_de_patotas.

    // Guardo el PID y la direccion logica de las tareas en el PCB
    
    int dir_log_tareas = DIR_LOG_TAREAS;

    memcpy_tabla_segmentos(tabla_patota, DIR_LOG_PCB + DESPL_PID, &PID, sizeof(uint32_t));
    memcpy_tabla_segmentos(tabla_patota, DIR_LOG_PCB + DESPL_TAREAS, &dir_log_tareas, sizeof(uint32_t));  

    // Guardo las tareas en el segmento de tareas
    memcpy_tabla_segmentos(tabla_patota, DIR_LOG_TAREAS, tareas, longitud_tareas);
    return EXIT_SUCCESS;  
}   

int calcular_direccion_fisica(tabla_segmentos_t* tabla, uint32_t direccion_logica){
    int numero_fila = (direccion_logica & 0x80000000) >> 31;
    return tabla->filas[numero_fila]->inicio + (direccion_logica & 0x7FFFFFFF);
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