#include "planificador.h"

void gestionar_tripulantes_en_exit();
int transicion(t_tripulante *tripulante, enum estado_tripulante estado_inicial, enum estado_tripulante estado_final);
bool existen_tripulantes_en_cola(int tipo_cola);
int cantidad_tripulantes_en_cola(int tipo_cola);
void pasar_todos_new_to_ready();
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

    t_tripulante* primer_tripulante;
    t_tripulante* tripulante;

    sabotaje_activo=0;

    log_info(logger,"DISPATCHER CREADO");
	
    while(status_discordiador != END || !list_is_empty(lista_tripulantes)){

        sem_wait(&sem_hay_evento_planificable);
        sem_wait(&sem_mutex_ejecutar_dispatcher);
        
        do{
            // Se admiten en el sistema cada uno de los tripulantes creados
            sem_wait(&sem_mutex_ingreso_tripulantes_new);
            while( existen_tripulantes_en_cola(NEW) ){
                //log_debug(logger, "Atendiendo COLA NEW");
                tripulante = desencolar(NEW); 
                transicion(tripulante, NEW, READY);
                sem_post(&sem_mutex_ingreso_tripulantes_new);
            }
            sem_post(&sem_mutex_ingreso_tripulantes_new);
            
            while( hay_espacio_disponible(grado_multiproc) && existen_tripulantes_en_cola(READY) ){
                //log_debug(logger, "Atendiendo COLA READY");
                tripulante = desencolar(READY);
                transicion(tripulante, READY, EXEC);
            }

            // Consulto si hay que atender Bloqueos IO
            while(existen_tripulantes_en_cola(BLOCKED_IO_TO_READY)){
                //log_debug(logger, "Atendiendo COLA BLOCKED IO TO READY");
                get_buffer_peticiones_and_swap(BLOCKED_IO_TO_READY);

                // Si quedo algun tripulante en la cola, al primero le dejo usar el dispositivo de E/S
                primer_tripulante = ojear_cola(BLOCKED_IO);
                if(primer_tripulante != NULL)
                    sem_post(primer_tripulante->sem_puede_usar_dispositivo_io);
            }

            while(existen_tripulantes_en_cola(EXEC_TO_BLOCKED_IO)){
                //log_debug(logger, "Atendiendo COLA EXEC TO BLOCKED IO");

                int cant_tripulantes = cantidad_tripulantes_en_cola(BLOCKED_IO);

                get_buffer_peticiones_and_swap(EXEC_TO_BLOCKED_IO);

                // Si la cola estaba vacia, al tripulante agregado (si hay) le dejo usar el dispositivo de E/S
                if(cant_tripulantes == 0){
                    primer_tripulante = ojear_cola(BLOCKED_IO);
                    if(primer_tripulante != NULL)
                        sem_post(primer_tripulante->sem_puede_usar_dispositivo_io);  
                }
            }

            // Consulto si hay sabotaje
            while( hay_sabotaje() ){
                //Atendiendo sabotaje
                log_debug(logger, "Atendiendo COLA BLOCKED_EMERGENCY");
                if (bloquear_tripulantes_por_sabotaje() != EXIT_SUCCESS)
                    log_error(logger, "No se pudo ejecutar la funcion de bloqueo ante sabotajes");
                //while ( !termino_sabotaje() ); //ESPERA ACTIVA o implementar SEMÁFORO.
                //TODO: pasar TRIPULANTE que atendio SABOTAJE de EXEC a BLOCKED_IO
                log_debug(logger, "Termino sabotaje"); 
                desbloquear_tripulantes_tras_sabotaje();
            }

            // Si hay tripulantes que se quedaron sin quantum (transicion EXEC_TO_READY)
            while( existen_tripulantes_en_cola(EXEC_TO_READY) ){
                get_buffer_peticiones_and_swap(EXEC_TO_READY);
            }
                
            // Si hay tripulantes en EXIT
            while( existen_tripulantes_en_cola(EXIT) ){
                gestionar_tripulantes_en_exit();
            }

        }while(false);
        sem_post(&sem_mutex_ejecutar_dispatcher);
    } // FIN WHILE
	liberar_estructuras_planificador();
    return 0;
}

void crear_estructuras_planificador(){
    estado_planificador = PLANIFICADOR_OFF;

    // Inicializo semaforos del dispatcher
	sem_init(&sem_hay_evento_planificable, 0, 0);
    sem_init(&sem_mutex_ingreso_tripulantes_new,0,1);
    sem_init(&sem_mutex_ejecutar_dispatcher,0,0);
	sem_init(&sem_sabotaje_activado,0,0);
	sem_init(&sem_sabotaje_mutex,0,1);
	sem_init(&sem_sabotaje_finalizado,0,0);
	sem_init(&sem_sabotaje_tripulante,0,0);

	crear_colas();

	// Lista de tripulantes (sirve para poder pausar/reanudar la planificacion)
	lista_tripulantes = list_create();

    // Creando hilo del dispatcher
    pthread_create(&hilo_dispatcher, NULL, (void*) dispatcher, NULL);
    pthread_create(&hilo_posteador, NULL, (void*) postear_sem_evento_planificable,NULL);
    pthread_detach(hilo_posteador);
}

void postear_sem_evento_planificable(){
    while(true){
        sleep(retardo_ciclo_cpu);
        sem_post(&sem_hay_evento_planificable);
    }
}


void liberar_estructuras_planificador(){
	//libero recursos:
    log_error(logger, "Desarmando todos los tripulantes de las colas por finalizacion");
    list_destroy_and_destroy_elements(lista_tripulantes, destructor_elementos_tripulante);
    for(int tipo_cola = 0;tipo_cola < CANT_COLAS; tipo_cola++){
        queue_destroy_and_destroy_elements(cola[tipo_cola], destructor_elementos_tripulante);
        sem_destroy(mutex_cola[tipo_cola]);
        free(mutex_cola[tipo_cola]);
    }
    free(cola);
    free(mutex_cola);
}

void finalizar_discordiador(){
    void ejecutar_expulsar_tripulante(void* args){
        t_tripulante* tripulante = (t_tripulante*) args;
        int* TID = malloc(sizeof(int));
	    *TID = tripulante->TID;
        rutina_expulsar_tripulante(TID);
    }

    // Enviamos a EXIT a todos los tripulantes
    list_iterate(lista_tripulantes, ejecutar_expulsar_tripulante);
    status_discordiador = END;  // Cambiamos el estado del discordiador a END
    sem_post(&sem_hay_evento_planificable);
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

int iniciar_dispatcher(){

    if(estado_planificador != PLANIFICADOR_RUNNING){
        log_info(logger, "Iniciando dispatcher");
        estado_planificador = PLANIFICADOR_RUNNING;
        sem_post(&sem_mutex_ejecutar_dispatcher);
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

 void pasar_todos_new_to_ready(){
    int cant_cola = cantidad_tripulantes_en_cola(NEW);
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
    return cantidad_tripulantes_en_cola(tipo_cola) > 0;
}

 int hay_espacio_disponible(int grado_multiprocesamiento){
    return cantidad_tripulantes_en_cola(EXEC) < grado_multiprocesamiento;
}

t_tripulante *buscar_tripulante_por_tid(int tid_buscado){

    bool tiene_TID(void* args){
        t_tripulante* tripulante_encontrado = (t_tripulante*) args;
        return tripulante_encontrado->TID == tid_buscado;
    }

    return list_find(lista_tripulantes, tiene_TID);
}

int rutina_expulsar_tripulante(void* args){
    int tid_buscado = *((int*) args);
    free(args);
    t_tripulante *trip_expulsado;
    int resultado_exit = EXIT_FAILURE;
    int estado_cola;

    sem_wait(&sem_mutex_ejecutar_dispatcher);

    // Busco en cual cola de estado esta el tripulante y lo saco
    for(estado_cola = 0; estado_cola < CANT_ESTADOS; estado_cola++){
        trip_expulsado = desencolar_tripulante_por_tid(cola[estado_cola], tid_buscado);
        if(trip_expulsado != NULL){
            transicion(trip_expulsado, estado_cola, EXIT);
            resultado_exit = EXIT_SUCCESS;
            break;
        }
    }

    // Limpio de los buffers de peticiones al tripulante
    for(estado_cola = CANT_ESTADOS; estado_cola < CANT_COLAS; estado_cola++)
        trip_expulsado = desencolar_tripulante_por_tid(cola[estado_cola], tid_buscado);
	
    if(resultado_exit == EXIT_FAILURE)
        log_error(logger,"El tripulante %d no existe", tid_buscado);

    sem_post(&sem_mutex_ejecutar_dispatcher);
    sem_post(&sem_hay_evento_planificable);
    
    return resultado_exit;
}

 int hay_sabotaje(void){
     if(sabotaje_activo){
         return 1;
     }
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

    // Destruyo todos sus semaforos
    sem_destroy(tripulante->sem_planificacion_fue_reanudada); 
    free(tripulante->sem_planificacion_fue_reanudada);   
    sem_destroy(tripulante->sem_finalizo); 
    free(tripulante->sem_finalizo);      
    sem_destroy(tripulante->sem_puede_usar_dispositivo_io); 
    free(tripulante->sem_puede_usar_dispositivo_io);    

    for(int i = 0;i < CANT_ESTADOS;i++){
        sem_destroy(tripulante->sem_tripulante_dejo[i]); 
        free(tripulante->sem_tripulante_dejo[i]);
    }
    free(tripulante->sem_tripulante_dejo);

    // Lo quito de la lista global de tripulantes y libero el struct
    list_remove_and_destroy_by_condition(lista_tripulantes, tiene_TID_a_eliminar, free);
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
    
    // Copio el estado actual de las colas para recuperarlo tras el sabotaje
    // Paso los tripulantes de ready y exec a blocked emergency
    while(existen_tripulantes_en_cola(EXEC)){
        t_tripulante * temporal = desencolar(EXEC);
        encolar(EXEC_TEMPORAL, temporal);
        transicion(temporal,EXEC,BLOCKED_EMERGENCY);
    }
        
    while(existen_tripulantes_en_cola(READY)){
        t_tripulante * temporal = desencolar(READY);
        encolar(READY_TEMPORAL, temporal);
        transicion(temporal,READY,BLOCKED_EMERGENCY);
    }
    sabotaje_activo=1;
    // Si no hay nadie para atender el sabotaje, falla, y no continua el sabotaje
    if (!existen_tripulantes_en_cola(BLOCKED_EMERGENCY)){
        printf("no existen tripulantes disponibles\n");
        int valor_semaforo;
        sem_getvalue(&sem_tripulante_disponible, &valor_semaforo);
        printf("valor semaforo:%d \n",valor_semaforo);
        sem_wait(&sem_tripulante_disponible);
    }

    // activo el sabotaje, para que los tripulantes cambien su comportamiento en exec y bloqued io
    
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


void desbloquear_tripulantes_tras_sabotaje(void){
    printf("desbloqueando tripulantes\n");
    // Paso los tripulantes de las colas temporales a las comunes. Si estaban pausados, los reanudo
    queue_clean(cola[EXEC]);
    queue_clean(cola[READY]);
    while(existen_tripulantes_en_cola(EXEC_TEMPORAL)){
        int valor_semaforo;
        t_tripulante * temporal = desencolar(EXEC_TEMPORAL);
        desencolar_tripulante_por_tid(cola[BLOCKED_EMERGENCY], temporal->TID);
        transicion(temporal,BLOCKED_EMERGENCY,EXEC);
        sem_getvalue(temporal->sem_planificacion_fue_reanudada, &valor_semaforo);
	    if(valor_semaforo == 0)
			sem_post(temporal->sem_planificacion_fue_reanudada);
    }
    while(existen_tripulantes_en_cola(READY_TEMPORAL)){
        int valor_semaforo;
        t_tripulante * temporal = desencolar(READY_TEMPORAL);
        desencolar_tripulante_por_tid(cola[BLOCKED_EMERGENCY], temporal->TID);
        transicion(temporal,BLOCKED_EMERGENCY,READY);
        sem_getvalue(temporal->sem_planificacion_fue_reanudada, &valor_semaforo);
	    if(valor_semaforo == 0)
			sem_post(temporal->sem_planificacion_fue_reanudada);
    }
    //Esto no tiene que funcionar asi, pero sirve para el test, siempre y cuando haya tripulantes
    //para atender el sabotaje, a parte de los bloqued io
    while(existen_tripulantes_en_cola(BLOCKED_EMERGENCY)){
        int valor_semaforo;
        t_tripulante * temporal = desencolar(BLOCKED_EMERGENCY);
        transicion(temporal,BLOCKED_EMERGENCY,READY);
        sem_getvalue(temporal->sem_planificacion_fue_reanudada, &valor_semaforo);
	    if(valor_semaforo == 0)
			sem_post(temporal->sem_planificacion_fue_reanudada);
    }
    //Limpio la queue de blocked emergency para reutilizarla
    queue_clean(cola[BLOCKED_EMERGENCY]);
}

int inicializar_algoritmo_planificacion(t_config* config){
    if(strcmp("RR", config_get_string_value(config, "ALGORITMO")) == 0){
        algoritmo_planificacion = RR;
        log_info(logger, "El algoritmo de planificacion es: RR");
        quantum = atoi(config_get_string_value(config, "QUANTUM"));	// Variable global
        log_info(logger, "El quantum es: %d", quantum);
        return EXIT_SUCCESS;
    }
    if(strcmp("FIFO", config_get_string_value(config, "ALGORITMO")) == 0){
        algoritmo_planificacion = FIFO;
        log_info(logger, "El algoritmo de planificacion es: FIFO");
        quantum = 0; // Variable global
        return EXIT_SUCCESS;
    }
    log_error(logger, "ERROR. Algoritmo de planificacion: %s. No es valido", config_get_string_value(config, "ALGORITMO"));
    return EXIT_FAILURE;
}

int get_buffer_peticiones_and_swap(enum peticion_transicion tipo_peticion){
    t_tripulante *tripulante;
    while( cantidad_tripulantes_en_cola(tipo_peticion) > 0 ){
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

/*
 void gestionar_exec(int grado_multiprocesamiento){
    t_tripulante *tripulante;
    while( cantidad_tripulantes_en_cola(EXEC) < grado_multiprocesamiento && existen_tripulantes_en_cola(READY) ){
        tripulante = desencolar(READY);
        transicion(tripulante, READY, EXEC);
    }
}*/

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
    nuevo -> sem_puede_usar_dispositivo_io = malloc(sizeof(sem_t));
    sem_init(nuevo -> sem_puede_usar_dispositivo_io, 0, 0);

    nuevo -> sem_tripulante_dejo = malloc(sizeof(sem_t*) * CANT_ESTADOS);
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

t_tripulante* ojear_cola(int tipo_cola){
    if(cantidad_tripulantes_en_cola(tipo_cola) == 0)
        return NULL;
    t_tripulante* tripulante;
    sem_wait(mutex_cola[tipo_cola]);
    tripulante = queue_peek(cola[tipo_cola]);
    sem_post(mutex_cola[tipo_cola]); 
    return tripulante;
}

int cantidad_tripulantes_en_cola(int tipo_cola){
    int cant_tripulantes;
    sem_wait(mutex_cola[tipo_cola]);
    cant_tripulantes = queue_size(cola[tipo_cola]);
    sem_post(mutex_cola[tipo_cola]); 
    return cant_tripulantes;
}