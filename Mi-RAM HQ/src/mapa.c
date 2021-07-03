#include "mapa.h"

#define ASSERT_CREATE(nivel, id, err)                                                   \
    if(err) {                                                                           \
        nivel_destruir(nivel);                                                          \
        nivel_gui_terminar();                                                           \
        fprintf(stderr, "Error al crear '%c': %s\n", id, nivel_gui_string_error(err));  \
        return EXIT_FAILURE;                                                            \
    }

int inicializar_mapa(){
    // Creo el mapa
    nivel_gui_inicializar();

    // Inicializo variables
    nivel_gui_get_area_nivel(&posicion_maxima_X, &posicion_maxima_Y);
    posicion_maxima_X--;
    posicion_maxima_Y--;

    // Creo el nivel
    nivel = nivel_crear("Test Chamber 04");

    // Dibujo el nivel
    nivel_gui_dibujar(nivel);
    
    return EXIT_SUCCESS;
}

int finalizar_mapa(){
    nivel_destruir(nivel);
    nivel_gui_terminar();
    return EXIT_SUCCESS;
}

int dibujar_tripulante_mapa(int TID, int posicion_X, int posicion_Y){

    if( posicion_X < 0 || posicion_maxima_X < posicion_X ){
        log_error(logger, "ERROR. Posicion X: %d. No se puede dibujar en el mapa", posicion_X);
        return EXIT_FAILURE;
    }

    if( posicion_Y < 0 || posicion_maxima_Y < posicion_Y ){
        log_error(logger, "ERROR. Posicion Y: %d. No se puede dibujar en el mapa", posicion_Y);
        return EXIT_FAILURE;
    }
    
    int error = personaje_crear(nivel, caracter_tripulante(TID), posicion_X, posicion_Y);
	ASSERT_CREATE(nivel, caracter_tripulante(TID), error);

    // Redibujo el nivel
    nivel_gui_dibujar(nivel);

    return EXIT_SUCCESS;
}

int borrar_tripulante_mapa(int TID){
    int error = item_borrar(nivel, caracter_tripulante(TID));
    // Redibujo el nivel
    nivel_gui_dibujar(nivel);
    if(error){
        log_error(logger, "WARNING: %s\n", nivel_gui_string_error(error));
        return EXIT_FAILURE;
    }
    return EXIT_SUCCESS; 
}

int desplazar_tripulante_mapa(int TID, int desplazamiento_X, int desplazamiento_Y){
    /*int error;
    switch(desplazamiento){
        case ARRIBA:
            error = item_desplazar(nivel, caracter_tripulante(TID), 0, -1);
            break;
        case ABAJO:
            error = item_desplazar(nivel, caracter_tripulante(TID), 0, 1);
            break;
        case IZQUIERDA:
            error = item_desplazar(nivel, caracter_tripulante(TID), -1, 0);
            break;
        case DERECHA:
            error = item_desplazar(nivel, caracter_tripulante(TID), 1, 0);
            break;
    }*/

    int error = item_desplazar(nivel, caracter_tripulante(TID), desplazamiento_X, desplazamiento_Y);
    // Redibujo el nivel
    nivel_gui_dibujar(nivel);
    
    if(error){
        log_error(logger, "WARNING: %s\n", nivel_gui_string_error(error));
        return EXIT_FAILURE;
    }

    return EXIT_SUCCESS;
}

char caracter_tripulante(int TID){
    return (TID % 50) + '0';
}