#include "servidor_mi_ram_hq.h"

void loguear_mensaje(char* payload){
    log_info(logger, "Recibi el mensaje: %s", payload);
}

void iniciar_patota(char* payload){
    int PID;
    int longitud_lista_de_tareas;
    char* lista_de_tareas;
    int offset = 0;

    log_info(logger, "Iniciando estructuras de la patota");

    memcpy(&PID, payload + offset, sizeof(int));
    offset += sizeof(int);

    log_info(logger, "El PID del proceso es: %d",PID);

    memcpy(&longitud_lista_de_tareas, payload + offset, sizeof(int));
    offset += sizeof(int);

    log_info(logger, "La longitud de la tarea es: %d",longitud_lista_de_tareas);

    lista_de_tareas = malloc(longitud_lista_de_tareas);
    memcpy(lista_de_tareas, payload + offset, longitud_lista_de_tareas);
    offset += sizeof(longitud_lista_de_tareas);

    log_info(logger, "La lista de tareas del proceso es: %s",lista_de_tareas);
}

void iniciar_tripulante(char* payload){
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

}

void recibir_ubicacion_tripulante(char* payload){

}

void enviar_proxima_tarea(char* payload){

}

void expulsar_tripulante(char* payload){

}