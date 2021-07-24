#include "servidor_i_mongo_store.h"

typedef struct {
    uint32_t PID;
    uint32_t TID;
    uint32_t pos_x;
    uint32_t pos_y;
}t_trip;

t_config_tarea tarea_test;
__thread t_config * tarea_config_file;
__thread t_config_tarea tarea_config;
__thread t_config * bitacora_config_file;
__thread t_config_bitacora bitacora_config;
__thread t_trip tripulante;
//__thread char* path_bitacora;

void liberar_recursos_thread(){
    config_destroy(bitacora_config_file);
    config_destroy(tarea_config_file);
    //free(path_bitacora);
}

int loguear_mensaje(char* payload){
    log_info(logger, "Recibi el mensaje: %s", payload);
    return EXIT_SUCCESS;
}

void iniciar_tripulante(char* payload){
    uint32_t PID, posicion_X, posicion_Y, TID;
    uint32_t offset = 0;
   
    log_info(logger, "Iniciando estructuras del tripulante");

    memcpy(&PID, payload + offset, sizeof(uint32_t));
    //memcpy(&(tripulante.PID), payload + offset, sizeof(uint32_t));
    offset += sizeof(uint32_t);
    log_info(logger, "El PID del tripulante es: %d",PID);    //tripulante.PID);

    memcpy(&TID, payload + offset, sizeof(uint32_t));
    //memcpy(&(tripulante.TID), payload + offset, sizeof(uint32_t));
	offset += sizeof(uint32_t);
    log_info(logger, "El TID del tripulante es: %d",TID);     //tripulante.TID);

    memcpy(&posicion_X, payload + offset, sizeof(uint32_t));
    //memcpy(&(tripulante.pos_x), payload + offset, sizeof(uint32_t));
	offset += sizeof(uint32_t);

    memcpy(&posicion_Y, payload + offset, sizeof(uint32_t));
    //memcpy(&(tripulante.pos_y), payload + offset, sizeof(uint32_t));
	offset += sizeof(uint32_t);
    log_info(logger, "La posicion inicial del tripulante es: (%d,%d)",posicion_X,posicion_Y);      //tripulante.pos_x,tripulante.pos_y);
    //char * mensaje = string_new();
    //string_append(&mensaje, "Se inicio el tripulante ");
    //string_append(&mensaje, string_itoa(tripulante.TID));
    //string_append(&mensaje, "\n");
    tripulante.TID = TID;
    tripulante.PID = PID;
    /*char * path_relativo= string_new();
    string_append(&path_relativo, "/Files/Bitacoras/Tripulante");
    char * tidstr = string_itoa(TID);
    string_append(&path_relativo, tidstr);
    string_append(&path_relativo, ".ims");
    path_bitacora= crear_path_absoluto(path_relativo);
    free(tidstr);*/
    //free(path_relativo);
    //escribir_bitacora(mensaje);
}



void escribir_bitacora(char* mensaje){
    //printf("\nBIT: trip:%d\nBIT: largo de mensaje: %d\nmensaje:%s\n",tripulante.TID, strlen(mensaje),mensaje);
    char * path_relativo = string_from_format("/Files/Bitacoras/Tripulante%d.ims",tripulante.TID);
    char * path_bitacora = crear_path_absoluto(path_relativo);
    char ** bloques_usables;
    int cantidad_de_bloques;
    int offset_bitacora=0;
   //printf("path: %s\n",path_bitacora);
    if(!existe_archivo(path_bitacora)){
        //creando archivo Tripulanten.ims de 0
       //printf("NO existe bitacora\n");
        int size_a_modificar = string_length(mensaje);
        int resto =size_a_modificar%super_bloque.blocksize;
        cantidad_de_bloques=size_a_modificar/super_bloque.blocksize;
        if(resto!=0){
            cantidad_de_bloques++;
        }
       //printf("BIT: cantidad bloques %d\n",cantidad_de_bloques);
        char * bloques_config;
        crear_archivo(path_bitacora);
        bitacora_config_file = config_create(path_bitacora);
        sem_wait(&sem_mutex_bitmap);
        //printf("hice wait\n");
        bloques_usables = bits_libres(&bitmap,cantidad_de_bloques);
        sem_post(&sem_mutex_bitmap);
        //printf("hice post\n");
        //modifica el array para guardarlo como char * en el config
        bloques_config = array_block_to_string(bloques_usables,cantidad_de_bloques);
       //printf("BIT: bloques a config: %s\n", bloques_config);
        
        for(int i = 0; bloques_usables[i]!=NULL;i++){
            sem_wait(&sem_mutex_bitmap);
            ocupar_bit(&bitmap, atoi(bloques_usables[i]));
            sem_post(&sem_mutex_bitmap);
            int offset = atoi(bloques_usables[i])*super_bloque.blocksize;
            if(i==cantidad_de_bloques-1){
                sem_wait(&sem_mutex_blocks);
                memcpy(blocksmap+offset,mensaje+offset_bitacora,resto);
                sem_post(&sem_mutex_blocks);
                offset_bitacora+=resto;
                //printf("ultima posicion escrita bitacora init %d\n",offset);
            }else{
                sem_wait(&sem_mutex_blocks);
                memcpy(blocksmap+offset,mensaje+offset_bitacora,super_bloque.blocksize);
                sem_post(&sem_mutex_blocks);
                offset_bitacora+=super_bloque.blocksize;
            }
        }
        /*sem_wait(&sem_mutex_blocks);
        msync(blocksmap, blocks_stat.st_size, MS_SYNC);
        sem_post(&sem_mutex_blocks);*/
        char * mensajelenstr = string_itoa(strlen(mensaje));
        char * blockcountstr = string_itoa(cantidad_de_bloques);
        config_set_value(bitacora_config_file,"SIZE",mensajelenstr);
        config_set_value(bitacora_config_file,"BLOCK_COUNT",blockcountstr);
        config_set_value(bitacora_config_file,"BLOCKS",bloques_config);
        config_save(bitacora_config_file);
        free(mensajelenstr);
        free(blockcountstr);
        config_destroy(bitacora_config_file);
        free(bloques_config);
        free(path_relativo);
        free(path_bitacora);
        liberar_char_array(bloques_usables);
    }else{
       //printf("existe bitacora\n");
        //Ya existiendo Bitacoran.ims
       //printf("path: %s\n",path_bitacora);
        bitacora_config_file = config_create(path_bitacora);
        leer_bitacora_config(bitacora_config_file);
        char ** lista_bloques = string_get_string_as_array(bitacora_config.blocks);
        char * bloques_config;
        int size_a_modificar = string_length(mensaje);
        int resto_anterior = bitacora_config.size%super_bloque.blocksize;
       //printf("BIT: resto anterior%d\n",resto_anterior);
        int cantidad_repetir;

        //En caso de que el ultimo bloque no este lleno, decide si 
        //hay que usar solo parte de el o
        //utilizar todo el bloque y solicitar otro
        if((super_bloque.blocksize-resto_anterior)>size_a_modificar){
            cantidad_repetir = size_a_modificar;
        }else{
            cantidad_repetir = super_bloque.blocksize-resto_anterior;
        }
        if(resto_anterior!=0){
            char * ultimo_bloque_anterior = lista_bloques[bitacora_config.block_count-1];
           //printf("BIT: bloque numero:%s\n",ultimo_bloque_anterior);
            int ultimo_bloque_offset = atoi(ultimo_bloque_anterior)*super_bloque.blocksize;
            sem_wait(&sem_mutex_blocks);
           //printf("BIT: si quedo resto, empiezo en :%d\n",ultimo_bloque_offset+ resto_anterior);
            memcpy(blocksmap+ultimo_bloque_offset+ resto_anterior,mensaje+offset_bitacora,cantidad_repetir);
            sem_post(&sem_mutex_blocks);
            size_a_modificar-=(cantidad_repetir);
            offset_bitacora+=cantidad_repetir;
        }

        int resto = size_a_modificar%super_bloque.blocksize;
       //printf("BIT: resto actual:%d\n",resto);
        cantidad_de_bloques=size_a_modificar/super_bloque.blocksize;
        if(resto!=0){
            cantidad_de_bloques++;
        }
       //printf("BIT: cantidad bloques %d\n",cantidad_de_bloques);
        
        //solicita una lista de bloques disponibles y los setea en memoria
        if(cantidad_de_bloques>0){
            sem_wait(&sem_mutex_bitmap);
            bloques_usables = bits_libres(&bitmap,cantidad_de_bloques);
            sem_post(&sem_mutex_bitmap);
            bloques_config=array_two_block_to_string(lista_bloques,bloques_usables,bitacora_config.block_count);
           //printf("BIT: bloques a config: %s\n", bloques_config);
            for(int i = 0; bloques_usables[i]!=NULL;i++){
               //printf("Escribo bloque %d",atoi(bloques_usables[i]));
                int offset = atoi(bloques_usables[i])*super_bloque.blocksize;
                if(i==cantidad_de_bloques-1){
                    if(resto==0){
                        resto = super_bloque.blocksize; 
                    }
                    sem_wait(&sem_mutex_blocks);
                    memcpy(blocksmap+offset,mensaje+offset_bitacora,resto);
                    sem_post(&sem_mutex_blocks);
                    offset_bitacora+=resto;
                   //printf("BIT: termino de escribir bitacora post init en %d\n",offset+resto);
                }else{
                    sem_wait(&sem_mutex_blocks);
                    memcpy(blocksmap+offset,mensaje+offset_bitacora,(int)super_bloque.blocksize);
                    sem_post(&sem_mutex_blocks);
                    offset_bitacora+=(int)super_bloque.blocksize;
                }
            }
            liberar_char_array(bloques_usables);
        }else{
            bloques_config = string_new();
            string_append(&bloques_config, bitacora_config.blocks);
           //printf("BIT: bloques a config: %s\n", bloques_config);
        }
        
        /*sem_wait(&sem_mutex_blocks);
        msync(blocksmap, blocks_stat.st_size, MS_SYNC);
        sem_post(&sem_mutex_blocks);*/
        char * bloques_config_final = string_new();
        string_append(&bloques_config_final,bloques_config);
        eliminar_keys_bitacora(bitacora_config_file);
        char * sizestr = string_itoa(bitacora_config.size+string_length(mensaje));
        char * blockcountstr = string_itoa(bitacora_config.block_count+cantidad_de_bloques);
        config_set_value(bitacora_config_file,"SIZE",sizestr);
        config_set_value(bitacora_config_file,"BLOCK_COUNT",blockcountstr);
       //printf("BIT: bloques a config antes de finalizar: %s\n\n", bloques_config_final);
        config_set_value(bitacora_config_file,"BLOCKS",bloques_config_final);
        config_save(bitacora_config_file);
        config_destroy(bitacora_config_file);
        free(sizestr);
        free(blockcountstr);
        liberar_char_array(lista_bloques);
        free(bloques_config_final);
        free(bloques_config);
        free(path_relativo);
        free(path_bitacora);
    }
    
}

char * leer_bitacora(char* payload){
    char * id_tripulante = payload;
    char * path_relativo = string_new();
    char * bitacora;
    
    string_append(&path_relativo,"/Files/Bitacoras/Tripulante");
    string_append(&path_relativo,id_tripulante);
    string_append(&path_relativo,".ims");
    char * path_absoluto = crear_path_absoluto(path_relativo);
   //printf("path: %s\n", path_absoluto);
    if(existe_archivo(path_absoluto)){
        bitacora_config_file=config_create(path_absoluto);
        leer_bitacora_config(bitacora_config_file);
        char ** lista_bloques = string_get_string_as_array(bitacora_config.blocks);
       //printf("bloques de bitacora: %s\n",bitacora_config.blocks);
        bitacora = calloc(bitacora_config.size+1,1);
        int offset_bitacora = 0;
       //printf("size bitacora:%d\n", bitacora_config.size);
        int resto = bitacora_config.size%super_bloque.blocksize;
        for(int i=0; lista_bloques[i]!=NULL; i++){
            if(i == bitacora_config.block_count-1){
                sem_wait(&sem_mutex_blocks);
                memcpy(bitacora+offset_bitacora, blocksmap+(super_bloque.blocksize*atoi(lista_bloques[i])), resto);
               //printf("contenido bitacora final: %s\n",bitacora);
                offset_bitacora+=resto;
                //printf("offset final %d\n",offset_bitacora);
                sem_post(&sem_mutex_blocks);
            }else{
                sem_wait(&sem_mutex_blocks);
                memcpy(bitacora+offset_bitacora, blocksmap+(super_bloque.blocksize*atoi(lista_bloques[i])), super_bloque.blocksize);
               //printf("contenido bitacora: %s\n",bitacora);
                offset_bitacora+=super_bloque.blocksize;
                //printf("offset actual %d\n",offset_bitacora);
                sem_post(&sem_mutex_blocks);    
            }
            
        }
        free(path_relativo);
        liberar_char_array(lista_bloques);
    }
    return bitacora;
}

int movimiento_tripulante(char * payload){
    escribir_bitacora(payload);
    log_info(logger,"Movimiento de tripulante escrito en bitacora");
    return EXIT_SUCCESS;
}

int terminar_tarea(char * payload){
    char * mensaje = string_new();
    string_append(&mensaje,"Se termino la tarea :");
    string_append(&mensaje,payload);
    string_append(&mensaje,"\n");
    log_debug(logger,mensaje);
    escribir_bitacora(mensaje);
    free(mensaje);
    return EXIT_SUCCESS;
}

int recibir_tarea(char* payload){
    
    t_tarea tarea;
    char ** parametros;
    char ** nombre_parametros;
    parametros = string_split(payload,";");
    tarea.nombre = parametros[0];
    tarea.pos_x = parametros[1];
    tarea.pos_y = parametros[2];
    tarea.duracion = parametros[3];
    nombre_parametros = string_split(tarea.nombre," ");
    char * mensaje = string_new();
    string_append(&mensaje,"Se comenzo la tarea :");
    string_append(&mensaje,payload);
    string_append(&mensaje,"\n");
    escribir_bitacora(mensaje);
    liberar_char_array(parametros);
    free(mensaje);
    if(nombre_parametros[1]==NULL){
        //tarea comun sin parametros
        log_debug(logger, "se realizo %s con exito", nombre_parametros[0]);

    }else{
        //se procesa tarea con parametros
       
    
        int codigo_tarea = -1;
        conseguir_codigo_tarea(nombre_parametros[0],&codigo_tarea);
        
        switch(codigo_tarea){
            case GENERAR_OXIGENO:{
                
                char * path = crear_path_absoluto("/Files/Oxigeno.ims");
                char caracter_llenado = 'O';
                sem_wait(&sem_mutex_oxigeno);
                //printf("hice wait soy trip%d\n",tripulante.TID);
                //printf("y no me bloquie %d\n",tripulante.TID);
                generar_recurso(path, caracter_llenado, nombre_parametros[1]);
                //printf("hice post soy trip%d\n",tripulante.TID);
                sem_post(&sem_mutex_oxigeno);
                log_debug(logger, "se genero %s unidades de oxigeno",nombre_parametros[1]);
                free(path);
                break;
            }
            case CONSUMIR_OXIGENO:{
                char * path = crear_path_absoluto("/Files/Oxigeno.ims");
                sem_wait(&sem_mutex_oxigeno);
                consumir_recurso(path,"Oxigeno",nombre_parametros[1]);
                sem_post(&sem_mutex_oxigeno);
                free(path);
                break;
            }
            case GENERAR_COMIDA:{
                char * path = crear_path_absoluto("/Files/Comida.ims");
                char caracter_llenado = 'C';
                sem_wait(&sem_mutex_comida);
                generar_recurso(path, caracter_llenado, nombre_parametros[1]);
                sem_post(&sem_mutex_comida);
                log_debug(logger, "se genero %s unidades de comida",nombre_parametros[1]);
                free(path);
                break;
            }
            case CONSUMIR_COMIDA:{
                char * path = crear_path_absoluto("/Files/Comida.ims");
                sem_wait(&sem_mutex_comida);
                consumir_recurso(path,"Comida",nombre_parametros[1]);
                sem_post(&sem_mutex_comida);
                free(path);
                break;
            }
            case GENERAR_BASURA:{
                char * path = crear_path_absoluto("/Files/Basura.ims");
                char caracter_llenado = 'B';
                sem_wait(&sem_mutex_basura);
                generar_recurso(path, caracter_llenado, nombre_parametros[1]);
                sem_post(&sem_mutex_basura);
                log_debug(logger, "se genero %s unidades de basura",nombre_parametros[1]);
                free(path);
                break;
            }
            case DESCARTAR_BASURA:{
                char * path = crear_path_absoluto("/Files/Basura.ims");
                //printf("leyo el path\n");
                sem_wait(&sem_mutex_basura);
                if(existe_archivo(path)){
                    tarea_config_file = config_create(path);
                    leer_tarea_config(tarea_config_file);
                    char ** lista_bloques = string_get_string_as_array(tarea_config.blocks);
                    //printf("entro al for\n");
                    for(int i = 0; lista_bloques[i]!=NULL;i++){
                        //printf("liberar bit: %d\n",atoi(lista_bloques[i]));
                        sem_wait(&sem_mutex_bitmap);
                        liberar_bit(&bitmap,atoi(lista_bloques[i]));
                        sem_post(&sem_mutex_bitmap);
                    }
                    char * bloques_liberados = string_from_format("Bloques %s liberados",tarea_config.blocks);
                    log_debug(logger,bloques_liberados);
                    free(bloques_liberados);
                    remove(path);
                    liberar_char_array(lista_bloques);
                    config_destroy(tarea_config_file);
                    log_info(logger, "Se descarto la basura");
                }else{
                    log_info(logger, "Se intento descartar basura pero no existe el archivo Basura.ims");
                }
                sem_post(&sem_mutex_basura);
                free(path);
                break;
            }
            default:{
                log_info(logger,"operacion desconocida");
                return EXIT_FAILURE;
                break;
            }
        }
        
    }
    liberar_char_array(nombre_parametros);
    return EXIT_SUCCESS;
}

void leer_tarea_config(t_config * tarea_config_file){
	tarea_config.size = config_get_int_value(tarea_config_file,"SIZE");
	tarea_config.block_count = config_get_int_value(tarea_config_file,"BLOCK_COUNT");
	tarea_config.blocks = config_get_string_value(tarea_config_file, "BLOCKS");
	tarea_config.caracter_llenado = config_get_string_value(tarea_config_file, "CARACTER_LLENADO");
    tarea_config.md5 = config_get_string_value(tarea_config_file, "MD5_ARCHIVO");
}

void leer_bitacora_config(t_config * bitacora_config_file){
    bitacora_config.size = config_get_int_value(bitacora_config_file,"SIZE");
	bitacora_config.block_count = config_get_int_value(bitacora_config_file,"BLOCK_COUNT");
	bitacora_config.blocks = config_get_string_value(bitacora_config_file, "BLOCKS");
}

void conseguir_codigo_tarea(char* nombre, int * codigo){
    if(string_equals_ignore_case(nombre, "GENERAR_OXIGENO")){
        *codigo = GENERAR_OXIGENO;
    }else if(string_equals_ignore_case(nombre, "CONSUMIR_OXIGENO")){
        *codigo = CONSUMIR_OXIGENO;
    }else if(string_equals_ignore_case(nombre, "GENERAR_COMIDA")){
        *codigo = GENERAR_COMIDA;
    }else if(string_equals_ignore_case(nombre, "CONSUMIR_COMIDA")){
        *codigo = CONSUMIR_COMIDA;
    }else if(string_equals_ignore_case(nombre, "GENERAR_BASURA")){
        *codigo = GENERAR_BASURA;
    }else if(string_equals_ignore_case(nombre, "DESCARTAR_BASURA")){
        *codigo = DESCARTAR_BASURA;
    }
};

void crear_archivo (char* path){
    FILE * fp;
    fp = fopen (path, "w");
    fclose(fp);
}

char ** bits_libres (t_bitarray * bitarray, int cantidad_bits) {
    
    char ** lista_bloques_libres;
    int bitoffset = 0;
    char * bloques_libres = string_new();
    int bit_counter = 0;

    while(bit_counter < cantidad_bits){
        char * bitoffstr = string_itoa(bitoffset);

        while(!bitarray_test_bit(bitarray, bitoffset)){

        if(bit_counter==0){
            string_append(&bloques_libres, bitoffstr);
        }else{
            string_append(&bloques_libres, ",");
            string_append(&bloques_libres, bitoffstr);
        }
        //printf("string: %s\n",bloques_libres);
        ocupar_bit(bitarray, bitoffset);
        bit_counter++;
        
        }
        bitoffset++;
        free(bitoffstr);
    }
    
    lista_bloques_libres = string_split(bloques_libres,",");
    free(bloques_libres);
    return lista_bloques_libres;
    
}

void ocupar_bit(t_bitarray *bitarray, int offset){
	bitarray_set_bit(bitarray, offset);
    // copio al void* mapeado el contenido del bitarray para actualizar el bitmap
    int posicion_bitarray = sizeof(super_bloque.blocksize)+ sizeof(super_bloque.blocks);
    sem_wait(&sem_mutex_superbloque);
    memcpy(superbloquemap+posicion_bitarray,super_bloque.bitarray,strlen(super_bloque.bitarray)+1);
    msync(superbloquemap, superbloque_stat.st_size, MS_SYNC);
    sem_post(&sem_mutex_superbloque);
    // fuerzo un sync para que el mapeo se refleje en SuperBloque.ims
    
}

void liberar_bit(t_bitarray *bitarray, int offset){
    /*//printf("bitarray antes de liberar:");
		for(int i = 0; i<super_bloque.blocks; i++){
			int test = bitarray_test_bit(&bitmap, i);
			//printf("%d",test);
		}
		//printf("\n");*/
	bitarray_clean_bit(bitarray, offset);
    //printf("libere el bit =%d\n",offset);
    // copio al void* mapeado el contenido del bitarray para actualizar el bitmap
    int posicion_bitarray = sizeof(super_bloque.blocksize)+ sizeof(super_bloque.blocks);
    sem_wait(&sem_mutex_superbloque);
    memcpy(superbloquemap+posicion_bitarray,super_bloque.bitarray,strlen(super_bloque.bitarray)+1);
    msync(superbloquemap, superbloque_stat.st_size, MS_SYNC);
    sem_post(&sem_mutex_superbloque);
    // fuerzo un sync para que el mapeo se refleje en SuperBloque.ims
    
    /*//printf("bitarray depues de liberar:");
		for(int i = 0; i<super_bloque.blocks; i++){
			int test = bitarray_test_bit(&bitmap, i);
			//printf("%d",test);
		}
		//printf("\n");*/
}

void eliminar_keys_tarea(t_config * tarea_config_file){
    config_remove_key(tarea_config_file,"SIZE");
    config_remove_key(tarea_config_file,"BLOCK_COUNT");
    config_remove_key(tarea_config_file,"BLOCKS");
    config_remove_key(tarea_config_file,"MD5_ARCHIVO");
}

void eliminar_keys_bitacora(t_config * bitacora_config_file){
    config_remove_key(bitacora_config_file,"SIZE");
    config_remove_key(bitacora_config_file,"BLOCK_COUNT");
    config_remove_key(bitacora_config_file,"BLOCKS");
}

void set_tarea_config(t_config * tarea_config_file,char* size, char* cantidad_bloques, char* bloques){
    config_set_value(tarea_config_file,"SIZE",size);
    config_set_value(tarea_config_file,"BLOCK_COUNT",cantidad_bloques);
    config_set_value(tarea_config_file,"BLOCKS",bloques);
}

//une dos char ** y los deja como char * [x,x,x]
char * array_two_block_to_string(char ** bloq_antiguo, char** bloq_nuevo, int config_block_count){
    char * bloques = string_new();
    string_append(&bloques, "[");
    if (bloq_antiguo[0]!=NULL){
        for(int i = 0; bloq_antiguo[i]!=NULL;i++){
            string_append(&bloques,bloq_antiguo[i]);
            string_append(&bloques,",");
        }
        for(int i = 0; bloq_nuevo[i]!=NULL;i++){
            if(bloq_nuevo[i+1]==NULL){
                string_append(&bloques,bloq_nuevo[i]);
            }else{
                string_append(&bloques,bloq_nuevo[i]);
                string_append(&bloques,",");
            }
        }
        /*
        //if(bloq_nuevo[0]!="0"&&atoi(bloq_nuevo[0])!=0){
        if(!strcmp(bloq_nuevo[0],"0")&&atoi(bloq_nuevo[0])!=0){
           //printf("CHAU\n");
            for(int i = 0; bloq_nuevo[i]!=NULL;i++){
            string_append(&bloques,",");
            string_append(&bloques,bloq_nuevo[i]);
            }
        //}else if (bloq_nuevo[0]=="0"){
        }else if (strcmp(bloq_nuevo[0],"0")){
           //printf("HOLA\n");
            for(int i = 0; bloq_nuevo[i]!=NULL;i++){
            //printf("entre bloqnuevo\n");
            string_append(&bloques,",");
            string_append(&bloques,bloq_nuevo[i]);
            }
        }*/
    }else{
        for(int i = 0; bloq_nuevo[i]!=NULL;i++){
            if(i==0){
                string_append(&bloques,bloq_nuevo[i]);
            }else{
                string_append(&bloques,",");
                string_append(&bloques,bloq_nuevo[i]);
            }
        }
    }
    //printf("saliendooo\n");
    string_append(&bloques, "]");
    return bloques;
}

//indice para "restar" bloques, util para funciones Consumir
char * array_block_to_string(char ** bloq_array, int indice){
    char * bloques = string_new();
    //printf("entre en arraytoblock con indice %d\n",indice);
    string_append(&bloques, "[");
    for(int i = 0; i<indice ;i++){
        if(i==indice-1){
            string_append(&bloques,bloq_array[i]);
        }else{
            string_append(&bloques,bloq_array[i]);
            string_append(&bloques,",");
        }
    }
    string_append(&bloques, "]");
    return bloques;
}

void generar_recurso(char* path, char caracter_llenado,char* parametro){
    char ** bloques_usables;
    int cantidad_de_bloques;
    if(!existe_archivo(path)){
        //creando archivo Oxigeno.ims de 0
        
        int resto = atoi(parametro)%super_bloque.blocksize;
        cantidad_de_bloques=atoi(parametro)/super_bloque.blocksize;
        if(resto!=0){
            cantidad_de_bloques++;
        }
        //printf("cantidad bloques %d\n",cantidad_de_bloques);
        char * bloques_config;
        //string_append(&bloques_config,"[");
        crear_archivo(path);
        tarea_config_file = config_create(path);
        sem_wait(&sem_mutex_bitmap);
        bloques_usables = bits_libres(&bitmap,cantidad_de_bloques);
        sem_post(&sem_mutex_bitmap);
        //modifica el array para guardarlo como char * en el config
        bloques_config = array_block_to_string(bloques_usables,cantidad_de_bloques);
        //printf("bloques recurso %c a config: %s\n", caracter_llenado, bloques_config);

        //char para md5 del ultimo bloque modificado
        char * md5_str = calloc(1,super_bloque.blocksize+1);
        for(int i = 0; bloques_usables[i]!=NULL;i++){
            int offset = atoi(bloques_usables[i])*super_bloque.blocksize;
            //char *test = bloques_usables[i];
            //printf("bloque usable : %s\noffset generar recurso : %d\n",test,offset);
            if(i==cantidad_de_bloques-1){
                char * fill = string_repeat(caracter_llenado,resto);
                sem_wait(&sem_mutex_blocks);
                //printf(" no existe y termina offset:%d\n",offset);
                memcpy(md5_str,fill,resto);
                memcpy(blocksmap+offset,fill,resto);
                sem_post(&sem_mutex_blocks);
                free(fill);
            }else{
                
                char * fill = string_repeat(caracter_llenado,(int)super_bloque.blocksize);
                sem_wait(&sem_mutex_blocks);
                //printf("no existe y empieza offset:%d\n",offset);
                memcpy(blocksmap+offset,fill,super_bloque.blocksize);
                sem_post(&sem_mutex_blocks);
                free(fill);
            }
        }
        
        /*sem_wait(&sem_mutex_blocks);
        msync(blocksmap, blocks_stat.st_size, MS_SYNC);
        sem_post(&sem_mutex_blocks);*/
        char * caracter = calloc(sizeof(char),1);
        *caracter = caracter_llenado;
        config_set_value(tarea_config_file,"CARACTER_LLENADO",caracter);
        char * cantidadbloquesstr = string_itoa(cantidad_de_bloques);
        set_tarea_config(
            tarea_config_file,
            parametro,
            cantidadbloquesstr,
            bloques_config
        );
        config_save(tarea_config_file);
        //printf("hola\n");
        char * md5 = md5_create(md5_str);
        config_set_value(tarea_config_file,"MD5_ARCHIVO",md5);
        //printf("el md5 es: %s \n",md5);
        config_save(tarea_config_file);
        config_destroy(tarea_config_file);
        free(md5_str);
        free(md5);
        free(caracter);
        free(cantidadbloquesstr);
        liberar_char_array(bloques_usables);
        free(bloques_config);
        //printf("caracter llenado: %s\n",config_get_string_value(tarea_config_file,"CARACTER_LLENADO"));
    }else{
        //Ya existiendo Recurso.ims
        tarea_config_file = config_create(path);
        leer_tarea_config(tarea_config_file);
        char * caracter_llenado = tarea_config.caracter_llenado;
        //printf("bloques en config:%s\n",tarea_config.blocks);
        char ** lista_bloques = string_get_string_as_array(tarea_config.blocks);
        char * bloques_config;
        int size_a_modificar = atoi(parametro);

        int resto_anterior = tarea_config.size%super_bloque.blocksize;
        //printf("resto anterior%d\n",resto_anterior);
        int cantidad_repetir;

        //En caso de que el ultimo bloque no este lleno, decide si 
        //hay que usar solo parte de el o
        //utilizar todo el bloque y solicitar otro
        if((super_bloque.blocksize-resto_anterior)>size_a_modificar){
            cantidad_repetir = size_a_modificar;
        }else{
            cantidad_repetir = super_bloque.blocksize-resto_anterior;
        }
        
        if(resto_anterior!=0){
            char * fill = string_repeat(*caracter_llenado,cantidad_repetir);
            char * ultimo_bloque_anterior = lista_bloques[tarea_config.block_count-1];
            //printf("bloque numero:%s\n",ultimo_bloque_anterior);
            int ultimo_bloque_offset = atoi(ultimo_bloque_anterior)*super_bloque.blocksize;
            sem_wait(&sem_mutex_blocks);
            //printf("existe y hay resto anterior offset:%d\n",ultimo_bloque_offset+ resto_anterior);
            memcpy(blocksmap+ultimo_bloque_offset+ resto_anterior,fill,cantidad_repetir);
            sem_post(&sem_mutex_blocks);
            size_a_modificar-=(cantidad_repetir);
            free(fill);
        }

        int resto = size_a_modificar%super_bloque.blocksize;
        //printf("resto actual:%d\n",resto);
        cantidad_de_bloques=size_a_modificar/super_bloque.blocksize;
        if(resto!=0){
            cantidad_de_bloques++;
        }
        //printf("cantidad bloques %d\n",cantidad_de_bloques);
        
        if(cantidad_de_bloques>0){
            sem_wait(&sem_mutex_bitmap);
            //solicita una lista de bloques disponibles y los setea en memoria
            bloques_usables = bits_libres(&bitmap,cantidad_de_bloques);
            sem_post(&sem_mutex_bitmap);
            //printf("bloque usable 1: %s  2:%s\n ", bloques_usables[0], bloques_usables[1]);
            //printf("bloque anterior: %s\n ", lista_bloques[0]);
            //une la lista de bloques del config con los bloques nuevos a usar

            bloques_config=array_two_block_to_string(lista_bloques,bloques_usables,tarea_config.block_count);
            //printf("bloques recurso %s a config: %s\n", caracter_llenado, bloques_config);

            
            for(int i = 0; bloques_usables[i]!=NULL;i++){
                //printf("bloque usable:%s\n",bloques_usables[i]);
                int offset = atoi(bloques_usables[i])*super_bloque.blocksize;
                if(i==cantidad_de_bloques-1){
                    char * fill = string_repeat(*caracter_llenado,resto);
                    sem_wait(&sem_mutex_blocks);
                    //printf("existe y termina offset:%d\n",offset);
                    memcpy(blocksmap+offset,fill,strlen(fill));
                    sem_post(&sem_mutex_blocks);
                    free(fill);
                }else{
                    char * fill = string_repeat(*caracter_llenado,(int)super_bloque.blocksize);
                    sem_wait(&sem_mutex_blocks);
                    //printf("existe y empieza bloque nuevo offset:%d\n",offset);
                    memcpy(blocksmap+offset,fill,strlen(fill));
                    sem_post(&sem_mutex_blocks);
                    free(fill);
                }
            }    
            liberar_char_array(bloques_usables);
        }else{
            bloques_config = string_new();
            string_append(&bloques_config, tarea_config.blocks);
            //printf("bloques recurso %s a config: %s\n", caracter_llenado, bloques_config);
        }
        /*sem_wait(&sem_mutex_blocks);
        msync(blocksmap, blocks_stat.st_size, MS_SYNC);
        sem_post(&sem_mutex_blocks);*/

        char * buffer_temporal = calloc(1,super_bloque.blocksize+1);
        char * md5_str = calloc(1,super_bloque.blocksize+1);
        int indice_md5=0;
        char ** lista_bloques_temp = string_get_string_as_array(bloques_config);
        int indice_ultimo_bloque = atoi(lista_bloques_temp[tarea_config.block_count+cantidad_de_bloques-1]);
        memcpy(buffer_temporal,blocksmap+(indice_ultimo_bloque*super_bloque.blocksize),super_bloque.blocksize);

        for(int j= 0;j<super_bloque.blocksize;j++){
            if(buffer_temporal[j]==caracter_llenado[0]){
                //printf("caracter leido:%c y caracter config:%c\n",buffer_temporal[j],caracter_config[0]);
                md5_str[indice_md5]=buffer_temporal[j];
                indice_md5++;
            }
        }

        eliminar_keys_tarea(tarea_config_file);
        //printf("bloques recurso %s a config: %s\n", caracter_llenado, bloques_config);
        char * sizestr = string_itoa(tarea_config.size + atoi(parametro));
        char * blockcountstr = string_itoa(tarea_config.block_count+cantidad_de_bloques);
        set_tarea_config(
            tarea_config_file,
            sizestr,
            blockcountstr,
            bloques_config
        );
        config_save(tarea_config_file);
        char * md5 = md5_create(md5_str);
        config_set_value(tarea_config_file,"MD5_ARCHIVO",md5);
        //printf("el md5 es: %s \n",md5);
        config_save(tarea_config_file);
        config_destroy(tarea_config_file);
        free(buffer_temporal);
        free(md5_str);
        free(md5);
        free(sizestr);
        free(blockcountstr);
        liberar_char_array(lista_bloques);
        liberar_char_array(lista_bloques_temp);
        free(bloques_config);
    }
}

void consumir_recurso(char* path, char* nombre_recurso ,char* parametro){
    //printf("empeze a consumir un recurso\n");
    tarea_config_file = config_create(path);
    //printf("empeze a consumir un recurso\n");
    leer_tarea_config(tarea_config_file);
    //printf("tarea config blocks%s\n",tarea_config.blocks);
    char ** lista_bloques = string_get_string_as_array(tarea_config.blocks);
    //printf("empeze a consumir un recurso\n");
    if(existe_archivo(path)){
        //printf("empeze a consumir un recurso\n");
        int size_a_modificar = atoi(parametro);
        //printf("entro 2do if\n");
        if(tarea_config.size >= size_a_modificar){
            //todabia quedan caracteres
            int elem_restantes = tarea_config.size - size_a_modificar;

            int indice_restantes = elem_restantes/super_bloque.blocksize;
            int elem_restantes_resto = elem_restantes%super_bloque.blocksize;
            int bloques_a_modificar = tarea_config.block_count - indice_restantes;
            int cantidad_repetir;
            int block_config;
            if(elem_restantes_resto !=0){
                block_config = indice_restantes +1;
            }else{
                block_config = indice_restantes;
            }
            if (bloques_a_modificar>0){
                cantidad_repetir = size_a_modificar;
            }else{
                cantidad_repetir = super_bloque.blocksize-elem_restantes_resto;
            }

            if(elem_restantes_resto!=0){
                char * fill = string_repeat(' ',cantidad_repetir);
                int bloque_offset = atoi(lista_bloques[indice_restantes]) *super_bloque.blocksize;
                sem_wait(&sem_mutex_blocks);
                memcpy(blocksmap+bloque_offset+elem_restantes_resto,fill,strlen(fill));
                sem_post(&sem_mutex_blocks);
                //size_a_modificar-=(cantidad_repetir);
                indice_restantes++;
                free(fill);
            }
           //printf("entro for bloques mod\n");
            for (int i = indice_restantes; lista_bloques[i]!=NULL; i++){
               //printf("entro for bloques mod\n");
                char * fill = string_repeat(' ',super_bloque.blocksize);
                int offset = super_bloque.blocksize*atoi(lista_bloques[i]);
                sem_wait(&sem_mutex_blocks);
                memcpy(blocksmap+offset,fill,strlen(fill));
                sem_post(&sem_mutex_blocks);
               //printf("entro BITMAP\n");
                sem_wait(&sem_mutex_bitmap);
                char * bloques_liberados = string_from_format("Bloque %s liberado por consumir %s",lista_bloques[i],nombre_recurso);
                log_debug(logger,bloques_liberados);
                free(bloques_liberados);
                liberar_bit(&bitmap,atoi(lista_bloques[i]));
                sem_post(&sem_mutex_bitmap);
                free(fill);
            }
           //printf("sali del for \n");
            char * bloques_config = array_block_to_string(lista_bloques,block_config);
            /*sem_wait(&sem_mutex_blocks);
            msync(blocksmap, blocks_stat.st_size, MS_SYNC);
            sem_post(&sem_mutex_blocks);*/

           //printf("config\n");
            eliminar_keys_tarea(tarea_config_file);
            char * sizestr = string_itoa(tarea_config.size - atoi(parametro));
            char * blockcountstr = string_itoa(block_config);
            set_tarea_config(
                tarea_config_file,
                sizestr,
                blockcountstr,
                bloques_config
            );


            char * buffer_temporal = calloc(1,super_bloque.blocksize+1);
            char * md5_str = calloc(1,super_bloque.blocksize+1);
            int indice_md5=0;
            char ** lista_bloques_temp = string_get_string_as_array(bloques_config);
            int indice_ultimo_bloque = atoi(lista_bloques_temp[block_config-1]);
            memcpy(buffer_temporal,blocksmap+(indice_ultimo_bloque*super_bloque.blocksize),super_bloque.blocksize);

            for(int j= 0;j<super_bloque.blocksize;j++){
                if(buffer_temporal[j]==tarea_config.caracter_llenado[0]){
                    //printf("caracter leido:%c y caracter config:%c\n",buffer_temporal[j],caracter_config[0]);
                    md5_str[indice_md5]=buffer_temporal[j];
                    indice_md5++;
                }
            }
            char * md5 = md5_create(md5_str);
            config_set_value(tarea_config_file,"MD5_ARCHIVO",md5);
           //printf("el md5 es: %s \n",md5);
            config_save(tarea_config_file);
            config_destroy(tarea_config_file);
            free(md5_str);
            free(md5);
            free(sizestr);
            free(blockcountstr);
            liberar_char_array(lista_bloques);
            free(bloques_config);
            log_info(logger, "se consumio %s unidades de %s\n",parametro, nombre_recurso);
        }else{
            //borrar archivo entero
            for (int i = 0; lista_bloques[i]!=NULL; i++){
                //printf("entro for bloques mod\n");
                char * fill = string_repeat(' ',super_bloque.blocksize);
                int offset = super_bloque.blocksize*atoi(lista_bloques[i]);
                sem_wait(&sem_mutex_blocks);
                memcpy(blocksmap+offset,fill,strlen(fill));
                sem_post(&sem_mutex_blocks);
                //printf("entro BITMAP\n");
                sem_wait(&sem_mutex_bitmap);
                liberar_bit(&bitmap,atoi(lista_bloques[i]));
                sem_post(&sem_mutex_bitmap);
                free(fill);
            }
            char * bloques_liberados = string_from_format("Bloques %s liberados",tarea_config.blocks);
            log_debug(logger,bloques_liberados);
            free(bloques_liberados);

            //printf("sali del for \n");
            char * bloques_config = array_block_to_string(lista_bloques,0);

            char * buffer_temporal = calloc(1,super_bloque.blocksize+1);
            char * md5_str = calloc(1,super_bloque.blocksize+1);
            int indice_md5=0;
            int indice_ultimo_bloque = atoi(lista_bloques[0]);
            memcpy(buffer_temporal,blocksmap+(indice_ultimo_bloque*super_bloque.blocksize),super_bloque.blocksize);

            for(int j= 0;j<super_bloque.blocksize;j++){
                if(buffer_temporal[j]==tarea_config.caracter_llenado[0]){
                    //printf("caracter leido:%c y caracter config:%c\n",buffer_temporal[j],caracter_config[0]);
                    md5_str[indice_md5]=buffer_temporal[j];
                    indice_md5++;
                }
            }



            eliminar_keys_tarea(tarea_config_file);
            char * cerostr = string_itoa(0);
            set_tarea_config(
                tarea_config_file,
                cerostr,
                cerostr,
                bloques_config
            );
            char * md5 = md5_create(md5_str);
            config_set_value(tarea_config_file,"MD5_ARCHIVO",md5);
           //printf("el md5 es: %s \n",md5);
            config_save(tarea_config_file);
            config_destroy(tarea_config_file);
            free(md5_str);
            free(md5);
            free(cerostr);
            free(bloques_config);
            liberar_char_array(lista_bloques);
            log_info(logger, "se intento consumir mas unidades de %s que las disponibles",nombre_recurso);
        }
        
    }else{
        log_info(logger,"No se encontro archivo %s.ims",nombre_recurso);
        liberar_char_array(lista_bloques);
    }
}

void liberar_char_array (char** array){
    for(int i = 0; array[i]!= NULL; i++){
        free(array[i]);
    }
    free(array);
}


char * md5_create (char * datos){
    char * command = string_from_format("echo -n %s | md5sum",datos);
    FILE *ls_cmd = popen(command, "r");
    if (ls_cmd == NULL) {
        fprintf(stderr, "popen(3) error");
        exit(EXIT_FAILURE);
    }

    free(command);

    static char buff[1024];
    char * md5temp = string_new();
    size_t n;

    while ((n = fread(buff, 1, sizeof(buff)-1, ls_cmd)) > 0) {
        buff[n] = '\0';
        string_append(&md5temp,buff);
    }
    char ** md5arr = string_split(md5temp," ");
    char * md5 = string_from_format("%s",md5arr[0]);
    liberar_char_array(md5arr);
    free(md5temp);
    if (pclose(ls_cmd) < 0)
        perror("pclose(3) error");

    return md5;

}

void testear_md5(){
    fsck();
}



void fsck(){
    t_superbloque st_sb_fsck;
    char * puntomontaje = crear_path_absoluto("/SuperBloque.ims");
    int offset = 0;
    int sbfile_fsck;
    log_debug(logger,"Ejecutando protocolo fsck..");
    //abro el superbloque.ims
    sbfile_fsck = open(puntomontaje, O_RDWR, S_IRUSR | S_IWUSR);
    //void * map_sb_fsck = mmap (NULL, superbloque_stat.st_size, PROT_READ | PROT_WRITE ,MAP_SHARED, sbfile_fsck, 0 );
    superbloquemap = mmap (NULL, superbloque_stat.st_size, PROT_READ | PROT_WRITE ,MAP_SHARED, sbfile_fsck, 0 );
    //tamaño de bloques
    memcpy(&st_sb_fsck.blocksize,superbloquemap,sizeof(st_sb_fsck.blocksize));
    offset = offset + sizeof(st_sb_fsck.blocksize);
    //cantidad de bloques(posiblemente modificado)
    memcpy(&st_sb_fsck.blocks,superbloquemap+offset,sizeof(st_sb_fsck.blocks));
    offset = offset + sizeof(st_sb_fsck.blocks);
    st_sb_fsck.bitarray = calloc(st_sb_fsck.blocks/8+1, sizeof(char*));
	memcpy(st_sb_fsck.bitarray,superbloquemap+offset,st_sb_fsck.blocks/8+1);
    t_bitarray bitmap_fsck = *bitarray_create_with_mode(st_sb_fsck.bitarray,st_sb_fsck.blocks/8,LSB_FIRST);

   //printf("cantidad de bloques:%d\n", st_sb_fsck.blocks);
    int cantidad_bloques = blocks_stat.st_size/st_sb_fsck.blocksize;
   //printf("cantidad de bloques:%d\n", cantidad_bloques);

    //detecto sabotaje a cantidad de blocks de Superbloque.ims
    if(cantidad_bloques!=st_sb_fsck.blocks){
        log_error(logger, "Cantidad de bloques inconsistente en Superbloque.ims");
        memcpy(superbloquemap+sizeof(super_bloque.blocksize),&cantidad_bloques,sizeof(super_bloque.blocks));
        msync(superbloquemap, superbloque_stat.st_size, MS_SYNC);
        log_info(logger,"Sabotaje corregido con exito");
    }

    char * path_oxigeno = crear_path_absoluto("/Files/Oxigeno.ims");
    char * path_comida = crear_path_absoluto("/Files/Comida.ims");
    char * path_basura = crear_path_absoluto("/Files/Basura.ims");
   //printf("test de bitarray:\n");
    //detecto sabotaje al bitmap
    if(strcmp(bitmap.bitarray,bitmap_fsck.bitarray)!=0){
        log_error(logger, "Bitarray inconsistente en Superbloque.ims");
        /*printf("test de bitarray viejo:");
		for(int i = 0; i<super_bloque.blocks; i++){
			int test = bitarray_test_bit(&bitmap, i);
			printf("%d",test);
		}
		printf("\n");
       //printf("test de bitarray nuevo:");
		for(int i = 0; i<super_bloque.blocks; i++){
			int test = bitarray_test_bit(&bitmap_fsck, i);
			printf("%d",test);
		}
		printf("\n");
       //printf("bitarrays distintos detectados\n");*/
        int cantidad_de_bloques = 0;
        int primer_modificacion = 0;
        
        char * path_bitacora = crear_path_absoluto("/Files/Bitacoras");
        char * bloques_ocupados = string_new();
        //consultar archivo oxigeno.ims
        if(existe_archivo(path_oxigeno)){
            t_config * config_oxigeno;
            config_oxigeno = config_create(path_oxigeno);
            char * bloques = config_get_string_value(config_oxigeno, "BLOCKS");
            int cantidad = config_get_int_value(config_oxigeno, "BLOCK_COUNT");
            cantidad_de_bloques+=cantidad;
            if(primer_modificacion == 0){
                string_append(&bloques_ocupados,bloques);
                primer_modificacion = 1;
            }
           //printf("bloques detectados en oxigeno: %s\n",bloques_ocupados);
            char * log = string_from_format("Bloques en uso: %s, despues de revisar Oxigeno.ims",bloques_ocupados);
            log_info(logger,log);
        }
        //consultar archivo comida.ims
        if(existe_archivo(path_comida)){
            t_config * config_comida;
            config_comida = config_create(path_comida);
            char * bloques = config_get_string_value(config_comida, "BLOCKS");
            int cantidad = config_get_int_value(config_comida, "BLOCK_COUNT");
            if(primer_modificacion == 0){
                string_append(&bloques_ocupados,bloques);
                primer_modificacion = 1;
            }else{
                char ** bloques_viejos = string_get_string_as_array(bloques_ocupados);
                char ** bloques_nuevos = string_get_string_as_array(bloques);
                free(bloques_ocupados);
                bloques_ocupados = array_two_block_to_string(bloques_viejos,bloques_nuevos,cantidad_de_bloques);                
            }
            cantidad_de_bloques+=cantidad;
           //printf("bloques detectados en comida: %s\n",bloques_ocupados);
            char * log = string_from_format("Bloques en uso: %s, despues de revisar Comida.ims",bloques_ocupados);
            log_info(logger,log);
        }
        //consultar archivo basura.ims
        if(existe_archivo(path_basura)){
            t_config * config_basura;
            config_basura = config_create(path_basura);
            char * bloques = config_get_string_value(config_basura, "BLOCKS");
            int cantidad = config_get_int_value(config_basura, "BLOCK_COUNT");
            if(primer_modificacion == 0){
                string_append(&bloques_ocupados,bloques);
                primer_modificacion = 1;
            }else{
                char ** bloques_viejos = string_get_string_as_array(bloques_ocupados);
                char ** bloques_nuevos = string_get_string_as_array(bloques);
                free(bloques_ocupados);
                bloques_ocupados = array_two_block_to_string(bloques_viejos,bloques_nuevos,cantidad_de_bloques);                
            }
            cantidad_de_bloques+=cantidad;
           //printf("bloques detectados en basura: %s\n",bloques_ocupados);
            char * log = string_from_format("Bloques en uso: %s, despues de revisar Basura.ims",bloques_ocupados);
            log_info(logger,log);
        }
        int indice_tripulante = 1;
        char * path_tripulante = string_from_format("%s/Tripulante%d.ims",path_bitacora,indice_tripulante);
        //consultar archivo tripulante.ims
        while(existe_archivo(path_tripulante)){
            t_config * config_tripulante;
            config_tripulante = config_create(path_tripulante);
            char * bloques = config_get_string_value(config_tripulante, "BLOCKS");
            int cantidad = config_get_int_value(config_tripulante, "BLOCK_COUNT");
            if(primer_modificacion == 0){
                string_append(&bloques_ocupados,bloques);
                primer_modificacion = 1;
            }else{
                char ** bloques_viejos = string_get_string_as_array(bloques_ocupados);
                char ** bloques_nuevos = string_get_string_as_array(bloques);
                free(bloques_ocupados);
                bloques_ocupados = array_two_block_to_string(bloques_viejos,bloques_nuevos,cantidad_de_bloques);                
            }
           //printf("bloques detectados en tripulante%d: %s\n",indice_tripulante,bloques_ocupados);
            char * log = string_from_format("Bloques en uso: %s, despues de revisar Tripulante%d.ims",bloques_ocupados,indice_tripulante);
            log_info(logger,log);
            cantidad_de_bloques+=cantidad;
            free(path_tripulante);
            indice_tripulante++;
            path_tripulante = string_from_format("%s/Tripulante%d.ims",path_bitacora,indice_tripulante);
           //printf("siguiente trip es:%s\n",path_tripulante);
        }
        

        if(cantidad_de_bloques>0){
            char ** array_bloques_ocupados = string_get_string_as_array(bloques_ocupados);
            //limpio el bitmap
            for(int i = 0; i<super_bloque.blocks; i++){
                bitarray_clean_bit(&bitmap, i);
            }
            //ocupo bitmap segun los blocks recolectados
            for(int i = 0;array_bloques_ocupados[i]!=NULL;i++){
                int indice = atoi(array_bloques_ocupados[i]);
                bitarray_set_bit(&bitmap,indice);
            }
            /*printf("test de bitarray viejo:");
            for(int i = 0; i<super_bloque.blocks; i++){
                int test = bitarray_test_bit(&bitmap, i);
               //printf("%d",test);
            }
           //printf("\n");*/

            //mapeo a memoria y bajo al archivo
            memcpy(superbloquemap+sizeof(super_bloque.blocksize)+sizeof(super_bloque.blocks),
                super_bloque.bitarray,
                strlen(super_bloque.bitarray)+1);
            msync(superbloquemap, superbloque_stat.st_size, MS_SYNC);
        }
        
        log_info(logger,"Sabotaje corregido con exito");
    }

    if(existe_archivo(path_oxigeno)){
        testear_files(path_oxigeno);
    }
    if(existe_archivo(path_comida)){
        testear_files(path_comida);
    }
    if(existe_archivo(path_basura)){
        testear_files(path_basura);
    }

}

void testear_files(char * path){
    //variables para testear los Files
    t_config * test;
    test = config_create(path);
    char * bloques_config = config_get_string_value(test,"BLOCKS");
    int cantidad_bloques_config = config_get_int_value(test,"BLOCK_COUNT");
   //printf("block count: %d\n",cantidad_bloques_config);
    int size_config = config_get_int_value(test,"SIZE");
    char * md5_config = config_get_string_value(test,"MD5_ARCHIVO");
    char * caracter_config = config_get_string_value(test,"CARACTER_LLENADO");

   //printf("md5 viejo = %s\n",md5_config);

    //comparo Block_count y Blocks, para validar Block_count y poder usarlo para verificar el size
    int cantidad_real_bloques = 0;
    
    //para el size del archivo
    int size_archivo = 0;
    char * buffer_temporal = calloc(1,super_bloque.blocksize+1);

    char ** lista_bloques = string_get_string_as_array(bloques_config);
    for(int i =0;lista_bloques[i]!=NULL;i++){
        cantidad_real_bloques++;
        int indice = atoi(lista_bloques[i]);
        memcpy(buffer_temporal,
            blocksmap+(indice*super_bloque.blocksize),
            super_bloque.blocksize);
        //calculo el size comparando si esta escrito el caracter de llenado
        for(int j= 0;j<super_bloque.blocksize;j++){
            if(buffer_temporal[j]==caracter_config[0]){
                //printf("caracter leido:%c y caracter config:%c\n",buffer_temporal[j],caracter_config[0]);
                size_archivo++;
            }
        }
       //printf("size parcial:%d\n",size_archivo);
    }
   //printf("empiezo comprobaciones\n");
   //printf("cantidad real block count:%d\n",cantidad_real_bloques);
    if(cantidad_real_bloques!=  cantidad_bloques_config){
        log_error(logger,"Inconsistencia en el valor de BLOCK_COUNT");
        char * block_count = string_itoa(cantidad_real_bloques);
        config_remove_key(test,"BLOCK_COUNT");
        config_set_value(test,"BLOCK_COUNT",block_count);
        config_save(test);
        log_info(logger,"Sabotaje reparado con exito");
    }
    if(size_config!=size_archivo){
        log_error(logger,"Inconsistencia en el valor de SIZE");
        char * size = string_itoa(size_archivo);
        config_remove_key(test,"SIZE");
        config_set_value(test,"SIZE",size);
        config_save(test);
        log_info(logger,"Sabotaje reparado con exito");
    }
   //printf("termine comprobaciones\n");
    //free(buffer_temporal);

    
    char * md5_fsck = calloc(1,super_bloque.blocksize+1);
    int indice_md5=0;
    int indice_ultimo_bloque = atoi(lista_bloques[cantidad_real_bloques-1]);
    memcpy(buffer_temporal,blocksmap+(indice_ultimo_bloque*super_bloque.blocksize),super_bloque.blocksize);

    for(int j= 0;j<super_bloque.blocksize;j++){
        if(buffer_temporal[j]==caracter_config[0]){
            //printf("caracter leido:%c y caracter config:%c\n",buffer_temporal[j],caracter_config[0]);
            md5_fsck[indice_md5]=buffer_temporal[j];
            indice_md5++;
        }
    }

    char * md5nuevo = md5_create(md5_fsck);
    for(int i=0;i<strlen(md5_fsck);i++){
        //printf("char leido %c\n",md5_fsck[i]);
    }
   //printf("md5 nuevo = %s\n", md5nuevo);
    //detecto problema en ultimo bloque del Blocks del File
    if(strcmp(md5_config,md5nuevo)!=0){
        log_error(logger,"Inconsistencia en el orden de blocks");
        int resto = size_archivo%super_bloque.blocksize;
        for(int i =0;lista_bloques[i]!=NULL;i++){
            int offset = atoi(lista_bloques[i])*super_bloque.blocksize;
            if(lista_bloques[i+1]==NULL){
                char * fill_space = string_repeat(' ',(int)super_bloque.blocksize);
                memcpy(blocksmap+offset,fill_space,strlen(fill_space));
                char * fill = string_repeat(*caracter_config,resto);
                memcpy(blocksmap+offset,fill,strlen(fill));
                free(fill);
            }else{
                char * fill = string_repeat(*caracter_config,(int)super_bloque.blocksize);
                memcpy(blocksmap+offset,fill,strlen(fill));
                free(fill);
            }
        }
        log_info(logger,"Sabotaje reparado con exito");
    }
    config_destroy(test);
}