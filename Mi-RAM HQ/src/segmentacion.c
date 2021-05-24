#include "segmentacion.h"

// ALGORITMOS DE UBICACION

int inicializar_algoritmo_de_ubicacion(t_config* config){
    char* string_algoritmo_ubicacion = config_get_string_value(config, "ALGORITMO_UBICACION");
    if(strcmp(string_algoritmo_ubicacion,"FIRST_FIT")==0){
        log_info(logger,"El algoritmo de ubicacion es: First Fit");
        algoritmo_de_ubicacion = &first_fit;
        return EXIT_SUCCESS;
    }
    if(strcmp(string_algoritmo_ubicacion,"BEST_FIT")==0){
        log_info(logger,"El algoritmo de ubicacion es: Best Fit");
        algoritmo_de_ubicacion = &best_fit;
        return EXIT_SUCCESS;
    }
    log_error(logger,"%s: Algoritmo de ubicacion invalido",string_algoritmo_ubicacion);
    return EXIT_FAILURE;
}

int first_fit(int memoria_pedida) {
    int memoria_disponible = 0;
    int posicion_memoria_disponible = 0;
    // Me fijo byte por byte de la memoria principal cuales estan disponibles
    for(int nro_byte = 0;nro_byte < tamanio_memoria && memoria_disponible < memoria_pedida;nro_byte++){
        // Si el byte esta ocupado
        if(bitarray_test_bit(mapa_memoria_disponible, nro_byte)){
            memoria_disponible = 0;
            posicion_memoria_disponible = nro_byte + 1;
        }
        // Si el byte esta disponible
        else{
            memoria_disponible++;
        }
	}
    // Si no se encontro un espacio de memoria del tamanio pedido, retorna -1
    if(memoria_disponible != memoria_pedida)
        return -1;
    return posicion_memoria_disponible;
}

int best_fit(int memoria_pedida) {
    int memoria_disponible = 0;
    int posicion_memoria_disponible = 0;
    int mejor_memoria_disponible = 0;
    int mejor_posicion_memoria_disponible = -1;
    log_info(logger,"ARRANCA EL BEST FIT");
    // Me fijo byte por byte de la memoria principal cuales estan disponibles
    for(int nro_byte = 0;nro_byte < tamanio_memoria;nro_byte++){
        // Si el byte esta ocupado
        if(bitarray_test_bit(mapa_memoria_disponible, nro_byte)){

            // Me fijo si la memoria pedida entra en la memoria hallada
            if(memoria_disponible >= memoria_pedida){
                // Si entra, la comparo con la mejor que habia encontrado
                if(mejor_memoria_disponible > memoria_disponible || mejor_posicion_memoria_disponible == -1){
                    // Si es mejor, se convierte en la nueva mejor
                    mejor_memoria_disponible = memoria_disponible;
                    mejor_posicion_memoria_disponible = posicion_memoria_disponible;
                }
            }

            memoria_disponible = 0;
            posicion_memoria_disponible = nro_byte + 1;
        }
        // Si el byte esta disponible
        else{
            memoria_disponible++;
        }
    }
    // Verifico si hay memoria disponible al final de la memoria principal
    // Me fijo si la memoria pedida entra en la memoria hallada
    if(memoria_disponible >= memoria_pedida){
        // Si entra, la comparo con la mejor que habia encontrado
        if(mejor_memoria_disponible > memoria_disponible || mejor_posicion_memoria_disponible == -1){
            // Si es mejor, se convierte en la nueva mejor
            mejor_memoria_disponible = memoria_disponible;
            mejor_posicion_memoria_disponible = posicion_memoria_disponible;
        }
    }
    log_info(logger,"El mejor ajuste es de tamanio %d", mejor_memoria_disponible);
    log_info(logger,"La direccion del mejor ajuste es %d", mejor_posicion_memoria_disponible);
    // Retorno la mejor posicion de memoria encontrada
    // Si no se encontro un espacio de memoria del tamanio pedido, retorna -1
    return mejor_posicion_memoria_disponible;
}

// LECTURA/ESCRITURA EN MEMORIA PRINCIPAL (TIENE QUE SER POLIMORFICO?)

int escribir_memoria_principal(tabla_segmentos_t* tabla, uint32_t direccion_logica, void* dato, int tamanio){
    //log_info(logger,"Escribiendo en memoria principal");
    int numero_seg = numero_de_segmento(direccion_logica);
    int direccion_fisica_dato = calcular_direccion_fisica(tabla, direccion_logica);
    fila_tabla_segmentos_t* fila = obtener_fila(tabla, numero_seg);
    if(fila->inicio + fila->tamanio < direccion_fisica_dato + tamanio){
        log_error(logger,"ERROR. Segmentation fault. Direccionamiento invalido en escritura");
        return EXIT_FAILURE;
    }
    memcpy(memoria_principal+direccion_fisica_dato, dato, tamanio);
    return EXIT_SUCCESS;
}

int leer_memoria_principal(tabla_segmentos_t* tabla, uint32_t direccion_logica, void* dato, int tamanio){
    int numero_seg = numero_de_segmento(direccion_logica);
    int direccion_fisica_dato = calcular_direccion_fisica(tabla, direccion_logica);
    fila_tabla_segmentos_t* fila = obtener_fila(tabla,numero_seg);
    if(fila->inicio + fila->tamanio < direccion_fisica_dato + tamanio){
        log_error(logger,"ERROR. Segmentation fault. Direccionamiento invalido en lectura");
        return EXIT_FAILURE;
    }
    memcpy(dato, memoria_principal+direccion_fisica_dato, tamanio);
    return EXIT_SUCCESS;
}

void leer_tarea_memoria_principal(tabla_segmentos_t* tabla, uint32_t dir_log_tareas, char** tarea, int id_prox_tarea){
    fila_tabla_segmentos_t* fila = obtener_fila(tabla, numero_de_segmento(dir_log_tareas));
    int tamanio = fila->tamanio;
    char* tareas = malloc(tamanio);
    leer_memoria_principal(tabla, dir_log_tareas, tareas, tamanio);
    char** array_tareas = (char**) string_split(tareas, "\n");
    int cant_tareas = cantidad_tareas(array_tareas); 
    if(id_prox_tarea < cant_tareas)
        *tarea = string_duplicate(array_tareas[id_prox_tarea]);
    else{
        if(id_prox_tarea == cant_tareas)
            *tarea = string_duplicate("FIN");
        else{
            *tarea = NULL;
            log_error(logger,"ERROR. No existe la instruccion con el identificador solicitado");
        }
    }
    // Libero el string tareas
    free(tareas);
	// Libero el array de tareas
	for(int i = 0;array_tareas[i]!=NULL;i++){
		free(array_tareas[i]);
	}
	free(array_tareas);
}

int cantidad_tareas(char** array_tareas){
	int cantidad = 0;
	for(;array_tareas[cantidad]!=NULL;cantidad++);
	return cantidad;
}

// TABLAS DE SEGMENTOS

void quitar_y_destruir_tabla(tabla_segmentos_t* tabla_a_destruir){
    int PID_tabla_a_destruir;
    leer_memoria_principal(tabla_a_destruir, DIR_LOG_PCB + DESPL_PID, &PID_tabla_a_destruir, sizeof(uint32_t));

    bool tienePID(void* args){
        tabla_segmentos_t* una_tabla = (tabla_segmentos_t*) args;
        int PID_una_tabla;
        leer_memoria_principal(una_tabla, DIR_LOG_PCB + DESPL_PID, &PID_una_tabla, sizeof(uint32_t));
        return PID_una_tabla == PID_tabla_a_destruir;
    }

    list_remove_and_destroy_by_condition(tablas_de_segmentos, tienePID, destruir_tabla_segmentos);    
}

void destruir_tabla_segmentos(void* args){
    tabla_segmentos_t* tabla = (tabla_segmentos_t*) args;
    // Destruir todas las filas y liberar segmentos;
    list_destroy_and_destroy_elements(tabla->filas, destruir_fila);
    free(tabla);
}

void quitar_y_destruir_fila(tabla_segmentos_t* tabla, int numero_seg){
    bool tiene_numero_de_segmento(void* args){
        fila_tabla_segmentos_t* fila = (fila_tabla_segmentos_t*) args;
        return fila->numero_segmento == numero_seg; 
    }
    list_remove_and_destroy_by_condition(tabla->filas, tiene_numero_de_segmento, destruir_fila);
}
 

tabla_segmentos_t* obtener_tabla_patota(int PID_buscado){
    bool tienePID(void* args){
        tabla_segmentos_t* tabla = (tabla_segmentos_t*) args;
        int PID_tabla;
        leer_memoria_principal(tabla, DIR_LOG_PCB + DESPL_PID, &PID_tabla, sizeof(uint32_t));
        return PID_tabla == PID_buscado;
    }
    return list_find(tablas_de_segmentos, tienePID);
}

fila_tabla_segmentos_t* obtener_fila(tabla_segmentos_t* tabla, int numero_seg){
    bool tiene_numero_de_segmento(void* args){
        fila_tabla_segmentos_t* fila = (fila_tabla_segmentos_t*) args;
        return fila->numero_segmento == numero_seg; 
    }
    /*
    bool tiene_numero_de_segmento(fila_tabla_segmentos_t* fila){
        return fila->numero_segmento == numero_seg; 
    }*/
    return list_find(tabla->filas, tiene_numero_de_segmento);
}

tabla_segmentos_t* crear_tabla_segmentos(){
    tabla_segmentos_t* tabla = malloc(sizeof(tabla_segmentos_t));
    tabla->filas = list_create();
    tabla->proximo_numero_segmento = 0;
    return tabla;
}

int cantidad_filas(tabla_segmentos_t* tabla){
    return list_size(tabla->filas);
}

void destruir_fila(void* args){
    fila_tabla_segmentos_t* fila = (fila_tabla_segmentos_t*) args;
    liberar_segmento(fila);
    free(fila);
}

fila_tabla_segmentos_t* crear_fila(tabla_segmentos_t* tabla, int tamanio){
    fila_tabla_segmentos_t* fila = reservar_segmento(tamanio);
    agregar_fila(tabla,fila);
    return fila;
}

void agregar_fila(tabla_segmentos_t* tabla, fila_tabla_segmentos_t* fila){
    list_add(tabla->filas,fila);
    fila->numero_segmento = generar_nuevo_numero_segmento(tabla);
}

int generar_nuevo_numero_segmento(tabla_segmentos_t* tabla){
    // AGREGAR SEMAFORO
    int nuevo = tabla->proximo_numero_segmento;
    tabla->proximo_numero_segmento++;
    return nuevo;
}

// SEGMENTOS

fila_tabla_segmentos_t* reservar_segmento(int tamanio){
    int inicio = algoritmo_de_ubicacion(tamanio);
    fila_tabla_segmentos_t* fila = NULL;
    if(inicio >= 0){
        fila = malloc(sizeof(fila_tabla_segmentos_t));
        fila->inicio = inicio;
        fila->tamanio = tamanio;
        for(int pos = inicio;pos < inicio + tamanio;pos++){
            bitarray_set_bit(mapa_memoria_disponible, pos);
        }
    }
    return fila;
}

void liberar_segmento(fila_tabla_segmentos_t* fila){
    int inicio = fila->inicio;
    int tamanio = fila->tamanio;
    for(int pos = inicio;pos < inicio + tamanio;pos++){
        bitarray_clean_bit(mapa_memoria_disponible, pos);
    }
}

// DIRECCIONAMIENTO

uint32_t direccion_logica(fila_tabla_segmentos_t* fila){
    return fila->numero_segmento << 16;
}

int calcular_direccion_fisica(tabla_segmentos_t* tabla, uint32_t direccion_logica){
    fila_tabla_segmentos_t* fila = obtener_fila(tabla, numero_de_segmento(direccion_logica));
    if(fila == NULL){
        log_error(logger,"NO EXISTE LA FILA");
    }
    return fila->inicio + desplazamiento(direccion_logica);
}

uint32_t numero_de_segmento(uint32_t direccion_logica){
    return (direccion_logica & 0xFFFF0000) >> 16;
}

uint32_t desplazamiento(uint32_t direccion_logica){
    return direccion_logica & 0x0000FFFF;
}