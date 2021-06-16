#include "servidor_i_mongo_store.h"

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
        t_config * tarea_config;
        int codigo_tarea = -1;
        conseguir_codigo_tarea(nombre_parametros[0],&codigo_tarea);
        switch(codigo_tarea){
            case GENERAR_OXIGENO:;
                char * path = crear_path_absoluto("/Files/Oxigeno.ims");
                char ** bloques_usables;
                int cantidad_de_bloques;
                int resto = atoi(nombre_parametros[1])%super_bloque.blocksize;
                cantidad_de_bloques=atoi(nombre_parametros[1])/super_bloque.blocksize;
                if(resto!=0){
                    cantidad_de_bloques++;
                }
                printf("cantidad bloques %d\n",cantidad_de_bloques);
                if(!existe_archivo(path)){
                    char * bloques_config= string_new();
                    string_append(&bloques_config,"[");
                    crear_archivo(path);
                    tarea_config = config_create(path);
                    config_set_value(tarea_config, "CARACTER_LLENADO","O");
                    bloques_usables = bits_libres(&bitmap,cantidad_de_bloques);
                    printf("bloques libres %s, %s\n",bloques_usables[0],bloques_usables[1]);
                    for(int i = 0; bloques_usables[i]!=NULL;i++){
                        ocupar_bit(&bitmap, atoi(bloques_usables[i]));
                        int offset = atoi(bloques_usables[i])*super_bloque.blocksize;
                        if(i==cantidad_de_bloques-1){
                            string_append(&bloques_config,bloques_usables[i]);
                            char * fill = string_repeat('O',resto);
                            memcpy(blocksmap+offset,fill,resto);
                        }else{
                            string_append(&bloques_config,bloques_usables[i]);
                            string_append(&bloques_config,",");
                            
                            char * fill = string_repeat('O',(int)super_bloque.blocksize);
                            memcpy(blocksmap+offset,fill,super_bloque.blocksize);
                        }
                    }
                    string_append(&bloques_config,"]");
                    msync(blocksmap, blocks_stat.st_size, MS_SYNC);

                    config_set_value(tarea_config,"BLOQUES",bloques_config);
                    config_save(tarea_config);
                    printf("caracter llenado: %s\n",config_get_string_value(tarea_config,"CARACTER_LLENADO"));
                }else{
                    tarea_config = config_create(path);

                }


                log_info(logger, "se genero %s unidades de oxigeno",nombre_parametros[1]);
                break;
            case CONSUMIR_OXIGENO:
                log_info(logger, "se consumio %s unidades de oxigeno",nombre_parametros[1]);
                break;
            case GENERAR_COMIDA:
                log_info(logger, "se genero %s unidades de comida",nombre_parametros[1]);
                break;
            case CONSUMIR_COMIDA:
                log_info(logger, "se consumio %s unidades de comida",nombre_parametros[1]);
                break;
            case GENERAR_BASURA:
                log_info(logger, "se genero %s unidades de basura",nombre_parametros[1]);
                break;
            case DESCARTAR_BASURA:
                log_info(logger, "se descarto la basura");
                break;
            default:
                log_info(logger,"operacion desconocida");
                break;
            
        }
        
    }
    
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
    char * bloques_libres = string_new();
    char ** lista_bloques_libres;
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
}

void ocupar_bit(t_bitarray *bitarray, int offset){
	bitarray_set_bit(bitarray, offset);
}

void liberar_bit(t_bitarray *bitarray, int offset){
	bitarray_clean_bit(bitarray, offset);
}
