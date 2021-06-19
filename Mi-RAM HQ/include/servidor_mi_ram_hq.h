#ifndef SERVIDOR_MI_RAM_HQ_H_
#define SERVIDOR_MI_RAM_HQ_H_

#include "sockets_shared.h"
#include "segmentacion.h"

int loguear_mensaje(char* payload);
int iniciar_patota(char* payload);
int iniciar_tripulante(char* payload, void** tabla, uint32_t* direccion_logica_TCB);
int recibir_ubicacion_tripulante(char* payload, void** tabla, uint32_t* direccion_logica_TCB);
int enviar_proxima_tarea(int tripulante_fd, char* payload, void** tabla, uint32_t* dir_log_tcb);
int expulsar_tripulante(char* payload, void** tabla, uint32_t* direccion_logica_TCB);

#endif