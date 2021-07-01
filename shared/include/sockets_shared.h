#ifndef SOCKETS_SHARED_H_
#define SOCKETS_SHARED_H_

#include <stdbool.h>
#include <signal.h>
#include <unistd.h>
#include <sys/socket.h>
#include <netdb.h>
#include <string.h>
#include "variables_globales_shared.h"
#include <commons/collections/list.h>

// PROTOCOLO: CODIGOS DE OPERACION 
enum op_code{
	COD_MENSAJE,

    // Codigos para servidor Submodulo tripulante
    COD_PROXIMA_TAREA,
    COD_ESTADO_TRIPULANTE,
    COD_UBICACION_TRIPULANTE,

    // Codigos para servidor Mi-RAM HQ
    COD_INICIAR_PATOTA,
    COD_INICIAR_TRIPULANTE,
    
    COD_RECIBIR_UBICACION_TRIPULANTE,
    COD_ENVIAR_UBICACION_TRIPULANTE,
    COD_RECIBIR_ESTADO_TRIPULANTE,
    COD_ENVIAR_ESTADO_TRIPULANTE,

    COD_ENVIAR_PROXIMA_TAREA,
    COD_EXPULSAR_TRIPULANTE,

    // Codigos de respuesta del servidor Mi-RAM HQ
    COD_INICIAR_PATOTA_ERROR,
    COD_INICIAR_PATOTA_OK,
    COD_INICIAR_TRIPULANTE_ERROR,
    COD_INICIAR_TRIPULANTE_OK,

    // Codigos para servidor i-Mongo-Store: FALTA
    COD_EJECUTAR_TAREA,
    COD_OBTENER_BITACORA
};

int recibir_operacion(int servidor_fd);
char* recibir_payload(int socket_emisor);
char* recibir_mensaje(int socket_cliente);
int enviar_fin_comunicacion(int socket_destino);
int enviar_mensaje(int socket_destino, char* mensaje);
int enviar_operacion(int fd_destino, int codigo_operacion, void* payload, int longitud_payload);
int iniciar_servidor(char*, char*);
int recibir_payload_y_ejecutar(int socket_fd, int(*funcion_a_ejecutar)(char*));

#endif