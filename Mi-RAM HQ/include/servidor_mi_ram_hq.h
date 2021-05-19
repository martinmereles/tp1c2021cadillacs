#ifndef SERVIDOR_MI_RAM_HQ_H_
#define SERVIDOR_MI_RAM_HQ_H_

#include "sockets_shared.h"
#include "memoria_principal.h"

int loguear_mensaje(char* payload);
int iniciar_patota(char* payload);
int iniciar_tripulante(char* payload, tabla_segmentos_t** tabla, uint32_t* direccion_logica_TCB);
int recibir_ubicacion_tripulante(char* payload, tabla_segmentos_t** tabla, uint32_t* direccion_logica_TCB);
int enviar_proxima_tarea(char* payload, tabla_segmentos_t** tabla, uint32_t* direccion_logica_TCB);
int expulsar_tripulante(char* payload, tabla_segmentos_t** tabla, uint32_t* direccion_logica_TCB);

#endif