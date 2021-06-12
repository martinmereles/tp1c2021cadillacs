#include "mi_ram_hq_variables_globales.h"

int cantidad_tareas(char** array_tareas){
	int cantidad = 0;
	for(;array_tareas[cantidad]!=NULL;cantidad++);
	return cantidad;
}

uint32_t desplazamiento(uint32_t direccion_logica){
    return direccion_logica & 0x0000FFFF;
}