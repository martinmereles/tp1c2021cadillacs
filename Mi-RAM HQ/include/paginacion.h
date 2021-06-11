#ifndef PAGINACION_H_
#define PAGINACION_H_

#include "mi_ram_hq_variables_globales.h"

/*
direccion logica = nro pagina | desplazamiento
nro pagina: 16 bits
desplazamiento: 16 bits
*/

enum estado_marco{
    MARCO_LIBRE = 0,
    MARCO_OCUPADO = 1
};

typedef struct{
    int numero_pagina;
    int numero_marco;
    bool bit_presencia;
    bool bit_modificado;
    int fragmentacion_interna;
    enum estado_marco estado;
    int PID;
} marco_t;

typedef struct{
    t_list *paginas;
    uint32_t proximo_numero_pagina;
    int fragmentacion_interna;
    int cantidad_tripulantes;
    int PID;
    uint32_t tamanio_tareas;
} tabla_paginas_t;

// Patotas y Tripulantes
int crear_patota_paginacion(uint32_t PID, uint32_t longitud_tareas, char* tareas);
int crear_tripulante_paginacion(tabla_paginas_t**, uint32_t*, uint32_t, uint32_t, uint32_t, uint32_t);

// Escritura/Lectura
int escribir_memoria_principal_paginacion(void* args, uint32_t inicio_logico, uint32_t desplazamiento_logico, void* dato, int tamanio_total);
int leer_memoria_principal_paginacion(void* args, uint32_t inicio_logico, uint32_t desplazamiento_logico, void* dato, int tamanio);

// Direccionamiento
uint32_t direccion_logica_paginacion(uint32_t inicio_logico, uint32_t desplazamiento_logico);
int direccion_fisica_paginacion(tabla_paginas_t* tabla_patota, uint32_t direccion_logica, int* direccion_fisica);
int numero_pagina(uint32_t direccion_logica);
int desplazamiento_paginacion(uint32_t direccion_logica);

// Paginas
tabla_paginas_t* obtener_tabla_patota_paginacion(int PID_buscado);
int reservar_memoria(tabla_paginas_t* tabla_patota, int tamanio, uint32_t* direccion_logica);
int crear_pagina(tabla_paginas_t* tabla_patota);
marco_t* ultima_pagina(tabla_paginas_t* tabla_patota);
marco_t* get_pagina(tabla_paginas_t* tabla_patota, int numero_pagina_buscada);
int cantidad_paginas(tabla_paginas_t* tabla_patota);
void destruir_tabla_paginas(tabla_paginas_t* tabla_patota);
void liberar_marco(void*);


// Dump
void dump_memoria_paginacion();
void dump_marco(marco_t* marco, FILE* archivo_dump);

// VARIABLES GLOBALES
int tamanio_pagina;
int cantidad_marcos;
t_list* lista_de_marcos;

#endif