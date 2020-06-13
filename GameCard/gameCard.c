/*
 * gameCard.c
 *
 *  Created on: 28 abr. 2020
 *      Author: aislamientoSOcial
 */

#include "gamecard.h"

int main(void){

	logGamecard = log_create("gamecard.log", "Gamecard", 0, LOG_LEVEL_INFO);
	logInterno = log_create("gamecardInterno.log", "GamecardInterno", 0, LOG_LEVEL_INFO);
	crear_config_gamecard();

	int socketServidorGamecard = crearSocketServidor(config_gamecard->ip_gamecard, config_gamecard->puerto_gamecard);

	if(socketServidorGamecard == -1){
		log_info(logGamecard, "No se pudo crear el Servidor GameCard");
	}else{
		log_info(logGamecard, "Socket Servidor %d\n", socketServidorGamecard);
		while(1){
					log_info(logInterno,"While de aceptar cliente");
					int cliente = aceptarCliente(socketServidorGamecard);

					pthread_t hiloCliente;
					pthread_create(&hiloCliente, NULL, (void*)atender_cliente, &cliente);

					pthread_detach(hiloCliente);
				}
		close(socketServidorGamecard);
		log_info(logGamecard, "Se cerro el Soket Servidor %d\n", socketServidorGamecard);
	}

}


int crear_config_gamecard(){

    log_info(logGamecard, "Inicializacion del log.\n");

	if (!existe_archivo(pathGamecardConfig)) {
		log_error(logGamecard, "Verificar path del archivo \n");
	    return -1;
	}

	config_gamecard = malloc(sizeof(t_configuracion));
	config_ruta = config_create(pathGamecardConfig);

	if (config_ruta != NULL){
	    config_gamecard->tiempo_reintento_conexion = config_get_int_value(config_ruta, "TIEMPO_DE_REINTENTO_CONEXION");
		config_gamecard->tiempo_reintento_operacion = config_get_int_value(config_ruta, "TIEMPO_DE_REINTENTO_OPERACION");
	    config_gamecard->punto_montaje_tall_grass = config_get_string_value(config_ruta, "PUNTO_MONTAJE_TALLGRASS");
	    config_gamecard->ip_gamecard = config_get_string_value(config_ruta, "IP_GAMECARD");
	    config_gamecard->puerto_gamecard = config_get_string_value(config_ruta, "PUERTO_GAMECARD");
	}

	if(config_gamecard == NULL){
	    log_error(logGamecard, "ERROR: No se pudo levantar el archivo de configuración.\n");
	    return -1;
	}
	return 0;
}

bool existe_archivo(char* path){
	FILE * file = fopen(path, "r");
	if(file!=NULL){
		fclose(file);
		return true;
	} else{
		return false;
	}
}

void atender_cliente(int* socket){
	int socket_cliente = *socket;
	log_info(logGamecard, "Atender cliente %d \n",socket_cliente);
	op_code cod_op = recibirOperacion(socket_cliente);
	switch(cod_op){
		case NEW_POKEMON:
			atender_newPokemon(socket_cliente);
			break;
		case GET_POKEMON:
			atender_getPokemon(socket_cliente);
			break;
		case CATCH_POKEMON:
			atender_catchPokemon(socket_cliente);
			break;
		default:
			printf("La operación no es correcta");
			break;
	}
}

void atender_newPokemon(int socket){
	t_new_pokemon* new_pokemon = recibirNewPokemon(socket);
	log_info(logGamecard, "Recibi NEW_POKEMON %s %d %d %d [%d]\n",new_pokemon->nombre_pokemon,new_pokemon->pos_x,new_pokemon->pos_y,new_pokemon->cantidad,new_pokemon->id_mensaje);

	char* pathMetadata = string_from_format("%s/Files/%s/Metadata.bin", config_gamecard->punto_montaje_tall_grass, new_pokemon->nombre_pokemon);

	if(existe_archivo(pathMetadata)){
		data_pokemon = malloc(sizeof(t_metadata));
		metadata = config_create(pathMetadata);
		if(metadata != NULL){
			data_pokemon->directory=config_get_string_value(metadata, "DIRECTORY");
			data_pokemon->size=config_get_int_value(metadata,"SIZE");
			data_pokemon->blocks=config_get_array_value(metadata, "BLOCKS");
			data_pokemon->open=config_get_string_value(metadata,"OPEN");
		}
		else {
			log_error(logInterno, "ERROR: No se pudo levantar la Metadata de %s.\n", new_pokemon->nombre_pokemon);
		}
	}
	else{
		crear_directorio_pokemon(new_pokemon->nombre_pokemon);
	}
}

void crear_directorio_pokemon(char* pokemon){
	char* pathFiles = string_from_format("%s/Files", config_gamecard->punto_montaje_tall_grass);
	int cambioAPathFiles = chdir(pathFiles);
	if(cambioAPathFiles){
		string_capitalized(pokemon);
		int creoDirectorio = mkdir(pokemon,0700);
		if(creoDirectorio){
			log_info(logInterno, "Se creo el directorio /%s", pokemon);
			//CREAR METADATA
		}
		else log_error(logInterno, "ERROR: No se pudo crear directorio /%s", pokemon);
	}
	else log_error(logInterno, "ERROR: No se cambio de directorio\n");
}

void atender_getPokemon(int socket){
	t_get_pokemon* get_pokemon= recibirGetPokemon(socket);
	log_info(logGamecard, "Recibi GET_POKEMON %s [%d]\n",get_pokemon->nombre_pokemon,get_pokemon->id_mensaje);
}

void atender_catchPokemon(int socket){
	t_catch_pokemon* catch_pokemon= recibirCatchPokemon(socket);
	log_info(logGamecard, "Recibi CATCH_POKEMON %s %d %d [%d]\n",catch_pokemon->nombre_pokemon,catch_pokemon->pos_x,catch_pokemon->pos_y,catch_pokemon->id_mensaje);
}
