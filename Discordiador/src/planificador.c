#include "planificador.h"

void iniciar_planificador(){
// TODO: ejecutar estas instrucciones si se inicio el planificador por 1ra vez, caso contrario actualizar flag pausado.
    pthread_t hilo_dispatcher;
    // TODO: leer config file - ALGORITMO

    // TODO: verificar que algoritmo recibido sea valido
    if(!es_algoritmo_valido(op_algoritmo)){
        // cancelar planificador:
        // liberar memoria
        // Log error: "Error de algoritmo de planificacion. Algoritmo no valido."
        return EXIT_FAILURE;
    }

    // TODO: crear hilo dispatcher
    hilo_dispatcher = malloc(sizeof(pthread_t));
	pthread_create(hilo_dispatcher, NULL, (void*) dispatcher, &struct_iniciar_tripulante);
	pthread_detach(*hilo_dispatcher);
	free(hilo_dispatcher);

}

int dispatcher(char* op_algoritmo){

    crear_colas();
    
    //operaciones ciclicas
    // TODO: para cada tripulante generado es necesario agregar al estado new.

    // mientras no haya emergencia/sabotaje:

    // operar transiciones: ante un cambio de estado de un Tripulante  
             
}

void crear_colas(){
    t_queue *cola_new= create_queue();
    t_queue *cola_ready= create_queue();
    t_queue *cola_running= create_queue();
    t_queue *cola_bloqueado_io= create_queue();
    t_queue *cola_bloqueado_emergency= create_queue();
}

void ordenamiento_ready_algoritmo_fifo(t_queue *nueva_cola,void* dato,t_queue *cola_origen){
    switch_cola_to(cola_ready,dato,cola_new);
}

bool verificar_algoritmo(char* cod_algor){
    if(strcpy(cod_algor, "FIFO") || strcpy(cod_algor,"RR"))
        return true;
    else
        return false;
}

static void switch_cola_to(t_queue *nueva_cola,void* dato,t_queue *cola_origen){
    // TODO
}