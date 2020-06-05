/*
 * gameCard.c
 *
 *  Created on: 28 abr. 2020
 *      Author: aislamientoSOcial
 */

#include "gamecard.h"

int crearConfigGamecard(){

	t_log* logGamecard = log_create("gamecard.txt", "LOG", 0, LOG_LEVEL_INFO);

    log_info(logGamecard, "Se hizo el Log.\n");

	if (!existeArchivoConfig(pathGamecardConfig)) {
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

bool existeArchivoConfig(char* path){
	FILE * file = fopen(path, "r");
	if(file!=NULL){
		fclose(file);
		return true;
	} else{
		return false;
	}
}

void atenderCliente(int socket_cliente){
	printf("Atender cliente %d \n", socket_cliente);
	op_code cod_op = recibirOperacion(socket_cliente);
	switch(cod_op){
		case NEW_POKEMON:
			printf("Recibi NEW_POKEMON");
			t_new_pokemon* new_pokemon= recibirNewPokemon(socket_cliente);
			printf("nombre: %s\n x: %d\n y: %d\n cant: %d\n id: %d\n",new_pokemon->nombre_pokemon,new_pokemon->pos_x,new_pokemon->pos_y,new_pokemon->cantidad,new_pokemon->id_mensaje);
			/**
			 * tratarRecepcionNewPokemon(new_pokemon)
			 */
			break;
		case GET_POKEMON:
			printf("Recibi GET_POKEMON\n");
			t_get_pokemon* get_pokemon= recibirGetPokemon(socket_cliente);
			printf(" nombre: %s\n id: %d\n",get_pokemon->nombre_pokemon,get_pokemon->id_mensaje);
			/**
			 * tratarRecepcionGetPokemon(get_pokemon)
			 */
			break;
		case CATCH_POKEMON:
			printf("Recibi CATCH_POKEMON\n");
			t_catch_pokemon* catch_pokemon= recibirCatchPokemon(socket_cliente);
			printf(" nombre: %s\n x: %d\n y: %d\n id: %d\n",catch_pokemon->nombre_pokemon,catch_pokemon->pos_x,catch_pokemon->pos_y,catch_pokemon->id_mensaje);
			/**
			 * tratarRecepcionCatchPokemon(catch_pokemon)
			 */
			break;
		default:
			printf("La operación no es correcta");
			break;
	}
}

int main(void){

	crearConfigGamecard();

	int socketServidorGamecard = crearSocketServidor(config_gamecard->ip_gamecard, config_gamecard->puerto_gamecard);

	if(socketServidorGamecard == -1){
		printf("No se pudo crear el Servidor GameCard");
	}else{
		printf("Socket Servidor %d\n", socketServidorGamecard);

		int cliente = aceptarCliente(socketServidorGamecard);

		atenderCliente(cliente);
	}

	if(socketServidorGamecard != -1) close(socketServidorGamecard);

}
