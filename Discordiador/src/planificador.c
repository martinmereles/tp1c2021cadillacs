#include "planificador.h"

void iniciar_planificador(){

    if(estado_planificador == OFF){
        // TODO: ejecutar estas instrucciones si se inicio el planificador por 1ra vez, caso contrario actualizar flag pausado.
        pthread_t *hilo_dispatcher;
    
        // TODO: verificar que algoritmo recibido sea valido
        if(!es_algoritmo_valido(algoritmo_Planificador)){
            // cancelar planificador:
            // liberar memoria
            // Log error: "Error de algoritmo de planificacion. Algoritmo no valido."
            /*
            t_config *config = config_create("./cfg/discordiador.config");
            algoritmo_Planificador = string_new();
            string_append(&algoritmo_Planificador, config_get_string_value(config, "ALGORITMO"));
            config_destroy(config);
            */
            return EXIT_FAILURE;
        }

        // TODO: crear hilo dispatcher
        hilo_dispatcher = malloc(sizeof(pthread_t));
        pthread_create(hilo_dispatcher, NULL, (void*) dispatcher,NULL);
        pthread_detach(*hilo_dispatcher);
        free(hilo_dispatcher);
    }
    else {
        estado_planificador = RUNNING;
    }

}

int dispatcher(void * args){

    crear_colas();
    
    while (estado_planificador == RUNNING){
        // Si existe cola en espera de NEW a READY:
        pasar_todos_new_to_ready(algoritmo_Planificador);

        // Ante la existencia de tareas, pasar (READY->EXEC validando TID) un tripulante por tarea HASTA completar cupo (grado de multiprocesamiento) o no haya tarea.

        // Verificar existencia de sabotaje
            //Caso afirmativo: pasar todos los tripulantes a blocked en orden segun ENUNCIADO.
            //Esperar a flag para dar finalizado el sabotaje
            //Ir devolviendo a sus estados previos en orden segun ENUNCIADO.

        // Vaciar cola exec (si existiesen elementos) liberando recursos correspondientes.

    }
    // mientras no haya emergencia/sabotaje:

    // operar transiciones: ante un cambio de estado de un Tripulante  
             
}

void crear_colas(void){
    //t_queue *cola_new= create_queue();
    cola_ready= create_queue();
    cola_running= create_queue();
    cola_bloqueado_io= create_queue();
    cola_bloqueado_emergency= create_queue();
    cola_exit= create_queue();
    
    /*
    index_planificador = create_queue();
    
    //Agregar a lista del planificador:
    queue_push(index_planificador,cola_new);
    queue_push(index_planificador,cola_ready);
    queue_push(index_planificador,cola_running);
    queue_push(index_planificador,cola_bloqueado_io);
    queue_push(index_planificador,cola_bloqueado_emergency);
    queue_push(index_planificador,cola_exit);
    */
}

void pasar_todos_new_to_ready(char* cod_algor){
    int cant_cola = queue_size(cola_new);
    t_key_tripulante *transito;
    
    for (int i = cant_cola; i > 0; i--){
        transito = queue_pop(cola_new); 
        transicion_new_to_ready(transito, algoritmo_Planificador);
    }
}

void ordenamiento_ready_algoritmo_fifo(t_queue *nueva_cola,void* dato,t_queue *cola_origen){
    switch_cola_to(cola_ready,dato,cola_new);
}

bool es_algoritmo_valido(char* cod_algor){
    if(strcpy(cod_algor, "FIFO") || strcpy(cod_algor,"RR"))
        return true;
    else
        return false;
}

static void transicion_new_to_ready(void *dato, char *cod_algor){
    
    switch(cod_algor){
        case "FIFO":
            t_dato_tripulante_fifo *nuevo_trip = malloc(sizeof(t_dato_tripulante_fifo));
            nuevo_trip->key = dato;
            nuevo_trip->estado_previo = NEW;
            queue_push(cola_ready,nuevo_trip);
            break;
        case "RR":
            t_dato_tripulante_rr *nuevo_trip = malloc(sizeof(t_dato_tripulante_rr));
            nuevo_trip->key = dato;
            nuevo_trip->estado_previo = NEW;
            nuevo_trip->quantum // = VARIABLE GLOBAL
            queue_push(cola_ready,nuevo_trip);
            break;
        default:
            //Error de ejecucion de transicion.
            return EXIT_FAILURE;
    }
}

static void switch_cola_to(t_queue *nueva_cola,void* dato){
    void *transito = dato;
    //transito = queue_pop(cola_origen); 
    queue_push(nueva_cola,transito);
}//Este metodo no debe usarse para NEW -> READY