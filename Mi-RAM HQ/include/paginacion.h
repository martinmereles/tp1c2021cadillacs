#ifndef PAGINACION_H_
#define PAGINACION_H_

#include "mi_ram_hq_variables_globales.h"

/*
direccion logica = nro segmento | desplazamiento
nro segmento: 16 bits
desplazamiento: 16 bits
*/

typedef struct{
    uint32_t numero_marco;
    bool bit_de_presencia;
    bool bit_de_modificado;
} fila_tabla_paginas_t;

typedef struct{
    t_list *filas;
    uint32_t proximo_numero_pagina; // Hace falta??
} tabla_paginas_t;

// Patotas y Tripulantes
int crear_patota_paginacion(uint32_t PID, uint32_t longitud_tareas, char* tareas);
int crear_tripulante_paginacion(void**, uint32_t*, uint32_t, uint32_t, uint32_t, uint32_t);

// Dump
void dump_memoria_paginacion();
void dump_patota_paginacion(tabla_paginas_t* tabla_patota, FILE* archivo_dump);
void dump_pagina(FILE* archivo_dump, tabla_paginas_t* tabla, int PID, int nro_segmento);

#endif