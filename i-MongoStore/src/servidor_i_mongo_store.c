#include "servidor_i_mongo_store.h"

void loguear_mensaje(char* payload){
    log_info(logger, "Recibi el mensaje: %s", payload);
}