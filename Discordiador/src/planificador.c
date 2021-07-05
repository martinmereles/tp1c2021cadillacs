#include "planificador.h"

void gestionar_tripulantes_en_exit();
int transicion(t_tripulante *tripulante, enum estado_tripulante estado_inicial, enum estado_tripulante estado_final);
bool existen_tripulantes_en_cola(int tipo_cola);
int cantidad_tripulantes_en_cola(int tipo_cola);
void pasar_todos_new_to_ready(enum algoritmo cod_algor);
 int hay_espacio_disponible(int grado_multiprocesamiento);
 void imprimir_info_elemento(void *data);
 void destructor_elementos_tripulante(void *data_tripulante);
 bool tripulante_tid_es_menor_que(void *lower, void *upper);
 int hay_sabotaje(void);
 int bloquear_tripulantes_por_sabotaje(void);
 int termino_sabotaje(void);
 void desbloquear_tripulantes_tras_sabotaje(void);
 void gestionar_exec(int grado_multiprocesamiento);
 t_tripulante *desencolar_tripulante_por_tid(t_queue *cola_src, int tid_buscado);
 int hay_tarea_a_realizar(void);
int estado_from(enum peticion_transicion tipo_peticion);
int estado_to(enum peticion_transicion tipo_peticion);
int get_buffer_peticiones_and_swap(enum peticion_transicion tipo_peticion);

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
	
    while(true){

        //log_debug(logger, "loop consultas");
        // Se admiten en el sistema cada uno de los tripulantes creados
        sem_wait(&sem_mutex_ejecutar_dispatcher);
        sem_wait(&sem_mutex_ingreso_tripulantes_new);
		if( existen_tripulantes_en_cola(NEW) ){
            log_debug(logger, "Atendiendo COLA NEW");
            pasar_todos_new_to_ready(code_algor);
        }
        sem_post(&sem_mutex_ingreso_tripulantes_new);
        sem_post(&sem_mutex_ejecutar_dispatcher);
        
        //espacio_disponible = hay_espacio_disponible();
        //hay_tarea = hay_tarea_a_realizar();
        sem_wait(&sem_mutex_ejecutar_dispatcher);
		if( hay_espacio_disponible(grado_multiproc) && existen_tripulantes_en_cola(READY) ){
            log_debug(logger, "Atendiendo COLA READY");
            gestionar_exec(grado_multiproc);
		}
        sem_post(&sem_mutex_ejecutar_dispatcher);

        // Consulto si hay que atender Bloqueos IO
        sem_wait(&sem_mutex_ejecutar_dispatcher);
        if(existen_tripulantes_en_cola(BLOCKED_IO_TO_READY)){
            log_debug(logger, "Atendiendo COLA BLOCKED IO TO READY");
            get_buffer_peticiones_and_swap(BLOCKED_IO_TO_READY);
        }
        sem_post(&sem_mutex_ejecutar_dispatcher);

        sem_wait(&sem_mutex_ejecutar_dispatcher);
        if(existen_tripulantes_en_cola(EXEC_TO_BLOCKED_IO)){
            log_debug(logger, "Atendiendo COLA EXEC TO BLOCKED IO");
            get_buffer_peticiones_and_swap(EXEC_TO_BLOCKED_IO);
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

        // Si hay tripulantes que se quedaron sin quantum (transicion EXEC_TO_READY)
        sem_wait(&sem_mutex_ejecutar_dispatcher);
		if( existen_tripulantes_en_cola(EXEC_TO_READY) )
            get_buffer_peticiones_and_swap(EXEC_TO_READY);
        sem_post(&sem_mutex_ejecutar_dispatcher);

        // Si hay tripulantes en EXIT
        sem_wait(&sem_mutex_ejecutar_dispatcher);
		while( existen_tripulantes_en_cola(EXIT) )
            gestionar_tripulantes_en_exit();
        sem_post(&sem_mutex_ejecutar_dispatcher);
    }

	//libero recursos:
    log_error(logger, "Desarmando todos los tripulantes de las colas por finalizacion");
    for(int tipo_cola = 0;tipo_cola < CANT_COLAS; tipo_cola++){
        list_destroy_and_destroy_elements(cola[tipo_cola]->elements, destructor_elementos_tripulante);
        sem_destroy(mutex_cola[tipo_cola]);
        free(mutex_cola[tipo_cola]);
    }
    list_destroy_and_destroy_elements(lista_tripulantes, destructor_elementos_tripulante);
    free(cola);
    free(mutex_cola); 
    return 0;
}


void gestionar_tripulantes_en_exit(){

    log_error(logger, "Iniciando gestion de cola EXIT");

    // Saco el primer tripulante de la lista de exit
    t_tripulante *tripulante = desencolar(EXIT);
    int tid = tripulante->TID;

    log_error(logger, "Iniciando expulsion tripulante %d", tripulante->TID);

    // Le aviso al tripulante que deje la cola de exit
    sem_post(tripulante->sem_tripulante_dejo[EXIT]);

    log_error(logger, "El tripulante %d dejo EXIT", tripulante->TID);

    // Espero a que el submodulo tripulante finalice para borrar sus estructuras
    sem_wait(tripulante->sem_finalizo);
    log_error(logger, "El submodulo tripulante %d finalizo", tripulante->TID);

    // Destruyo todas sus estructuras del Discordiador
    log_error(logger, "Destruyo estructuras del tripulante %d", tripulante->TID);
    destructor_elementos_tripulante(tripulante);

    log_debug(logger, "Se elimino al tripulante %d exitosamente", tid);
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
    cola = malloc(sizeof(t_queue *) * CANT_COLAS);
    mutex_cola = malloc(sizeof(sem_t *) * CANT_COLAS);
    for(int i = 0;i < CANT_COLAS;i++){
        cola[i] = queue_create();
        mutex_cola[i] = malloc(sizeof(sem_t));
        sem_init(mutex_cola[i], 0, 1);
    }
}

 void pasar_todos_new_to_ready(enum algoritmo cod_algor){
    int cant_cola = queue_size(cola[NEW]);
    t_tripulante *transito;
    
    for (int i = cant_cola; i > 0; i--){
        transito = desencolar(NEW); 
        transicion(transito, NEW, READY);
    }
}

int transicion(t_tripulante *tripulante, enum estado_tripulante estado_inicial, enum estado_tripulante estado_final){
    tripulante->estado = estado_final;
    tripulante->quantum = quantum;
    encolar(estado_final, tripulante);
    log_debug(logger, "Tripulante %d: %s => %s", 
        tripulante->TID, 
        code_dispatcher_to_string(estado_inicial),
        code_dispatcher_to_string(estado_final));
    sem_post(tripulante->sem_tripulante_dejo[estado_inicial]);
    return EXIT_SUCCESS;
}

bool existen_tripulantes_en_cola(int tipo_cola){
    return (queue_size(cola[tipo_cola]) > 0)? true: false;
}

 int hay_espacio_disponible(int grado_multiprocesamiento){
    return (queue_size(cola[EXEC]) < grado_multiprocesamiento)? 1: 0;
}

int hay_tarea_a_realizar(void){
    // TODO: fijarse si hay siguiente tarea a leer AUN. ¿POSIBLE sincronizacion?
    // USAR funciones de commons txt / o medir si no alcanzó a EOF de lista_tareas.txt
    return 1;
}

int rutina_expulsar_tripulante(void* args){
    int tid_buscado = *((int*) args);
    free(args);
    t_tripulante *trip_expulsado;

    if ( estado_planificador == PLANIFICADOR_OFF ){
        log_error(logger,"ERROR. rutina_expulsar_tripulante: El planificador esta apagado");
        return -1;
    }

    sem_wait(&sem_mutex_ejecutar_dispatcher);

    for(int estado_cola = 0; estado_cola < CANT_ESTADOS; estado_cola++){
        trip_expulsado = desencolar_tripulante_por_tid(cola[estado_cola], tid_buscado);
        if(trip_expulsado != NULL){
            transicion(trip_expulsado, estado_cola, EXIT);
	        sem_post(&sem_mutex_ejecutar_dispatcher);
            return EXIT_SUCCESS;
        }
    }
	
    log_error(logger,"El tripulante %d no existe", tid_buscado);
	sem_post(&sem_mutex_ejecutar_dispatcher);
    return EXIT_FAILURE;
}

 int hay_sabotaje(void){
    // TODO: verificar cuando reciba el MODULO DISCORDIADOR el aviso de parte de iMongo-Store.
    return 0;
}

int listar_tripulantes(void){
    char *fecha = temporal_get_string_time("%d/%m/%y %H:%M:%S");
    printf("------------------------------------------------\n");
    printf("Estado de la Nave: %s\n",fecha);
    list_iterate(lista_tripulantes, imprimir_info_elemento);
    printf("------------------------------------------------\n");
    free(fecha);
    return EXIT_SUCCESS;
}

void imprimir_info_elemento(void *data){
    t_tripulante *tripulante = data;
    printf("Tripulante:%3d\tPatota:%3d\tStatus: %s\n", tripulante->TID, tripulante->PID, code_dispatcher_to_string(tripulante->estado));
}

void destructor_elementos_tripulante(void *data){
    t_tripulante *tripulante = (t_tripulante*) data;

    bool tiene_TID_a_eliminar(void* args){
        t_tripulante* tripulante_encontrado = (t_tripulante*) args;
        return tripulante_encontrado->TID == tripulante->TID;
    }

    // Lo quito de la lista global de tripulantes
    list_remove_by_condition(lista_tripulantes, tiene_TID_a_eliminar);

    // Destruyo todos sus semaforos
    sem_destroy(tripulante->sem_planificacion_fue_reanudada); 
    free(tripulante->sem_planificacion_fue_reanudada);   
    sem_destroy(tripulante->sem_finalizo); 
    free(tripulante->sem_finalizo);        

    for(int i = 0;i < CANT_ESTADOS;i++){
        sem_destroy(tripulante->sem_tripulante_dejo[i]); 
        free(tripulante->sem_tripulante_dejo[i]);
    }

    free(tripulante);   // Libero el struct
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

int bloquear_tripulantes_por_sabotaje(void){

    //Pasar todos los tripulantes a blocked_emerg SEGUN ORDEN (1-EXEC -> 2-READY ->3- BLOCKED_IO)

    // Ordeno la cola de EXEC por TID
    list_sort(cola[EXEC]->elements, tripulante_tid_es_menor_que);
    // Muevo uno por uno los tripulantes a la cola BLOCKED_EMERGENCY
    while(existen_tripulantes_en_cola(EXEC))
        encolar(BLOCKED_EMERGENCY, desencolar(EXEC));

    // Ordeno la cola de READY por TID
    list_sort(cola[READY]->elements, tripulante_tid_es_menor_que);
    // Muevo uno por uno los tripulantes a la cola BLOCKED_EMERGENCY
    while(existen_tripulantes_en_cola(READY))
        encolar(BLOCKED_EMERGENCY, desencolar(READY));

    // Ordeno la cola de BLOCKED_IO por TID
    list_sort(cola[BLOCKED_IO]->elements, tripulante_tid_es_menor_que);
    // Muevo uno por uno los tripulantes a la cola BLOCKED_EMERGENCY
    while(existen_tripulantes_en_cola(BLOCKED_IO))
        encolar(BLOCKED_EMERGENCY, desencolar(BLOCKED_IO));

    if (!existen_tripulantes_en_cola(BLOCKED_EMERGENCY))
        return EXIT_FAILURE;

    // Si hay algun tripulante en la cola de BLOCKED_EMERGENCY:
    //TODO
    //OBS: se debe mover un UNICO tripulante a EXEC que es quien TRABAJA/ATIENDE el SABOTAJE.

    return EXIT_SUCCESS;
}

 int termino_sabotaje(void){
    // TODO: verificar si termina el sabotaje. ¿Implementar con flag?
    return 0;
}

 bool tripulante_tid_es_menor_que(void *data1, void *data2){
    t_tripulante *temp1 = data1;
    t_tripulante *temp2 = data2;
    return (temp1->TID <= temp2->TID)? true: false;
}

// Revisar (no van todos a ready)
 void desbloquear_tripulantes_tras_sabotaje(void){

    // Deberia guardarse el estado anterior antes del bloqueo
    // Asi al desbloquearse vuelven a donde estaban

    while(existen_tripulantes_en_cola(BLOCKED_EMERGENCY)){
        encolar(READY, desencolar(BLOCKED_EMERGENCY));
    }
}

enum algoritmo string_to_code_algor(char *string_code){
    if (strcmp("RR",string_code) == 0)
        return RR;
    else
        return FIFO;
}// TODO: evaluar su aplicacion

int get_buffer_peticiones_and_swap(enum peticion_transicion tipo_peticion){
    t_tripulante *tripulante;
    while( queue_size(cola[tipo_peticion]) > 0 ){
        tripulante = desencolar(tipo_peticion);
        desencolar_tripulante_por_tid(cola[estado_from(tipo_peticion)], tripulante->TID);
        transicion(tripulante, estado_from(tipo_peticion), estado_to(tipo_peticion));
    }
    return EXIT_SUCCESS;
}

int estado_from(enum peticion_transicion tipo_peticion){
    switch(tipo_peticion){
        case EXEC_TO_BLOCKED_IO:
        case EXEC_TO_READY:
            return EXEC;
        case BLOCKED_IO_TO_READY:
            return BLOCKED_IO;
    }
    return -1;
}

int estado_to(enum peticion_transicion tipo_peticion){
    switch(tipo_peticion){
        case EXEC_TO_BLOCKED_IO:
            return BLOCKED_IO;
        case EXEC_TO_READY:
        case BLOCKED_IO_TO_READY:
            return READY;
    }
    return -1;
}

 void gestionar_exec(int grado_multiprocesamiento){
    t_tripulante *tripulante;
    while( queue_size(cola[EXEC]) < grado_multiprocesamiento && existen_tripulantes_en_cola(READY) ){
        tripulante = desencolar(READY);
        transicion(tripulante, READY, EXEC);
    }
}

void dispatcher_pausar(){
    estado_planificador = PLANIFICADOR_BLOCKED;
    sem_wait(&sem_mutex_ejecutar_dispatcher);
}

t_tripulante *desencolar_tripulante_por_tid(t_queue *cola_src, int tid_buscado){

    bool tiene_TID_a_desencolar(void* args){
        t_tripulante* tripulante_encontrado = (t_tripulante*) args;
        return tripulante_encontrado->TID == tid_buscado;
    }

    return list_remove_by_condition(cola_src->elements, tiene_TID_a_desencolar);
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
	sem_init(nuevo -> sem_planificacion_fue_reanudada, 0, 0);    
    nuevo -> sem_finalizo = malloc(sizeof(sem_t));
	sem_init(nuevo -> sem_finalizo, 0, 0);    

    nuevo -> sem_tripulante_dejo = malloc(sizeof(sem_t*) * 6);
    for(int i = 0;i < CANT_ESTADOS;i++){
        nuevo->sem_tripulante_dejo[i] = malloc(sizeof(sem_t));
        sem_init(nuevo->sem_tripulante_dejo[i], 0, 0);
    }

    // Lo agrego a la lista global de tripulantes ordenado por TID
    list_add_sorted(lista_tripulantes, nuevo, tripulante_tid_es_menor_que);
    encolar(NEW, nuevo);
    return nuevo;
}

void encolar(int tipo_cola, t_tripulante* tripulante){
    sem_wait(mutex_cola[tipo_cola]);
    queue_push(cola[tipo_cola], tripulante);
    sem_post(mutex_cola[tipo_cola]); 
}

t_tripulante* desencolar(int tipo_cola){
    t_tripulante* tripulante;
    sem_wait(mutex_cola[tipo_cola]);
    tripulante = queue_pop(cola[tipo_cola]);
    sem_post(mutex_cola[tipo_cola]); 
    return tripulante;
}