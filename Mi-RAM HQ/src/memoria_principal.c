#include "memoria_principal.h"

void inicializar_estructuras_memoria(t_config* config){
	// Inicializo las estructuras para administrar la RAM
	tamanio_memoria = atoi(config_get_string_value(config, "TAMANIO_MEMORIA"));
	memoria_principal = malloc(tamanio_memoria);

    // Inicializo las listas de tablas de segmentos
    tablas_de_segmentos_de_patotas = list_create();
    tablas_de_segmentos_de_tripulantes = list_create();

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
}

void liberar_estructuras_memoria(){
    log_info(logger, "Liberando estructuras administrativas de la memoria principal");
    free(memoria_principal);
    // Destruir listas de tablas
    //      tablas_de_segmentos_de_patotas;
    //      tablas_de_segmentos_de_tripulantes;
    free(bitarray_mapa_memoria_disponible);
    vbitarray_destroy(mapa_memoria_disponible);
}

fila_tabla_segmentos_t* reservar_segmento(int tamanio){
    int inicio = first_fit(tamanio);
    fila_tabla_segmentos_t* fila = NULL;
    if(inicio >= 0){
        fila = malloc(sizeof(fila_tabla_segmentos_t));
        fila->inicio = inicio;
        fila->tamanio = tamanio;
        for(int pos = inicio;pos < inicio + tamanio;pos++){
            void bitarray_set_bit(mapa_memoria_disponible, pos);
        }
    }
    return fila;
}

void liberar_segmento(fila_tabla_segmentos_t* fila){
    int inicio = fila->inicio;
    int tamanio = fila->tamanio;
    for(int pos = inicio;pos < inicio + tamanio;pos++){
        void bitarray_clean_bit(mapa_memoria_disponible, pos);
    }
    free(fila);
}

int first_fit(int memoria_pedida) {
    int memoria_disponible = 0;
    int posicion_memoria_disponible = 0;
    
    // Me fijo byte por byte de la memoria principal cuales estan disponibles
    for(int nro_byte = 0;nro_byte < tamanio_memoria && memoria_disponible < memoria_pedida;nro_byte++){
        // Si el byte esta ocupado
        if(bitarray_test_bit(mapa_memoria_disponible, nro_byte)){
            memoria_disponible++;
        }
        // Si el byte esta disponible
        else{
            memoria_disponible = 0;
            posicion_memoria_disponible = nro_byte + 1;
        }
	}

    // Si no se encontro un espacio de memoria del tamanio pedido, retorna -1
    if(memoria_disponible != memoria_pedida) {
        return -1;
    }   

    return posicion_memoria_disponible;
}

int memcpy_tabla_segmentos(tabla_segmentos_t* tabla, uint32_t direccion_logica, void* dato, int tamanio){
    int numero_fila = (direccion_logica & 0x80000000) >> 31;
    int direccion_fisica_dato = calcular_direccion_fisica(tabla, direccion_logica);
    if(tabla->filas[numero_fila]->inicio + tabla->filas[numero_fila]->tamanio < direccion_fisica_dato + tamanio){
        log_error(logger,"ERROR. Segmentation fault. Esta intentando escribir en una posicion de memoria no reservada");
        return EXIT_FAILURE;
    }
    memcpy(memoria_principal+direccion_fisica_dato, dato, tamanio);
    return EXIT_SUCCESS;
}