#include "sockets_cliente.h"

int crear_conexion(char *ip, char* puerto, int *servidor_fd)
{
	int exit_status = EXIT_SUCCESS;
	// Estructura Estatica Auxiliar addrinfo
	// Se utiliza para inicializar server_info
	struct addrinfo hints;
	memset(&hints, 0, sizeof(hints));
	hints.ai_family = AF_UNSPEC;		// Familia de protocolos
	hints.ai_socktype = SOCK_STREAM;	// Tipo de socket
	hints.ai_flags = AI_PASSIVE;

	// Estructura Dinamica Final addrinfo
	// Este objeto representa la direccion del host hacia el cual queremos conectarnos (server)
	// Se usa para configurar el socket y la conexion
	struct addrinfo *server_info;

	// Inicializamos la estructura server_info
	// Argumentos:	IP hacia el cual queremos conectarnos (en notacion dotted-quad)
	//				Puerto hacia el cual queremos conectarnos
	//				Estructura auxiliar (hints)
	//				Estructura que queremos inicializar (server_info)
	getaddrinfo(ip, puerto, &hints, &server_info);

	// Creamos el socket para comunicarnos con el server
	// Argumentos:	Familia de protocolos a usar
	// 				Tipo de socket (Puede ser tipo stream o tipo datagram, pero siempre usamos stream)
	//				Protocolo a usar
	// Usamos server_info porque ambos hosts deben usar los mismos protocolos y tipo de socket
	*servidor_fd = socket(server_info->ai_family, server_info->ai_socktype, server_info->ai_protocol);

	// Una vez creado el socket, creamos la conexion
	// Argumentos: 	socket que creamos para comunicarnos con el server,
	//				direccion del host hacia el cual queremos conectarnos (el server)
	//				longitud de la direccion del host hacia el cual queremos conectarnos (el servidor)
	if(connect(*servidor_fd, server_info->ai_addr, server_info->ai_addrlen) == -1){
		exit_status = EXIT_FAILURE;
	}
		
	// Liberamos la estructura server_info
	freeaddrinfo(server_info);

	return exit_status;
}

void liberar_conexion(int servidor_fd)
{
	// Liberamos los recursos asociados al socket y lo cerramos
	close(servidor_fd);
}

int enviar_op_iniciar_patota(int mi_ram_hq_fd, int PID, char* lista_de_tareas){
	int exit_status;
	int longitud_lista_tareas = strlen(lista_de_tareas) + 1;
	int longitud_stream = sizeof(PID) + sizeof(longitud_lista_tareas) + longitud_lista_tareas;
	void* stream = malloc(longitud_stream);
	int offset = 0;

	memcpy(stream + offset, &PID, sizeof(int));
	offset += sizeof(int);

	memcpy(stream + offset, &longitud_lista_tareas, sizeof(int));
	offset += sizeof(int);

	memcpy(stream + offset, lista_de_tareas, longitud_lista_tareas);
	offset += longitud_lista_tareas;

	exit_status = enviar_operacion(mi_ram_hq_fd, COD_INICIAR_PATOTA, stream, longitud_stream);
	free(stream);
	
	return exit_status;
}

int enviar_op_iniciar_tripulante(int mi_ram_hq_fd, iniciar_tripulante_t struct_iniciar_tripulante){
	int exit_status;
	int longitud_stream = sizeof(int) * 4;
	char* stream = malloc(longitud_stream);
	int offset = 0;

	memcpy(stream + offset, &struct_iniciar_tripulante.PID, sizeof(int));
	offset += sizeof(int);

	memcpy(stream + offset, &struct_iniciar_tripulante.TID, sizeof(int));
	offset += sizeof(int);

	memcpy(stream + offset, &struct_iniciar_tripulante.posicion_X, sizeof(int));
	offset += sizeof(int);

	memcpy(stream + offset, &struct_iniciar_tripulante.posicion_Y, sizeof(int));
	offset += sizeof(int);

	exit_status = enviar_operacion(mi_ram_hq_fd, COD_INICIAR_TRIPULANTE, stream, longitud_stream);
	free(stream);

	return exit_status;
}

