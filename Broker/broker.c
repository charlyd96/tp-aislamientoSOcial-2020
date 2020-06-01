/*
 * broker.c
 *
 *  Created on: 24 abr. 2020
 *      Author: aislamientoSOcial
 */

#include "broker.h"

int crearConfigBroker(){
	t_log* logBrokerInterno = log_create("broker.txt", "LOG", 0, LOG_LEVEL_INFO);

    log_info(logBrokerInterno, "Se hizo el Log.\n");

    if (!existeArchivoConfig(pathConfigBroker)) {
		log_error(logBrokerInterno, "Verificar path del archivo \n");
	    return -1;
	}

	config_broker = malloc(sizeof(t_configuracion));
	config_ruta = config_create(pathConfigBroker);

	if (config_ruta != NULL){
	    config_broker->tam_memoria = config_get_int_value(config_ruta, "TAMANO_MEMORIA");
		config_broker->tam_minimo_particion = config_get_int_value(config_ruta, "TAMANO_MINIMO_PARTICION");
	    config_broker->algoritmo_memoria = config_get_string_value(config_ruta, "ALGORITMO_MEMORIA");
	    config_broker->algoritmo_reemplazo = config_get_string_value(config_ruta, "ALGORITMO_REEMPLAZO");
	    config_broker->algoritmo_particion_libre = config_get_string_value(config_ruta, "ALGORITMO_PARTICION_LIBRE");
	    config_broker->ip_broker = config_get_string_value(config_ruta, "IP_BROKER");
	    config_broker->puerto_broker = config_get_string_value(config_ruta, "PUERTO_BROKER");
	    config_broker->frecuencia_compatacion = config_get_int_value(config_ruta, "FRECUENCIA_COMPACTACION");
	}

	if(config_broker == NULL){
	    log_error(logBrokerInterno, "ERROR: No se pudo levantar el archivo de configuración.\n");
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
		case APPEARED_POKEMON:
			printf("Recibi APPEARED\n");
			t_appeared_pokemon* app_pokemon= recibirAppearedPokemon(socket_cliente);
			printf(" nombre: %s\n x: %d\n y: %d\n id correlativo: %d\n",app_pokemon->nombre_pokemon,app_pokemon->pos_x,app_pokemon->pos_y,app_pokemon->id_mensaje_correlativo);
			/**
			 * tratarRecepcionAppPokemon(app_pokemon)
			 */
			break;
		case LOCALIZED_POKEMON:
			printf("Recibi LOCALIZED_POKEMON\n");
			t_localized_pokemon* localized_pokemon= recibirLocalizedPokemon(socket_cliente);
			printf(" nombre: %s\n cantidad: %d\n posiciones: %s\n id correlativo: %d\n",localized_pokemon->nombre_pokemon,localized_pokemon->cant_pos,localized_pokemon->posiciones,localized_pokemon->id_mensaje_correlativo);
			/**
			 * tratarRecepcionLocalizedPokemon(localized_pokemon)
			 */
			break;
		case CAUGHT_POKEMON:
			printf(" Recibi CAUGHT_POKEMON\n");
			t_caught_pokemon* caught_pokemon= recibirCaughtPokemon(socket_cliente);
			printf(" resultado: %d\n id correlativo: %d\n",caught_pokemon->atrapo_pokemon,caught_pokemon->id_mensaje_correlativo);
			/**
			 * tratarRecepcionCaughtPokemon(caught_pokemon)
			 */
			break;
		default:
			printf("La operación no es correcta");
			break;
	}
}

int main(void){
	crearConfigBroker();

	int socketServidorBroker = crearSocketServidor(config_broker->ip_broker, config_broker->puerto_broker);

	if(socketServidorBroker == -1){
		printf("No se pudo crear el Servidor Broker");
	}else{
		printf("Socket Servidor %d\n", socketServidorBroker);
	}

	int cliente = aceptarCliente(socketServidorBroker);

	atenderCliente(cliente);

	if(socketServidorBroker != -1) close(socketServidorBroker);

}
