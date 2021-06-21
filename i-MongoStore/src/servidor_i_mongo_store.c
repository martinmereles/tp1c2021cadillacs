#include "servidor_i_mongo_store.h"
t_config * tarea_config_file;
t_config_tarea tarea_config;

void loguear_mensaje(char* payload){
    log_info(logger, "Recibi el mensaje: %s", payload);
}

void recibir_tarea(char* payload){
    t_tarea tarea;
    char ** parametros;
    char ** nombre_parametros;
    parametros = string_split(payload,";");
    tarea.nombre = parametros[0];
    tarea.pos_x = parametros[1];
    tarea.pos_y = parametros[2];
    tarea.duracion = parametros[3];
    nombre_parametros = string_split(tarea.nombre," ");
    if(nombre_parametros[1]==NULL){
        //tarea comun sin parametros
        log_info(logger, "se realizo %s con exito", nombre_parametros[0]);
    }else{
        //se procesa tarea con parametros
        
        int codigo_tarea = -1;
        conseguir_codigo_tarea(nombre_parametros[0],&codigo_tarea);
        
        switch(codigo_tarea){
            case GENERAR_OXIGENO:{
                char * path = crear_path_absoluto("/Files/Oxigeno.ims");
                char caracter_llenado = 'O';
                generar_recurso(path, caracter_llenado, nombre_parametros[1]);
                log_info(logger, "se genero %s unidades de oxigeno",nombre_parametros[1]);
                break;
            }
            case CONSUMIR_OXIGENO:{
                char * path = crear_path_absoluto("/Files/Oxigeno.ims");
                consumir_recurso(path,"Oxigeno",nombre_parametros[1]);
                break;
            }
            case GENERAR_COMIDA:{
                char * path = crear_path_absoluto("/Files/Comida.ims");
                char caracter_llenado = 'C';
                generar_recurso(path, caracter_llenado, nombre_parametros[1]);
                log_info(logger, "se genero %s unidades de comida",nombre_parametros[1]);
                break;
            }
            case CONSUMIR_COMIDA:{
                char * path = crear_path_absoluto("/Files/Comida.ims");
                consumir_recurso(path,"Comida",nombre_parametros[1]);
                break;
            }
            case GENERAR_BASURA:{
                char * path = crear_path_absoluto("/Files/Basura.ims");
                char caracter_llenado = 'B';
                generar_recurso(path, caracter_llenado, nombre_parametros[1]);
                log_info(logger, "se genero %s unidades de basura",nombre_parametros[1]);
                break;
            }
            case DESCARTAR_BASURA:{
                char * path = crear_path_absoluto("/Files/Basura.ims");
                printf("leyo el path\n");
                if(existe_archivo(path)){
                    tarea_config_file = config_create(path);
                    leer_tarea_config(tarea_config_file);
                    char ** lista_bloques = string_get_string_as_array(tarea_config.blocks);
                    printf("entro al for\n");
                    for(int i = 0; lista_bloques[i]!=NULL;i++){
                        printf("liberar bit: %d\n",atoi(lista_bloques[i]));
                        liberar_bit(&bitmap,atoi(lista_bloques[i]));
                    }

                    remove(path);
                    log_info(logger, "Se descarto la basura");
                }else{
                    log_info(logger, "Se intento descartar basura pero no existe el archivo Basura.ims");
                }
                
                break;
            }
            default:{
                log_info(logger,"operacion desconocida");
                break;
            }
        }
        
    }
    
}

void leer_tarea_config(t_config * tarea_config_file){
	tarea_config.size = config_get_int_value(tarea_config_file,"SIZE");
	tarea_config.block_count = config_get_int_value(tarea_config_file,"BLOCK_COUNT");
	tarea_config.blocks = config_get_string_value(tarea_config_file, "BLOCKS");
	tarea_config.caracter_llenado = config_get_string_value(tarea_config_file, "CARACTER_LLENADO");
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
    if(cantidad_bits>0){
        char * bloques_libres = string_new();
        int bit_counter = 0;

        while(bit_counter < cantidad_bits){
            while(!bitarray_test_bit(bitarray, bitoffset)){

            if(bit_counter==0){
                string_append(&bloques_libres, string_itoa(bitoffset));
            }else{
                string_append(&bloques_libres, ",");
                string_append(&bloques_libres, string_itoa(bitoffset));
            }
            printf("string: %s\n",bloques_libres);
            ocupar_bit(bitarray, bitoffset);
            bit_counter++;
            }
            bitoffset++;
        }
        
        lista_bloques_libres = string_split(bloques_libres,",");

        return lista_bloques_libres;
    }else{
        lista_bloques_libres[0] = NULL;
        return lista_bloques_libres;
    }
    
}

void ocupar_bit(t_bitarray *bitarray, int offset){
	bitarray_set_bit(bitarray, offset);
    // copio al void* mapeado el contenido del bitarray para actualizar el bitmap
    int posicion_bitarray = sizeof(super_bloque.blocksize)+ sizeof(super_bloque.blocks);
    memcpy(superbloquemap+posicion_bitarray,super_bloque.bitarray,strlen(super_bloque.bitarray)+1);
    // fuerzo un sync para que el mapeo se refleje en SuperBloque.ims
    msync(superbloquemap, superbloque_stat.st_size, MS_SYNC);
}

void liberar_bit(t_bitarray *bitarray, int offset){
    printf("bitarray antes de liberar:");
		for(int i = 0; i<super_bloque.blocks; i++){
			int test = bitarray_test_bit(&bitmap, i);
			printf("%d",test);
		}
		printf("\n");
	bitarray_clean_bit(bitarray, offset);
    // copio al void* mapeado el contenido del bitarray para actualizar el bitmap
    int posicion_bitarray = sizeof(super_bloque.blocksize)+ sizeof(super_bloque.blocks);
    memcpy(superbloquemap+posicion_bitarray,super_bloque.bitarray,strlen(super_bloque.bitarray)+1);
    // fuerzo un sync para que el mapeo se refleje en SuperBloque.ims
    msync(superbloquemap, superbloque_stat.st_size, MS_SYNC);
    printf("bitarray depues de liberar:");
		for(int i = 0; i<super_bloque.blocks; i++){
			int test = bitarray_test_bit(&bitmap, i);
			printf("%d",test);
		}
		printf("\n");
}

void eliminar_keys_tarea(t_config * tarea_config_file){
    config_remove_key(tarea_config_file,"SIZE");
    config_remove_key(tarea_config_file,"BLOCK_COUNT");
    config_remove_key(tarea_config_file,"BLOCKS");
    config_remove_key(tarea_config_file,"MD5_ARCHIVO");
}

void set_tarea_config(t_config * tarea_config_file,char* size, char* cantidad_bloques, char* bloques, char * md5){
    config_set_value(tarea_config_file,"SIZE",size);
    config_set_value(tarea_config_file,"BLOCK_COUNT",cantidad_bloques);
    config_set_value(tarea_config_file,"BLOCKS",bloques);
    config_set_value(tarea_config_file, "MD5_ARCHIVO", md5);
}

//une dos char ** y los deja como char * [x,x,x]
char * array_two_block_to_string(char ** bloq_antiguo, char** bloq_nuevo){
    char * bloques = string_new();
    string_append(&bloques, "[");
    if (bloq_antiguo[0]!=NULL){
        for(int i = 0; bloq_antiguo[i]!=NULL;i++){
            if(i==tarea_config.block_count-1){
                string_append(&bloques,bloq_antiguo[i]);
            }else{
                string_append(&bloques,bloq_antiguo[i]);
                string_append(&bloques,",");
            }
        }
        for(int i = 0; bloq_nuevo[i]!=NULL;i++){
            string_append(&bloques,",");
            string_append(&bloques,bloq_nuevo[i]);
        }
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
    
    string_append(&bloques, "]");
    return bloques;
}

//indice para "restar" bloques, util para funciones Consumir
char * array_block_to_string(char ** bloq_array, int indice){
    char * bloques = string_new();
    printf("entre en arraytoblock con indice %d\n",indice);
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
        printf("cantidad bloques %d\n",cantidad_de_bloques);
        char * bloques_config;
        //string_append(&bloques_config,"[");
        crear_archivo(path);
        tarea_config_file = config_create(path);
        bloques_usables = bits_libres(&bitmap,cantidad_de_bloques);

        //modifica el array para guardarlo como char * en el config
        bloques_config = array_block_to_string(bloques_usables,cantidad_de_bloques);
        printf("bloques a config: %s\n", bloques_config);
        
        for(int i = 0; bloques_usables[i]!=NULL;i++){
            ocupar_bit(&bitmap, atoi(bloques_usables[i]));
            int offset = atoi(bloques_usables[i])*super_bloque.blocksize;
            if(i==cantidad_de_bloques-1){
                //string_append(&bloques_config,bloques_usables[i]);
                char * fill = string_repeat(caracter_llenado,resto);
                memcpy(blocksmap+offset,fill,resto);
            }else{
                //string_append(&bloques_config,bloques_usables[i]);
                //string_append(&bloques_config,",");
                
                char * fill = string_repeat(caracter_llenado,(int)super_bloque.blocksize);
                memcpy(blocksmap+offset,fill,super_bloque.blocksize);
            }
        }
        //string_append(&bloques_config,"]");
        msync(blocksmap, blocks_stat.st_size, MS_SYNC);
        config_set_value(tarea_config_file,"CARACTER_LLENADO",&caracter_llenado);
        set_tarea_config(
            tarea_config_file,
            parametro,
            string_itoa(cantidad_de_bloques),
            bloques_config,
            "QWERTY"
        );
        config_save(tarea_config_file);
        printf("caracter llenado: %c\n",config_get_string_value(tarea_config_file,"CARACTER_LLENADO"));
    }else{
        //Ya existiendo Oxigeno.ims
        tarea_config_file = config_create(path);
        leer_tarea_config(tarea_config_file);
        char * caracter_llenado = tarea_config.caracter_llenado;
        char ** lista_bloques = string_get_string_as_array(tarea_config.blocks);
        char * bloques_config;
        int size_a_modificar = atoi(parametro);

        int resto_anterior = tarea_config.size%super_bloque.blocksize;
        printf("resto anterior%d\n",resto_anterior);
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
            printf("bloque numero:%s\n",ultimo_bloque_anterior);
            int ultimo_bloque_offset = atoi(ultimo_bloque_anterior)*super_bloque.blocksize;
            memcpy(blocksmap+ultimo_bloque_offset+ resto_anterior,fill,cantidad_repetir);
            size_a_modificar-=(cantidad_repetir);
        }

        int resto = size_a_modificar%super_bloque.blocksize;
        printf("resto actual:%d\n",resto);
        cantidad_de_bloques=size_a_modificar/super_bloque.blocksize;
        if(resto!=0){
            cantidad_de_bloques++;
        }
        printf("cantidad bloques %d\n",cantidad_de_bloques);
        
        //solicita una lista de bloques disponibles y los setea en memoria
        bloques_usables = bits_libres(&bitmap,cantidad_de_bloques);

        //une la lista de bloques del config con los bloques nuevos a usar
        bloques_config=array_two_block_to_string(lista_bloques,bloques_usables);
        printf("bloques a config: %s\n", bloques_config);

        for(int i = 0; bloques_usables[i]!=NULL;i++){
            int offset = atoi(bloques_usables[i])*super_bloque.blocksize;
            if(i==cantidad_de_bloques-1){
                char * fill = string_repeat(*caracter_llenado,resto);
                memcpy(blocksmap+offset,fill,strlen(fill));
            }else{
                char * fill = string_repeat(*caracter_llenado,(int)super_bloque.blocksize);
                memcpy(blocksmap+offset,fill,strlen(fill));
            }
        }
        msync(blocksmap, blocks_stat.st_size, MS_SYNC);
        eliminar_keys_tarea(tarea_config_file);
        set_tarea_config(
            tarea_config_file,
            string_itoa(tarea_config.size + atoi(parametro)),
            string_itoa(tarea_config.block_count+cantidad_de_bloques),
            bloques_config,
            "QWERTY"
        );
        config_save(tarea_config_file);
    }
}

void consumir_recurso(char* path, char* nombre_recurso ,char* parametro){
    tarea_config_file = config_create(path);
    leer_tarea_config(tarea_config_file);
    char ** lista_bloques = string_get_string_as_array(tarea_config.blocks);
    if(existe_archivo(path)){
        int size_a_modificar = atoi(parametro);
        printf("entro 2do if\n");
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
                memcpy(blocksmap+bloque_offset+elem_restantes_resto,fill,strlen(fill));
                //size_a_modificar-=(cantidad_repetir);
                indice_restantes++;
            }
            printf("entro for bloques mod\n");
            for (int i = indice_restantes; lista_bloques[i]!=NULL; i++){
                printf("entro for bloques mod\n");
                char * fill = string_repeat(' ',super_bloque.blocksize);
                int offset = super_bloque.blocksize*atoi(lista_bloques[i]);
                memcpy(blocksmap+offset,fill,strlen(fill));
                printf("entro BITMAP\n");
                liberar_bit(&bitmap,atoi(lista_bloques[i]));
            }
            printf("sali del for \n");
            char * bloques_config = array_block_to_string(lista_bloques,block_config);
            msync(blocksmap, blocks_stat.st_size, MS_SYNC);
            printf("config\n");
            eliminar_keys_tarea(tarea_config_file);
            set_tarea_config(
                tarea_config_file,
                string_itoa(tarea_config.size - atoi(parametro)),
                string_itoa(block_config),
                bloques_config,
                "QWERTY"
            );
            config_save(tarea_config_file);
            log_info(logger, "se consumio %s unidades de %s\n",parametro, nombre_recurso);
        }else{
            //borrar archivo entero
            for (int i = 0; lista_bloques[i]!=NULL; i++){
                printf("entro for bloques mod\n");
                char * fill = string_repeat(' ',super_bloque.blocksize);
                int offset = super_bloque.blocksize*atoi(lista_bloques[i]);
                memcpy(blocksmap+offset,fill,strlen(fill));
                printf("entro BITMAP\n");
                liberar_bit(&bitmap,atoi(lista_bloques[i]));
            }
            printf("sali del for \n");
            char * bloques_config = array_block_to_string(lista_bloques,0);
            msync(blocksmap, blocks_stat.st_size, MS_SYNC);
            printf("config\n");
            eliminar_keys_tarea(tarea_config_file);
            set_tarea_config(
                tarea_config_file,
                string_itoa(0),
                string_itoa(0),
                bloques_config,
                "QWERTY"
            );
            config_save(tarea_config_file);
            log_info(logger, "se intento consumir mas unidades de %s que las disponibles",nombre_recurso);
        }
        
    }else{
        log_info(logger,"No se encontro archivo %s.ims",nombre_recurso);
    }
}