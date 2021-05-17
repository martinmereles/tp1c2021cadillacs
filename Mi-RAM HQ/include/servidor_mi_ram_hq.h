#ifndef SERVIDOR_MI_RAM_HQ_H_
#define SERVIDOR_MI_RAM_HQ_H_

#include "sockets_shared.h"
#include "memoria_principal.h"

int loguear_mensaje(char* payload);
int iniciar_patota(char* payload);
int iniciar_tripulante(char* payload);
int recibir_ubicacion_tripulante(char* payload);
int enviar_proxima_tarea(char* payload);
int expulsar_tripulante(char* payload);

#endif