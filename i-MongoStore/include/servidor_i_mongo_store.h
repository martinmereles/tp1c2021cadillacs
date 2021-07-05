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

typedef struct {
    char* caracter_llenado;
    char* blocks;
    int block_count;
    int size;
}t_config_tarea;

typedef struct {
    char* blocks;
    int block_count;
    int size;
}t_config_bitacora;

int loguear_mensaje(char* payload);
int recibir_tarea(char* payload);
void conseguir_codigo_tarea(char* nombre, int * codigo);
void crear_archivo (char* path);
char ** bits_libres (t_bitarray * bitarray, int cantidad_bits);
void ocupar_bit(t_bitarray *bitarray, int offset);
void liberar_bit(t_bitarray *bitarray, int offset);
void leer_tarea_config(t_config * tarea_config_file);
void eliminar_keys_tarea(t_config * tarea_config_file);
void set_tarea_config(t_config * tarea_config_file,char * size, char * cantidad_bloques, char* bloques, char * md5);
char * array_two_block_to_string(char ** bloq_antiguo, char** bloq_nuevo, int config_block_count);
char * array_block_to_string(char ** bloq_array,int indice);
void generar_recurso(char* path, char caracter_llenado,char* parametro);
void consumir_recurso(char* path, char* nombre_recurso ,char* parametro);
void escribir_bitacora(char* payload);
char* leer_bitacora(char* payload);
void iniciar_tripulante(char* payload);
void leer_bitacora_config(t_config * bitacora_config_file);
void eliminar_keys_bitacora(t_config * bitacora_config_file);
int terminar_tarea(char * payload);
int movimiento_tripulante(char * payload);



#endif