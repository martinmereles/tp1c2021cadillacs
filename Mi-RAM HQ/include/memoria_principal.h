#ifndef MEMORIA_PRINCIPAL_H_
#define MEMORIA_PRINCIPAL_H_

#include "segmentacion.h"

// NO DEBE DEPENDER DEL ESQUEMA DE MEMORIA (SEGMENTACION O PAGINACION)
int inicializar_estructuras_memoria(t_config* config);
void liberar_estructuras_memoria();
void dump_patota(void*);
void dump_tripulante(tabla_segmentos_t* tabla, int nro_fila);

// ESQUEMA DE MEMORIA
int inicializar_esquema_memoria(t_config* config);

#endif