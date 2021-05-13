#ifndef SERVIDOR_MI_RAM_HQ_H_
#define SERVIDOR_MI_RAM_HQ_H_

#include "sockets_shared.h"

void loguear_mensaje(char* payload);
void iniciar_patota(char* payload);
void iniciar_tripulante(char* payload);
void recibir_ubicacion_tripulante(char* payload);
void enviar_proxima_tarea(char* payload);
void expulsar_tripulante(char* payload);

#endif