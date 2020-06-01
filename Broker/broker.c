/*
 * broker.c
 *
 *  Created on: 24 abr. 2020
 *      Author: aislamientoSOcial
 */

#include "broker.h"

int crearConfigBroker(){
	log_info(logBrokerInterno, "Se hizo el Log.");

    if (!existeArchivoConfig(pathConfigBroker)) {
		log_error(logBrokerInterno, "ERROR: Verificar path del archivo.\n");
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
	printf("Atender Cliente %d: \n", socket_cliente);
	op_code cod_op = recibirOperacion(socket_cliente);
	switch(cod_op){
		case NEW_POKEMON:
			t_new_pokemon* new_pokemon = recibirNewPokemon(socket_cliente);
			log_info(logBroker, "Llego el mensaje NEW_POKEMON.");
			break;
		case APPEARED_POKEMON:
			t_appeared_pokemon* app_pokemon = recibirAppearedPokemon(socket_cliente);
			log_info(logBroker, "Llego el mensaje APPEARED_POKEMON.");
			break;
		case CATCH_POKEMON:
			t_catch_pokemon* catch_pokemon = recibirCatchPokemon(socket_cliente);
			log_info(logBroker, "Llego el mensaje CATCH_POKEMON.");
			break;
		case CAUGHT_POKEMON:
			t_caught_pokemon* caught_pokemon= recibirCaughtPokemon(socket_cliente);
			log_info(logBroker, "Llego el mensaje CAUGHT_POKEMON.");
			break;
		case GET_POKEMON:
			t_get_pokemon* get_pokemon = recibirGetPokemon(socket_cliente);
			log_info(logBroker, "Llego el mensaje GET_POKEMON.");
			break;
		case LOCALIZED_POKEMON:
			t_localized_pokemon* localized_pokemon = recibirLocalizedPokemon(socket_cliente);
			log_info(logBroker, "Llego el mensaje LOCALIZED_POKEMON.");
			break;
		default:
			printf("La operación no es correcta");
			break;
	}
}

void inicializarColas(){
	crearConfigBroker();

	colaNewPokemon = list_create();
	colaAppearedPokemon = list_create();
	colaCatchPokemon = list_create();
	colaCaughtPokemon = list_create();
	colaGetPokemon = list_create();
	colaLocalizedPokemon = list_create();
}

int main(void){
	inicializarColas();

	int socketServidorBroker = crearSocketServidor(config_broker->ip_broker, config_broker->puerto_broker);

	if(socketServidorBroker == -1){
		printf("No se pudo crear el Servidor Broker.");
	}else{
		printf("Socket Servidor %d.\n", socketServidorBroker);
	}

	int cliente = aceptarCliente(socketServidorBroker);

	atenderCliente(cliente);

	if(socketServidorBroker != -1) close(socketServidorBroker);
}
