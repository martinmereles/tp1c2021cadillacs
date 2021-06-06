#include "memoria_principal.h"

void inicializar_estructuras_memoria(t_config* config){
	// Inicializo las estructuras para administrar la RAM
	tamanio_memoria = atoi(config_get_string_value(config, "TAMANIO_MEMORIA"));
	memoria_principal = malloc(tamanio_memoria);

    // Inicializo la lista de tablas de segmentos
    tablas_de_segmentos = list_create();

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