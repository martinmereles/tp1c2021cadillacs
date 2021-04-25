#include "sockets_shared.h"

int recibir_operacion(int servidor_fd)
{
	int cod_op;
	if(recv(servidor_fd, &cod_op, sizeof(int), MSG_WAITALL) != 0)
		return cod_op;
	else
		return -1;
}

void* recibir_payload(int socket_emisor)
{
	void * payload;
	int longitud_payload;

	recv(socket_emisor, &longitud_payload, sizeof(int), MSG_WAITALL);
	payload = malloc(longitud_payload);
	recv(socket_emisor, payload, longitud_payload, MSG_WAITALL);

	return payload;
}

char* recibir_mensaje(int socket_cliente)
{
	char* mensaje = recibir_payload(socket_cliente);
	return mensaje;
}

int enviar_fin_comunicacion(int fd_destino){
	return enviar_operacion(fd_destino, FIN_COMUNICACION, NULL);
}

int enviar_mensaje(int fd_destino, char* mensaje){	
	return enviar_operacion(fd_destino, MENSAJE, mensaje);
}

int enviar_operacion(int fd_destino, int codigo_operacion, char* payload) {
	int bytes_enviados;
	int exit_status = EXIT_SUCCESS;
	char* stream;
	char offset = 0;
	int cantidad_bytes;
	int tamanio_stream;

	// Si no hay payload, solo se envia el codigo de operacion
	if(payload == NULL) {
		tamanio_stream = sizeof(codigo_operacion);

		stream = malloc(tamanio_stream);

		memcpy(stream + offset, &codigo_operacion, sizeof(int));
		offset += sizeof(int);
	}

	// Si hay payload, enviamos codigo de operacion + longitud + payload
	else {
		cantidad_bytes = strlen(payload) + 1;
		tamanio_stream = sizeof(codigo_operacion) + sizeof(cantidad_bytes) + cantidad_bytes;
	
		stream = malloc(tamanio_stream);
	
		memcpy(stream + offset, &codigo_operacion, sizeof(int));
		offset += sizeof(int);
		
		memcpy(stream + offset, &cantidad_bytes, sizeof(cantidad_bytes));
		offset += sizeof(cantidad_bytes);

		memcpy(stream + offset, payload, cantidad_bytes);
		offset += sizeof(char) * cantidad_bytes;
	}

	bytes_enviados = send(fd_destino, stream, tamanio_stream, 0);

	if(bytes_enviados == -1){
		printf("ERROR. NO SE ENVIO LA OPERACION\n");
		exit_status = EXIT_FAILURE;
	}
	if(bytes_enviados < tamanio_stream){
		printf("ERROR. NO SE ENVIARON TODOS LOS BYTES\n");
		exit_status = EXIT_FAILURE;
	}

	free(stream);
	return exit_status;
}