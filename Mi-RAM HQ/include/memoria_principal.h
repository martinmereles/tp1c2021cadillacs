#ifndef MEMORIA_PRINCIPAL_H_
#define MEMORIA_PRINCIPAL_H_

#include "segmentacion.h"
#include "paginacion.h"
#include <signal.h>
#include <sys/types.h>
#include <unistd.h>
#include <sys/syscall.h>

// NO DEBE DEPENDER DEL ESQUEMA DE MEMORIA (SEGMENTACION O PAGINACION)
int inicializar_estructuras_memoria(t_config* config_general, t_config* config_test);
void liberar_estructuras_memoria(t_config* config);

// ESQUEMA DE MEMORIA
int inicializar_esquema_memoria(t_config* config_general, t_config* config_test);

// FUNCIONES COMUNES PARA SEGMENTACION Y PAGINACION
void leer_tarea_memoria_principal(void* tabla, char** tarea, uint32_t id_prox_tarea);

// Para manejar signals
void signal_handler(int senial);

#endif