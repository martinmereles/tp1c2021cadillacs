#include "memoria_principal.h"

int inicializar_estructuras_memoria(t_config* config){
	// Inicializo las estructuras para administrar la RAM
	tamanio_memoria = atoi(config_get_string_value(config, "TAMANIO_MEMORIA"));
	memoria_principal = malloc(tamanio_memoria);

    // Inicializar esquema de memoria
    if(inicializar_esquema_memoria(config) == EXIT_FAILURE)
        return EXIT_FAILURE;
    
    // Inicializo la lista de tablas de segmentos
    tablas_de_patotas = list_create();

	// Cada byte del mapa representa 8 bytes en RAM
	int tamanio_mapa_memoria_disponible = tamanio_memoria/8;
	// Le sumo 1 byte en el caso que el tamanio en bytes de la RAM no sea multiplo de 8
	if(tamanio_memoria%8 != 0)
		tamanio_mapa_memoria_disponible++;
	bitarray_mapa_memoria_disponible = malloc(tamanio_mapa_memoria_disponible);
	mapa_memoria_disponible = bitarray_create_with_mode(bitarray_mapa_memoria_disponible,
														tamanio_mapa_memoria_disponible,
														LSB_FIRST);
	// Inicializo todos los bits del mapa en 0
	for(int i = 0;i < tamanio_memoria;i++){
		bitarray_clean_bit(mapa_memoria_disponible, i);
	}

	log_info(logger, "El tamanio de la RAM es: %d",tamanio_memoria);
	log_info(logger, "El tamanio del mapa de memoria disponible es: %d",bitarray_get_max_bit(mapa_memoria_disponible));
    return EXIT_SUCCESS;
}

int inicializar_esquema_memoria(t_config* config){
    char* string_algoritmo_ubicacion = config_get_string_value(config, "ESQUEMA_MEMORIA");
    if(strcmp(string_algoritmo_ubicacion,"SEGMENTACION")==0){
        log_info(logger,"El esquema de memoria es: SEGMENTACION");

        // Inicializo ALGORITMO UBICACION
        if(inicializar_algoritmo_de_ubicacion(config) == EXIT_FAILURE)
            return EXIT_FAILURE;

        // Inicializo vectores a funciones
        dump_patota = &dump_patota_segmentacion;

        return EXIT_SUCCESS;
    }
    if(strcmp(string_algoritmo_ubicacion,"PAGINACION")==0){
        log_info(logger,"El esquema de memoria es: PAGINACION");

        // Inicializo TAMANIO PAGINA
        // Inicializo TAMANIO SWAP
        // Inicializo PATH SWAP
        // Inicializo ALGORITMO REEMPLAZO

        // Inicializo vectores a funciones
        // dump_patota = &dump_patota_paginacion;

        return EXIT_SUCCESS;
    }
    log_error(logger,"%s: Esquema de memoria invalido",string_algoritmo_ubicacion);
    return EXIT_FAILURE;
}
   
void liberar_estructuras_memoria(){
    log_info(logger, "Liberando estructuras administrativas de la memoria principal");
    list_destroy_and_destroy_elements(tablas_de_patotas, destruir_tabla_segmentos);
    free(bitarray_mapa_memoria_disponible);
    bitarray_destroy(mapa_memoria_disponible);
    free(memoria_principal);
}