#include "planificador.h"

void crear_colas();
int existe_tripulantes_en_cola(t_queue *cola);
void pasar_todos_new_to_ready(enum algoritmo cod_algor);
 int hay_espacio_disponible(int grado_multiprocesamiento);
 int transicion_new_to_ready(t_tripulante *dato, enum algoritmo cod_algor);
 void imprimir_info_elemento_fifo(void *data);
 void imprimir_info_elemento_rr(void *data);
 void transicion_ready_to_exec(t_tripulante *dato);
 void transicion_exec_to_blocked_io(t_tripulante *dato, enum algoritmo cod_algor);
 void transicion_blocked_io_to_ready(t_tripulante *dato, enum algoritmo cod_algor);
 void transicion_exec_to_ready(t_tripulante *dato); // RR
 void destructor_elementos_tripulante(void *data_tripulante);
 void ordenar_lista_tid_ascendente(t_queue *listado);
 bool tripulante_tid_es_menor_que(void *lower, void *upper);
 int hay_bloqueo_io(void);
 void gestionar_bloqueo_io(t_queue *peticiones_from_exec, t_queue *peticiones_to_ready, enum algoritmo code_algor);
 int hay_sabotaje(void);
 int bloquear_tripulantes_por_sabotaje(void);
 int termino_sabotaje(void);
 void desbloquear_tripulantes_tras_sabotaje(void);
 int get_buffer_peticiones_and_swap_exec_blocked_io(t_queue *peticiones_origen, enum algoritmo code_algor);
 int get_buffer_peticiones_and_swap_blocked_io_ready(t_queue *peticiones_origen, enum algoritmo code_algor);
 void gestionar_exec(int grado_multiprocesamiento);
 int get_index_from_cola_by_tid(t_queue *src_list, int tid_buscado);
 t_tripulante *obtener_tripulante_por_tid(t_queue *cola_src, int tid_buscado);
 int hay_tripulantes_sin_quantum();
 void gestionar_tripulantes_sin_quantum();
 int hay_tarea_a_realizar(void);

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
    sabotaje_activo=0;
    int code_algor = string_to_code_algor(algor_planif);

    crear_colas();
	
    while(true){
        //log_debug(logger, "loop consultas");
        // Se admiten en el sistema cada uno de los tripulantes creados
        sem_wait(&sem_mutex_ejecutar_dispatcher);
        sem_wait(&sem_mutex_ingreso_tripulantes_new);
		if( existe_tripulantes_en_cola(cola_new) > 0){
            log_debug(logger, "Atendiendo COLA NEW");
            pasar_todos_new_to_ready(code_algor);
        }
        sem_post(&sem_mutex_ingreso_tripulantes_new);
        sem_post(&sem_mutex_ejecutar_dispatcher);
        
        //espacio_disponible = hay_espacio_disponible();
        //hay_tarea = hay_tarea_a_realizar();
        sem_wait(&sem_mutex_ejecutar_dispatcher);
		if( hay_espacio_disponible(grado_multiproc) && existe_tripulantes_en_cola(cola_ready) ){
            log_debug(logger, "Atendiendo COLA EXEC");
            gestionar_exec(grado_multiproc);
		}
        sem_post(&sem_mutex_ejecutar_dispatcher);

        // Consulto si hay que atender Bloqueos IO
        sem_wait(&sem_mutex_ejecutar_dispatcher);
		if( hay_bloqueo_io() ){
            log_debug(logger, "Atendiendo COLA BLOCKED IO");
            gestionar_bloqueo_io(buffer_peticiones_exec_to_blocked_io, buffer_peticiones_blocked_io_to_ready, code_algor);
        }
        sem_post(&sem_mutex_ejecutar_dispatcher);

        // Consulto si hay sabotaje
        sem_wait(&sem_mutex_ejecutar_dispatcher);
		if( hay_sabotaje() ){
            //Atendiendo sabotaje
            log_debug(logger, "Atendiendo COLA BLOCKED_EMERGENCY");
            if (bloquear_tripulantes_por_sabotaje() != EXIT_SUCCESS)
                log_error(logger, "No se pudo ejecutar la funcion de bloqueo ante sabotajes");
            sem_wait(&sem_sabotaje_activado);
            //while ( !termino_sabotaje() ); //ESPERA ACTIVA o implementar SEMÁFORO.
            //TODO: pasar TRIPULANTE que atendio SABOTAJE de EXEC a BLOCKED_IO
            log_debug(logger, "Termino sabotaje"); 
            desbloquear_tripulantes_tras_sabotaje();
		}
        sem_post(&sem_mutex_ejecutar_dispatcher);

        sem_wait(&sem_mutex_ejecutar_dispatcher);
		if( hay_tripulantes_sin_quantum() )
            gestionar_tripulantes_sin_quantum();
        sem_post(&sem_mutex_ejecutar_dispatcher);
	}
	//libero recursos:
    log_error(logger, "Desarmando todos los tripulantes de las colas por finalizacion");
    list_destroy_and_destroy_elements(cola_new->elements,destructor_elementos_tripulante);
    list_destroy_and_destroy_elements(cola_ready->elements,destructor_elementos_tripulante);
	list_destroy_and_destroy_elements(cola_running->elements,destructor_elementos_tripulante);
    list_destroy_and_destroy_elements(cola_bloqueado_io->elements,destructor_elementos_tripulante);
    list_destroy_and_destroy_elements(cola_bloqueado_emergency->elements,destructor_elementos_tripulante);
    list_destroy_and_destroy_elements(cola_exit->elements,destructor_elementos_tripulante);
    return 0;
}

int iniciar_dispatcher(char *algoritmo_planificador){

    if(estado_planificador == PLANIFICADOR_OFF){
        pthread_t *hilo_dispatcher;
        hilo_dispatcher = malloc(sizeof(pthread_t));

        // Creando hilo del dispatcher
        pthread_create(hilo_dispatcher, NULL, (void*) dispatcher,&algoritmo_planificador);
        pthread_detach(*hilo_dispatcher);
        free(hilo_dispatcher);

        estado_planificador = PLANIFICADOR_RUNNING;
    }
    else if(estado_planificador == PLANIFICADOR_BLOCKED){
        sem_post(&sem_mutex_ejecutar_dispatcher);
        estado_planificador = PLANIFICADOR_RUNNING;
    }
    
    return EXIT_SUCCESS;
}

 void crear_colas(){
    cola_ready = queue_create();
    cola_running = queue_create();
    cola_bloqueado_io = queue_create();
    cola_bloqueado_emergency = queue_create();
    cola_exit = queue_create();
    
    buffer_peticiones_exec_to_blocked_io = queue_create();
    buffer_peticiones_blocked_io_to_ready = queue_create();
    buffer_peticiones_exec_to_ready = queue_create();
}

 void pasar_todos_new_to_ready(enum algoritmo cod_algor){
    int cant_cola = queue_size(cola_new);
    t_tripulante *transito;
    
    for (int i = cant_cola; i > 0; i--){
        transito = queue_pop(cola_new); 
        transicion_new_to_ready(transito, cod_algor);
    }
}

 int transicion_new_to_ready(t_tripulante *nuevo_trip, enum algoritmo cod_algor){
    switch(cod_algor){
        case FIFO:
        case RR:
            nuevo_trip->estado = READY;
            nuevo_trip->quantum = quantum;
            queue_push(cola_ready, nuevo_trip); // Variable global (si es FIFO vale 0 por defecto)
            sem_post(nuevo_trip->sem_tripulante_dejo_new);
            break;
        default:
            //Error de ejecucion de transicion.
            return EXIT_FAILURE;
    }
    return EXIT_SUCCESS;
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

int dispatcher_expulsar_tripulante(int tid_buscado){
    int estado = -1;
    t_tripulante *trip_expulsado;

    if ( estado_planificador == PLANIFICADOR_OFF )
        return -1;

    // Busqueda del tripulante a expulsar por cada cola del planificador

    // TODO: se puede buscar en la lista global
    /*
        // TODO: Hay un deadlock??
        // desbloqueo semaforo
        int valor_semaforo;
        log_info(logger, "Reanudo planificacion del tripulante %d", trip_expulsado->TID);
        sem_getvalue(trip_expulsado->sem_planificacion_fue_reanudada, &valor_semaforo);
	    if(valor_semaforo == 0)
        sem_post(&(trip_expulsado->sem_planificacion_fue_reanudada));
    */


    // busqueda en cola BLOQUEADO_IO:
    estado = get_index_from_cola_by_tid(cola_bloqueado_io, tid_buscado);
    if(estado != -1){
        trip_expulsado = obtener_tripulante_por_tid(cola_bloqueado_io, tid_buscado);        
        queue_push(cola_exit, trip_expulsado);
        //list_clean_and_destroy_elements(cola_exit->elements,destructor_elementos_tripulante);
    }
    else{

    // busqueda en cola NEW:
        estado = get_index_from_cola_by_tid(cola_new, tid_buscado);
        if(estado != -1){
            trip_expulsado = obtener_tripulante_por_tid(cola_new, tid_buscado);
            queue_push(cola_exit, trip_expulsado);
            //list_clean_and_destroy_elements(cola_exit->elements,destructor_elementos_tripulante);
        }
        else{

    // busqueda en cola READY:
            estado = get_index_from_cola_by_tid(cola_ready, tid_buscado);
            if(estado != -1){
                trip_expulsado = obtener_tripulante_por_tid(cola_ready, tid_buscado);
                queue_push(cola_exit, trip_expulsado);
                //list_clean_and_destroy_elements(cola_exit->elements,destructor_elementos_tripulante);
            }
            else{

    // busqueda en cola RUNNING/EXEC:
                estado = get_index_from_cola_by_tid(cola_running, tid_buscado);
                if(estado != -1){
                    trip_expulsado = obtener_tripulante_por_tid(cola_running, tid_buscado);
                    queue_push(cola_exit, trip_expulsado);
                    //list_clean_and_destroy_elements(cola_exit->elements,destructor_elementos_tripulante);
                }
                else{
    // busqueda en cola EXIT:
                    estado = get_index_from_cola_by_tid(cola_exit, tid_buscado);
                    if(estado != -1){
                        log_warning(logger, "El tripulante esta pendiente de expulsion");
                        //list_clean_and_destroy_elements(cola_exit->elements,destructor_elementos_tripulante);
                    }
                    else
                    // Error: no encontrado / ya fue expulsado.
                        return EXIT_FAILURE;
                }
            }
        }
    }
    //list_clean_and_destroy_elements(cola_exit->elements,destructor_elementos_tripulante);
    return EXIT_SUCCESS;
}

 int hay_bloqueo_io(void){
    return existe_tripulantes_en_cola(buffer_peticiones_blocked_io_to_ready) || existe_tripulantes_en_cola(buffer_peticiones_exec_to_blocked_io);
}

 int hay_sabotaje(void){
    // TODO: verificar cuando reciba el MODULO DISCORDIADOR el aviso de parte de iMongo-Store.
    return 0;
}

int listar_tripulantes(void){
    t_queue *listado_tripulantes;
    t_queue *temporal;
    char *fecha;

    listado_tripulantes = queue_create();
    temporal = queue_create();

    //-- Formando un listado único a partir de la copia de cada cola: --

    // Acumulando desde la cola NEW
    temporal->elements = list_duplicate(cola_new->elements);
    while(queue_size(temporal)>0){
        queue_push(listado_tripulantes,queue_pop(temporal));
    }

    if(estado_planificador != PLANIFICADOR_OFF){
        // Acumulando desde la cola READY
        temporal->elements = list_duplicate(cola_ready->elements);
        while(queue_size(temporal)>0){
            queue_push(listado_tripulantes,queue_pop(temporal));
        }
        // Acumulando desde la cola RUNNING
        temporal->elements = list_duplicate(cola_running->elements);
        while(queue_size(temporal)>0){
            queue_push(listado_tripulantes,queue_pop(temporal));
        }

        // Acumulando desde la cola BLOCKED_IO
        temporal->elements = list_duplicate(cola_bloqueado_io->elements);
        while(queue_size(temporal)>0){
            queue_push(listado_tripulantes,queue_pop(temporal));
        }

        // Acumulando desde la cola BLOCKED_EMERGENCY
        temporal->elements = list_duplicate(cola_bloqueado_emergency->elements);
        while(queue_size(temporal)>0){
            queue_push(listado_tripulantes,queue_pop(temporal));
        }

        // Acumulando desde la cola EXIT
        temporal->elements = list_duplicate(cola_exit->elements);
        while(queue_size(temporal)>0){
            queue_push(listado_tripulantes,queue_pop(temporal));
        }
    }
    
    ordenar_lista_tid_ascendente(listado_tripulantes);

    if(!existe_tripulantes_en_cola(listado_tripulantes))
        return EXIT_FAILURE;

    fecha = temporal_get_string_time("%d/%m/%y %H:%M:%S");
    printf("------------------------------------------------\n");
    printf("Estado de la Nave: %s\n",fecha);
    list_iterate(listado_tripulantes->elements,imprimir_info_elemento_fifo);
    printf("------------------------------------------------\n");
    queue_destroy_and_destroy_elements(listado_tripulantes,destructor_elementos_tripulante);

    queue_destroy_and_destroy_elements(temporal,free);
    free(fecha);
    return EXIT_SUCCESS;
}

void imprimir_info_elemento_fifo(void *data){
    t_tripulante *tripulante_fifo;
    tripulante_fifo = data;
    printf("Tripulante:%3d\tPatota:%3d\tStatus: %s\n", tripulante_fifo->TID, tripulante_fifo->PID, code_dispatcher_to_string(tripulante_fifo->estado));
}
/*
 void imprimir_info_elemento_rr(void *data){
    t_tripulante *tripulante_rr;     
    tripulante_rr = data;
    printf("Elemento:\nNro TID: %d - Nro PID: %d - Estado: %s\n", tripulante_rr->TID, tripulante_rr->PID, code_dispatcher_to_string(tripulante_rr->estado));
}*/

 void destructor_elementos_tripulante(void *data){
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
        case EXIT:
            return "EXIT";
        default:
            return "";
    }
}

 void transicion_ready_to_exec(t_tripulante *tripulante){
    tripulante->estado = EXEC;
    queue_push(cola_running, tripulante);
    sem_post(tripulante->sem_tripulante_dejo_ready);
}

 void transicion_exec_to_blocked_io(t_tripulante *dato, enum algoritmo cod_algor){
    t_tripulante *temp;
    //temp = malloc(sizeof(t_tripulante));
    //memcpy(temp, dato, sizeof(t_tripulante));
    temp = dato;
    temp->estado = BLOCKED_IO;
    if(cod_algor == RR)
        temp->quantum = 0;
    queue_push(cola_bloqueado_io,temp);
}

 void transicion_blocked_io_to_ready(t_tripulante *dato, enum algoritmo cod_algor){
    t_tripulante *temp;
    //temp = malloc(sizeof(t_tripulante));
    //memcpy(temp, dato, sizeof(t_tripulante));
    temp = dato;
    temp->estado = READY;
    if(cod_algor == RR)
        temp->quantum = quantum;
    queue_push(cola_running,temp);
}

 void transicion_exec_to_ready(t_tripulante *dato){
    t_tripulante *temp;
    //temp = malloc(sizeof(t_tripulante));
    //memcpy(temp, dato, sizeof(t_tripulante));
    temp = dato;
    temp->estado = READY;
    temp->quantum = quantum;
    queue_push(cola_ready,temp);
}

 int bloquear_tripulantes_por_sabotaje(void){

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

    // Pasar desde cola BLOCKED_IO // los BLOCKED_IO deben quedar en "pausa" sin ejecutar ciclos pero no cambian de cola
    /*while(existe_tripulantes_en_cola(cola_bloqueado_io)){
        list_add_sorted(temporal->elements,queue_pop(cola_bloqueado_io),tripulante_tid_es_menor_que);
        queue_push(cola_bloqueado_emergency,queue_pop(temporal));
    }*/

    queue_destroy(temporal);

    if (!existe_tripulantes_en_cola(cola_bloqueado_emergency))
        return EXIT_FAILURE;
    return EXIT_SUCCESS;
}

 int termino_sabotaje(void){
    // TODO: verificar si termina el sabotaje. ¿Implementar con flag?
    return 0;
}

 void ordenar_lista_tid_ascendente(t_queue *listado){
    list_sort(listado->elements,tripulante_tid_es_menor_que);
}

 bool tripulante_tid_es_menor_que(void *data1, void *data2){
    t_tripulante *temp1;
    t_tripulante *temp2;
    temp1 = data1;
    temp2 = data2;
    return (temp1->TID <= temp2->TID)? true: false;
}

 int get_index_from_cola_by_tid(t_queue *src_list, int tid_buscado){
    t_list_iterator *iterador_nuevo;
    iterador_nuevo = list_iterator_create(src_list->elements);
    t_tripulante *tripulante_buscado;
    int index_buscado = -1;

    // Busqueda del tripulante por TID y retornar su index
    while( list_iterator_has_next(iterador_nuevo) ){
        tripulante_buscado = list_iterator_next(iterador_nuevo);
        if(tripulante_buscado->TID == tid_buscado){
            index_buscado = iterador_nuevo->index;
            break;
        }
    }
    list_iterator_destroy(iterador_nuevo);
    return index_buscado;
}

 void gestionar_bloqueo_io(t_queue *peticiones_from_exec, t_queue *peticiones_to_ready, enum algoritmo code_algor){
    // Gestionando bloqueos_IO
    if (get_buffer_peticiones_and_swap_exec_blocked_io(peticiones_from_exec, code_algor) != EXIT_SUCCESS )
        log_error(logger, "Error en atender peticiones EXEC to BLOCKED IO");
    if (get_buffer_peticiones_and_swap_blocked_io_ready(peticiones_to_ready, code_algor) != EXIT_SUCCESS )
        log_error(logger, "Error en atender peticiones BLOCKED IO to READY");
}

 void desbloquear_tripulantes_tras_sabotaje(void){
    while(existe_tripulantes_en_cola(cola_bloqueado_emergency)){
        queue_push(cola_ready, queue_pop(cola_bloqueado_emergency));
    }
}

enum algoritmo string_to_code_algor(char *string_code){
    if (strcmp("RR",string_code) == 0)
        return RR;
    else
        return FIFO;
}// TODO: evaluar su aplicacion

void agregar_a_buffer_peticiones(t_queue *peticiones_destino, int tid){
    int *tid_ptr;
    tid_ptr = malloc(sizeof(int));
    *tid_ptr = tid;
    queue_push(peticiones_destino, tid_ptr);
}

 int get_buffer_peticiones_and_swap_exec_blocked_io(t_queue *peticiones_origen, enum algoritmo code_algor){
    int *buffer;
    t_tripulante *trip_transito;
    if ( queue_size(peticiones_origen) > 0){
        while( queue_size(peticiones_origen) > 0 ){
            buffer = queue_pop(peticiones_origen);
            trip_transito = obtener_tripulante_por_tid(cola_running, *buffer);
            transicion_exec_to_blocked_io(trip_transito, code_algor);
            free(buffer);
        }
        return EXIT_SUCCESS;
    }else
        return EXIT_FAILURE;
}

 int get_buffer_peticiones_and_swap_blocked_io_ready(t_queue *peticiones_origen, enum algoritmo code_algor){
    int *buffer;
    t_tripulante *trip_transito;
    if ( queue_size(peticiones_origen) > 0){
        while( queue_size(peticiones_origen) > 0 ){
            buffer = queue_pop(peticiones_origen);
            trip_transito = obtener_tripulante_por_tid(cola_bloqueado_io, *buffer);
            transicion_blocked_io_to_ready(trip_transito, code_algor);
            free(buffer);
        }
        return EXIT_SUCCESS;
    }else
        return EXIT_FAILURE;
}

 void gestionar_exec(int grado_multiprocesamiento){
    t_tripulante *temp;
    while( queue_size(cola_running) < grado_multiprocesamiento && existe_tripulantes_en_cola(cola_ready) ){
        temp = queue_pop(cola_ready);
        transicion_ready_to_exec(temp);
    }
}

void dispatcher_pausar(){
    estado_planificador = PLANIFICADOR_BLOCKED;
    sem_wait(&sem_mutex_ejecutar_dispatcher);
}

 t_tripulante *obtener_tripulante_por_tid(t_queue *cola_src, int tid_buscado){
    int index;
    index = get_index_from_cola_by_tid(cola_src, tid_buscado);
    return list_remove(cola_src->elements, index);
}

 int hay_tripulantes_sin_quantum(){
    return existe_tripulantes_en_cola(buffer_peticiones_exec_to_ready);
}

 void gestionar_tripulantes_sin_quantum(){
    int *buffer;
    while( hay_tripulantes_sin_quantum() ){
        buffer = queue_pop(buffer_peticiones_exec_to_ready);
        transicion_exec_to_ready(obtener_tripulante_por_tid(cola_running, *buffer));
    }
}


int dispatcher_eliminar_tripulante(int tid_eliminar){
    /*t_tripulante* tripulante = obtener_tripulante_por_tid(cola_exit, tid_eliminar);

    bool tiene_TID_a_eliminar(void* args){
        t_tripulante* tripulante_encontrado = (t_tripulante*) args;
        return tripulante_encontrado->TID == tid_eliminar;
    }

    // Lo quito de la lista global de tripulantes
    list_remove_by_condition(lista_tripulantes, tiene_TID_a_eliminar);

    // Destruyo todos sus semaforos
    sem_destroy(tripulante->sem_planificacion_fue_reanudada);    
    sem_destroy(tripulante->sem_tripulante_dejo_ready);          
    sem_destroy(tripulante->sem_tripulante_dejo_new);            
    free(tripulante->sem_planificacion_fue_reanudada);         
    free(tripulante->sem_tripulante_dejo_ready);         
    free(tripulante->sem_tripulante_dejo_new);

    free(tripulante);   // Libero el struct
*/
    if (dispatcher_expulsar_tripulante(tid_eliminar) != EXIT_SUCCESS)
        return EXIT_SUCCESS;
    else
        return EXIT_FAILURE;
}

t_tripulante* iniciador_tripulante(int tid, int pid, int pos_x, int pos_y){
    t_tripulante *nuevo;
    nuevo = malloc(sizeof(t_tripulante));
    nuevo -> PID = pid;
    nuevo -> TID = tid;
    nuevo -> posicion_X = pos_x;
    nuevo -> posicion_Y = pos_y;
    nuevo -> estado = NEW;
    
	// Inicializamos los semaforos (inicializan todos en 0)
    nuevo -> sem_planificacion_fue_reanudada = malloc(sizeof(sem_t));
    nuevo -> sem_tripulante_dejo_ready = malloc(sizeof(sem_t));
    nuevo -> sem_tripulante_dejo_new = malloc(sizeof(sem_t));

	sem_init(nuevo -> sem_planificacion_fue_reanudada, 0, 0);    // Inicializa en 0 a proposito
    sem_init(nuevo -> sem_tripulante_dejo_ready, 0, 0);          // Inicializa en 0 a proposito
    sem_init(nuevo -> sem_tripulante_dejo_new, 0, 0);            // Inicializa en 0 a proposito

    list_add(lista_tripulantes, nuevo); // Lo agrego a la lista global de tripulantes
    queue_push(cola_new, nuevo);
    return nuevo;
}