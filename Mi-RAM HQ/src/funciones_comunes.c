#include "mi_ram_hq_variables_globales.h"

int cantidad_tareas(char** array_tareas){
	int cantidad = 0;
	for(;array_tareas[cantidad]!=NULL;cantidad++);
	return cantidad;
}

uint32_t get_desplazamiento(uint32_t direccion_logica){
    return direccion_logica & 0x0000FFFF;
}

void crear_archivo_dump(t_list* lista_dump, void (*funcion_dump)(void*,FILE*)){
    log_info(logger,"INICIANDO DUMP");

    // Creamos el archivo con fecha
    char* fecha = temporal_get_string_time("%d_%m_%y_%H_%M_%S");
    char* nombre_archivo = string_from_format("./dump/Dump_%s.dmp", fecha);
    FILE* archivo_dump = fopen(nombre_archivo, "w");
    free(nombre_archivo);
    free(fecha);

    // Escribimos info general del dump
    fecha = temporal_get_string_time("%d/%m/%y %H:%M:%S");
    char* info_dump = string_from_format("Dump: %s", fecha);
    fwrite(info_dump, sizeof(char), strlen(info_dump), archivo_dump);
    free(info_dump);
    free(fecha);

    // Por cada elemento de la lista, realizamos el dump correspondiente
    t_list_iterator* iterador = list_iterator_create(lista_dump);    // Creamos el iterador
    void* elemento;

    while(list_iterator_has_next(iterador)){
        elemento = list_iterator_next(iterador);
        funcion_dump(elemento, archivo_dump);
    }

    list_iterator_destroy(iterador);    // Liberamos el iterador

    // Cerramos el archivo
    fclose(archivo_dump);
}