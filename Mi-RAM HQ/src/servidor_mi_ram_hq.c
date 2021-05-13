#include "servidor_mi_ram_hq.h"

void loguear_mensaje(char* payload){
    log_info(logger, "Recibi el mensaje: %s", payload);
}

void iniciar_patota(char* payload){
    log_info(logger, "Iniciando estructuras de la patota");
}

void iniciar_tripulante(char* payload){
    log_info(logger, "Iniciando estructuras del tripulante");
}

void recibir_ubicacion_tripulante(char* payload){

}

void enviar_proxima_tarea(char* payload){

}

void expulsar_tripulante(char* payload){

}