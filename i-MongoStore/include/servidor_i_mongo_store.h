#ifndef SERVIDOR_I_MONGO_STORE_H_
#define SERVIDOR_I_MONGO_STORE_H_

#include "i_filesystem.h"
#include "i_mongostore.h"
#include "sockets_shared.h"
#include <commons/string.h>

//estructura tarea
typedef struct {
    char* nombre;
    char* pos_x;
    char* pos_y;
    char* duracion;
}t_tarea;

enum tarea_cod{
    GENERAR_OXIGENO,
    CONSUMIR_OXIGENO,
    GENERAR_COMIDA,
    CONSUMIR_COMIDA,
    GENERAR_BASURA,
    DESCARTAR_BASURA
};


void loguear_mensaje(char* payload);
void recibir_tarea(char* payload);
void conseguir_codigo_tarea(char* nombre, int * codigo);
void crear_archivo (char* path);
char ** bits_libres (t_bitarray * bitarray, int cantidad_bits);
void ocupar_bit(t_bitarray *bitarray, int offset);
void liberar_bit(t_bitarray *bitarray, int offset);


#endif