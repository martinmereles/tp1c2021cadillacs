#include "memoria_principal.h"

int inicializar_estructuras_memoria(t_config* config){
	// Inicializo las estructuras para administrar la RAM
	tamanio_memoria = atoi(config_get_string_value(config, "TAMANIO_MEMORIA"));
	memoria_principal = malloc(tamanio_memoria);

    log_info(logger, "El tamanio de la RAM es: %d",tamanio_memoria);

    // Inicializar esquema de memoria
    if(inicializar_esquema_memoria(config) == EXIT_FAILURE)
        return EXIT_FAILURE;
    
    // Inicializo la lista de tablas de segmentos
    tablas_de_patotas = list_create();

    return EXIT_SUCCESS;
}

int inicializar_esquema_memoria(t_config* config){
    char* string_algoritmo_ubicacion = config_get_string_value(config, "ESQUEMA_MEMORIA");
    if(strcmp(string_algoritmo_ubicacion,"SEGMENTACION")==0){
        log_info(logger,"El esquema de memoria es: SEGMENTACION");

        // Cada byte del mapa representa 8 bytes en RAM
        int tamanio_mapa_memoria_disponible = tamanio_memoria/8;
        // Le sumo 1 byte en el caso que el tamanio en bytes de la RAM no sea multiplo de 8
        if(tamanio_memoria%8 != 0)
            tamanio_mapa_memoria_disponible++;
        bitarray_mapa_memoria_disponible = malloc(tamanio_mapa_memoria_disponible);
        mapa_memoria_disponible = bitarray_create_with_mode(bitarray_mapa_memoria_disponible,
                                                            tamanio_mapa_memoria_disponible,
                                                            LSB_FIRST);

        // Inicializo todos los bits del mapa de memoria disponible en 0
        for(int i = 0;i < tamanio_memoria;i++){
            bitarray_clean_bit(mapa_memoria_disponible, i);
        }

        // Inicializo ALGORITMO UBICACION
        if(inicializar_algoritmo_de_ubicacion(config) == EXIT_FAILURE)
            return EXIT_FAILURE;

        // Inicializo vectores a funciones
        dump_memoria = &dump_memoria_segmentacion;
        crear_patota = &crear_patota_segmentacion;
        crear_tripulante = &crear_tripulante_segmentacion;
        escribir_memoria_principal = &escribir_memoria_principal_segmentacion;
        leer_memoria_principal = &leer_memoria_principal_segmentacion;
        obtener_tabla_patota = &obtener_tabla_patota_segmentacion;

        log_info(logger, "El tamanio del mapa de memoria disponible es: %d",bitarray_get_max_bit(mapa_memoria_disponible));

        return EXIT_SUCCESS;
    }

    if(strcmp(string_algoritmo_ubicacion,"PAGINACION")==0){
        log_info(logger,"El esquema de memoria es: PAGINACION");

        // Inicializo TAMANIO PAGINA
	    tamanio_pagina = atoi(config_get_string_value(config, "TAMANIO_PAGINA"));

        log_info(logger,"El tamanio de pagina es: %d", tamanio_pagina);

        if(tamanio_memoria % tamanio_pagina != 0){
            log_error(logger,"ERROR. El tamanio de la memoria no es multiplo del tamanio de pagina");
            return EXIT_FAILURE;
        }

        cantidad_marcos = tamanio_memoria/tamanio_pagina;

        log_info(logger,"La cantidad de marcos es: %d", cantidad_marcos);

        // Creamos la lista global de marcos
        lista_de_marcos = list_create();
        marco_t* marco;

        // Inicializo todos los marcos
        for(int i = 0;i < cantidad_marcos;i++){
            marco = malloc(sizeof(marco_t));
            marco->numero_marco = i;
            marco->estado = 0;
            list_add(lista_de_marcos, marco); 
        }

        // Inicializo TAMANIO SWAP
        // Inicializo PATH SWAP
        // Inicializo ALGORITMO REEMPLAZO

        // Inicializo vectores a funciones
        dump_memoria = &dump_memoria_paginacion;
        crear_patota = &crear_patota_paginacion;
        crear_tripulante = &crear_tripulante_paginacion;
        escribir_memoria_principal = &escribir_memoria_principal_paginacion;
        leer_memoria_principal = &leer_memoria_principal_paginacion;
        obtener_tabla_patota = &obtener_tabla_patota_paginacion;

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