#ifndef MAPA_H_
#define MAPA_H_

#include <nivel-gui/nivel-gui.h>
#include <nivel-gui/tad_nivel.h>

#include <stdlib.h>
#include <curses.h>
#include <commons/collections/list.h>

#include "variables_globales_shared.h"

enum desplazamiento_mapa{  ARRIBA, ABAJO, IZQUIERDA, DERECHA  };

// FUNCIONES
int inicializar_mapa();
int finalizar_mapa();
int dibujar_tripulante_mapa(int TID, int posicion_X, int posicion_Y);
int borrar_tripulante_mapa(int TID);
int desplazar_tripulante_mapa(int TID, enum desplazamiento_mapa desplazamiento);
char caracter_tripulante(int TID);

// VARIABLES GLOBALES
NIVEL* nivel;
int posicion_maxima_X;
int posicion_maxima_Y;

#endif