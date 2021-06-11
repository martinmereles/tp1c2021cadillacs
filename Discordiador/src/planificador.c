#include "planificador.h"

int iniciar_planificador(){

    if(estado_planificador == PLANIFICADOR_OFF){
        // TODO: ejecutar estas instrucciones si se inicio el planificador por 1ra vez, caso contrario actualizar flag pausado.
        pthread_t *hilo_dispatcher;

        // TODO: crear hilo dispatcher
        hilo_dispatcher = malloc(sizeof(pthread_t));
        pthread_create(hilo_dispatcher, NULL, (void*) dispatcher,&algoritmo_Planificador);
        pthread_detach(*hilo_dispatcher);
        free(hilo_dispatcher);
    }
    else {
        estado_planificador = PLANIFICADOR_RUNNING;
    }
    return EXIT_SUCCESS;
}

int dispatcher(void *algor_planif){
/*
    int flag_espacio_new = 0;
    int espacio_disponible = 0;
    int hay_tarea = 0;
    int hay_trip_a_borrar = 0;
    int hay_block_io = 0;
    int hay_sabot = 0;
    int listado_de_tripulantes_solicitado = 0;
*/
    int code_algor = string_to_code_algor(algor_planif);
    int GRADO_MP = 8; // TODO: variable global
    crear_colas();
	
    while(1){
        //flag_espacio_new = existe_tripulantes_en_cola(cola_new);
        printf("\nFlag tripulantes en NEW: %d\n",existe_tripulantes_en_cola(cola_new));
		if( existe_tripulantes_en_cola(cola_new) > 0){
            pasar_todos_new_to_ready(code_algor);
        }
        sleep(2);
        //espacio_disponible = hay_espacio_disponible();
        //hay_tarea = hay_tarea_a_realizar();
		if( hay_espacio_disponible(GRADO_MP) && hay_tarea_a_realizar() ){
            printf("\nAsignando tarea...\n");
			//TODO: "Asignar tarea (en realidad es pasar a exec el Tripulante ID que realmente ejecuta) ¿¿O se puede forzar al trip a eleccion??"
		}
        sleep(2);
        //hay_trip_a_borrar = hay_tripulantes_a_borrar();
		if( hay_tripulantes_a_borrar() ){
            expulsar_tripulante();
		}
        sleep(2);
        //hay_block_io = hay_bloqueo_io();
		if( hay_bloqueo_io() ){
            printf("\nPasando de exec a bloqueado IO...\n");
            gestionar_bloqueo_io(); // TODO
        }
        sleep(2);
        //hay_sabot = hay_sabotaje();
		if( hay_sabotaje() ){
            printf("\nAtendiendo sabotaje...\n");
            bloquear_tripulantes_por_sabotaje();
            while ( !termino_sabotaje() ); //ESPERA ACTIVA
            //TODO: pasar TRIPULANTE que atendio SABOTAJE de EXEC a BLOCKED_IO 
            desbloquear_tripulantes_tras_sabotaje();
		}
        sleep(2);
        //listado_de_tripulantes_solicitado = se_solicito_lista_trip();
        if( se_solicito_lista_trip() ){
            printf("\nListando tripulantes...\n");
            sleep(1);
            listar_tripulantes(code_algor);
        }
        printf("\nEsperando 5 segundos...\n");
        sleep(5);
	}
	//libero recursos:
    list_destroy_and_destroy_elements(cola_new->elements,destructor_elementos_tripulante);
    list_destroy_and_destroy_elements(cola_ready->elements,destructor_elementos_tripulante);
	list_destroy_and_destroy_elements(cola_running->elements,destructor_elementos_tripulante);
    list_destroy_and_destroy_elements(cola_bloqueado_io->elements,destructor_elementos_tripulante);
    list_destroy_and_destroy_elements(cola_bloqueado_emergency->elements,destructor_elementos_tripulante);
    list_destroy_and_destroy_elements(cola_exit->elements,destructor_elementos_tripulante);
    return 0;
}

static void crear_colas(){
    //t_queue *cola_new= create_queue();
    cola_ready = queue_create();
    cola_running = queue_create();
    cola_bloqueado_io = queue_create();
    cola_bloqueado_emergency = queue_create();
    cola_exit = queue_create();
    
}

static void pasar_todos_new_to_ready(enum algoritmo cod_algor){
    int cant_cola = queue_size(cola_new);
    t_tripulante *transito;
    
    for (int i = cant_cola; i > 0; i--){
        transito = queue_pop(cola_new); 
        transicion_new_to_ready(transito, cod_algor);
    }
}

static int transicion_new_to_ready(t_tripulante *dato, enum algoritmo cod_algor){
    t_tripulante *nuevo_trip_fifo;
    t_tripulante *nuevo_trip_rr;

    switch(cod_algor){
        case FIFO:
            //nuevo_trip_fifo = malloc(sizeof(t_tripulante));
            //memcpy(nuevo_trip_fifo, dato, sizeof(t_tripulante));
            nuevo_trip_fifo = dato;
            nuevo_trip_fifo->estado_previo = READY;
            nuevo_trip_fifo->quantum = 0;
            queue_push(cola_ready,nuevo_trip_fifo);
            break;
        case RR:
            //nuevo_trip_rr = malloc(sizeof(t_tripulante));
            //memcpy(&nuevo_trip_rr, dato, sizeof(t_tripulante));
            nuevo_trip_rr = dato;
            nuevo_trip_rr->estado_previo = READY;
            nuevo_trip_rr->quantum = 4;
            queue_push(cola_ready,nuevo_trip_rr);
            break;
        default:
            //Error de ejecucion de transicion.
            return EXIT_FAILURE;
    }
}

int existe_tripulantes_en_cola(t_queue *cola){
    return (queue_size(cola) > 0)? 1: 0;
}

int hay_espacio_disponible(int grado_multiprocesamiento){
    return (queue_size(cola_running) < grado_multiprocesamiento)? 1: 0;
}

int hay_tarea_a_realizar(void){
    // TODO: fijarse si hay siguiente tarea a leer AUN. ¿POSIBLE sincronizacion?
    // USAR funciones de commons txt / o medir si no alcanzó a EOF de lista_tareas.txt
    return 1;
}

int hay_tripulantes_a_borrar(void){
    return existe_tripulantes_en_cola(cola_exit);
}

void expulsar_tripulante(void){
    printf("\nExpulsando tripulantes...\n");
    list_clean_and_destroy_elements(cola_exit->elements,destructor_elementos_tripulante);
		if( queue_size(cola_exit) == 0)
            printf("Expulsados exitosamente\n");
        else
            printf("Error de borrado cola EXIT...\n");
}

int hay_bloqueo_io(void){
    return existe_tripulantes_en_cola(cola_bloqueado_io);
}

int hay_sabotaje(void){
    // TODO: verificar cuando reciba el MODULO DISCORDIADOR el aviso de parte de iMongo-Store.
    return 1;
}

int se_solicito_lista_trip(void){
    // TODO: verificar cuando reciba el MODULO DISCORDIADOR el comando.
    return 1;
}

void listar_tripulantes(enum algoritmo cod_algor){
    int cant_tripulantes = 0;
    int contador = 0;
    t_queue *listado_tripulantes;
    t_queue *temporal;
    cant_tripulantes += queue_size(cola_new);
    cant_tripulantes += queue_size(cola_ready);
    cant_tripulantes += queue_size(cola_running);
    cant_tripulantes += queue_size(cola_bloqueado_io);
    cant_tripulantes += queue_size(cola_bloqueado_emergency);
    //cant_tripulantes += queue_size(cola_exit);
    
    printf("\nCantidad de tripulantes existentes: %d\n", cant_tripulantes);

    listado_tripulantes = queue_create();
    temporal = queue_create();
    //agregando cada cola en un listado único:
    sleep(1);
    temporal->elements = list_duplicate(cola_new->elements);
    while(queue_size(temporal)>0){
        printf("Acumulando desde la cola NEW\n");
        queue_push(listado_tripulantes,queue_pop(temporal));
    }
    sleep(1);
    temporal->elements = list_duplicate(cola_ready->elements);
    while(queue_size(temporal)>0){
        printf("Acumulando desde la cola READY\n");
        queue_push(listado_tripulantes,queue_pop(temporal));
    }
    sleep(1);
    temporal->elements = list_duplicate(cola_running->elements);
    while(queue_size(temporal)>0){
        printf("Acumulando desde la cola RUNNING\n");
        queue_push(listado_tripulantes,queue_pop(temporal));
    }
    sleep(1);
    temporal->elements = list_duplicate(cola_bloqueado_io->elements);
    while(queue_size(temporal)>0){
        printf("Acumulando desde la cola BLOCKED_IO\n");
        queue_push(listado_tripulantes,queue_pop(temporal));
    }
    sleep(1);
    temporal->elements = list_duplicate(cola_bloqueado_emergency->elements);
    while(queue_size(temporal)>0){
        printf("Acumulando desde la cola BLOCKED_EMERGENCY\n");
        queue_push(listado_tripulantes,queue_pop(temporal));
    }
    sleep(1);
    // TODO: Agregar o no la cola EXIT-sobre Tripulantes pendientes de Expulsion (aun desarmando sus estructuras) 
    ordenar_lista_tid_ascendente(listado_tripulantes);

    if(cod_algor == RR){
        list_iterate(listado_tripulantes->elements,imprimir_info_elemento_rr);
        list_destroy_and_destroy_elements(temporal->elements,destructor_elementos_tripulante);
    }        
    else{
        list_iterate(listado_tripulantes->elements,imprimir_info_elemento_fifo);
        list_destroy_and_destroy_elements(temporal->elements,destructor_elementos_tripulante);
    }

}

static void imprimir_info_elemento_fifo(void *data){
    t_tripulante *tripulante_fifo;
    tripulante_fifo = data;
    printf("Elemento:\nNro TID: %d - Nro PID: %d - Estado: %s\n", tripulante_fifo->TID, tripulante_fifo->PID, code_dispatcher_to_string(tripulante_fifo->estado_previo));
}

static void imprimir_info_elemento_rr(void *data){
    t_tripulante *tripulante_rr;     
    tripulante_rr = data;
    printf("Elemento:\nNro TID: %d - Nro PID: %d - Estado: %s\n", tripulante_rr->TID, tripulante_rr->PID, code_dispatcher_to_string(tripulante_rr->estado_previo));
}

static void destructor_elementos_tripulante(void *data){
    t_tripulante *temp;
    temp = data;
    free(temp);
}

char *code_dispatcher_to_string(enum estado_tripulante code){
    switch(code){
        case NEW:
            return "NEW";
        case READY:
            return "READY";
        case EXEC:
            return "EXEC";
        case BLOCKED_IO:
            return "BLOCKED_IO";
        case BLOCKED_EMERGENCY:
            return "BLOCKED_EMERGENCY";
    }
}

void transicion_ready_to_exec(t_tripulante *dato){
    t_tripulante *temp;
    //temp = malloc(sizeof(t_tripulante));
    //memcpy(temp, dato, sizeof(t_tripulante));
    temp = dato;
    temp->estado_previo = EXEC;
    queue_push(cola_running,temp);
}

void transicion_exec_to_blocked_io(t_tripulante *dato, enum algoritmo cod_algor){
    t_tripulante *temp;
    //temp = malloc(sizeof(t_tripulante));
    //memcpy(temp, dato, sizeof(t_tripulante));
    temp = dato;
    temp->estado_previo = BLOCKED_IO;
    if(cod_algor == RR)
        temp->quantum = 0;
    queue_push(cola_bloqueado_io,temp);
}

void transicion_blocked_io_to_ready(t_tripulante *dato, enum algoritmo cod_algor){
    t_tripulante *temp;
    //temp = malloc(sizeof(t_tripulante));
    //memcpy(temp, dato, sizeof(t_tripulante));
    temp = dato;
    temp->estado_previo = READY;
    if(cod_algor == RR)
        temp->quantum = 4; //TODO: Variable global
    queue_push(cola_running,temp);
}

static void transicion_exec_to_ready(t_tripulante *dato){
    t_tripulante *temp;
    //temp = malloc(sizeof(t_tripulante));
    //memcpy(temp, dato, sizeof(t_tripulante));
    temp = dato;
    temp->estado_previo = READY;
    temp->quantum = 4; //TODO: Variable global
    queue_push(cola_ready,temp);
}

void bloquear_tripulantes_por_sabotaje(void){

    //Pasar todos los tripulantes a blocked_emerg SEGUN ORDEN (1-EXEC -> 2-READY ->3- BLOCKED_IO)
    t_queue *temporal;
    temporal = queue_create();
    //TODO
    //OBS: el UNICO tripulante que debe permanecer en EXEC es quien TRABAJA/ATIENDE el SABOTAJE.

    // Pasar desde cola EXEC/RUNNING
    while(existe_tripulantes_en_cola(cola_running))
        list_add_sorted(cola_bloqueado_emergency->elements,queue_pop(cola_running),tripulante_tid_es_menor_que);

    // Pasar desde cola READY
    while(existe_tripulantes_en_cola(cola_ready)){
        list_add_sorted(temporal->elements,queue_pop(cola_ready),tripulante_tid_es_menor_que);
        queue_push(cola_bloqueado_emergency,queue_pop(temporal));
    }

    // Pasar desde cola BLOCKED_IO
    while(existe_tripulantes_en_cola(cola_bloqueado_io)){
        list_add_sorted(temporal->elements,queue_pop(cola_bloqueado_io),tripulante_tid_es_menor_que);
        queue_push(cola_bloqueado_emergency,queue_pop(temporal));
    }
    queue_destroy(temporal);		
}

int termino_sabotaje(void){
    // TODO: verificar si termina el sabotaje. ¿Implementar con flag?
    return 1;
}

void ordenar_lista_tid_ascendente(t_queue *listado){
    list_sort(listado->elements,tripulante_tid_es_menor_que);
}

static bool tripulante_tid_es_menor_que(void *data1, void *data2){
    t_tripulante *temp1;
    t_tripulante *temp2;
    temp1 = data1;
    temp2 = data2;
    return (temp1->TID <= temp2->TID)? true: false;
}

void *get_data_tripulante_by_tid(t_queue *src_list, int tid_buscado){
    t_queue *buffer;
    buffer = src_list;
    t_list_iterator *un_iterador;
    bool encontrado = false;
    t_tripulante *tripulante_buscado;
    un_iterador = list_iterator_create(buffer->elements);

    while(list_iterator_has_next(un_iterador) && !encontrado){
        tripulante_buscado = un_iterador->element->data;
        if ( tripulante_buscado->TID == tid_buscado){
            encontrado = true;
            list_iterator_remove(un_iterador);
        }
        else
            list_iterator_next(un_iterador);
    }
    //if(!encontrado){ //printf("\nERROR en busqueda por TID no encontrado.");//list_iterator_remove(un_iterador); //return(EXIT_FAILURE); }
    //else
        return tripulante_buscado;
}

static void gestionar_bloqueo_io(void){
    printf("Gestionando bloqueos_IO...\n");
    // TODO:
      // Obtener lista de tripulantes por TID que se encuentran bloqueados. -> ¿Leyendo de una lista buffer?
	  // Pasar de exec a blocked_io (indicando el TID que se bloqueo correspondiente, SABIENDO QUIEN efectivamente lo está)	
}

static void desbloquear_tripulantes_tras_sabotaje(void){
    while(existe_tripulantes_en_cola(cola_bloqueado_emergency)){
        queue_push(cola_ready, queue_pop(cola_bloqueado_emergency));
    }
}

static enum algoritmo string_to_code_algor(char *string_code){
    if (strcmp("RR",string_code) == 0)
        return RR;
    else 
        if (strcmp("FIFO",string_code) == 0)
            return FIFO;
}// TODO: evaluar su aplicacion