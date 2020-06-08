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

void encolarNewPokemon(t_new_pokemon* mensaje){
	t_nodo_cola_new* nodo = malloc(sizeof(t_nodo_cola_new));
	nodo->mensaje = mensaje;
	nodo->susc_enviados = list_create();
	nodo->susc_ack = list_create();

	list_add(cola_new->nodos,nodo);
}
void atenderCliente(int socket_cliente){
	printf("Atender Cliente %d: \n", socket_cliente);
	op_code cod_op = recibirOperacion(socket_cliente);
	switch(cod_op){
		case NEW_POKEMON:{
			t_new_pokemon* new_pokemon = recibirNewPokemon(socket_cliente);
			log_info(logBroker, "Llego el mensaje NEW_POKEMON.");
			encolarNewPokemon(new_pokemon);
			break;
		}
		case APPEARED_POKEMON:{
			t_appeared_pokemon* app_pokemon = recibirAppearedPokemon(socket_cliente);
			log_info(logBroker, "Llego el mensaje APPEARED_POKEMON.");
			break;
		}
		case CATCH_POKEMON:{
			t_catch_pokemon* catch_pokemon = recibirCatchPokemon(socket_cliente);
			log_info(logBroker, "Llego el mensaje CATCH_POKEMON.");
			break;
		}
		case CAUGHT_POKEMON:{
			t_caught_pokemon* caught_pokemon= recibirCaughtPokemon(socket_cliente);
			log_info(logBroker, "Llego el mensaje CAUGHT_POKEMON.");
			break;
		}
		case GET_POKEMON:{
			t_get_pokemon* get_pokemon = recibirGetPokemon(socket_cliente);
			log_info(logBroker, "Llego el mensaje GET_POKEMON.");
			break;
		}
		case LOCALIZED_POKEMON:{
			t_localized_pokemon* localized_pokemon = recibirLocalizedPokemon(socket_cliente);
			log_info(logBroker, "Llego el mensaje LOCALIZED_POKEMON.");
			break;
		}
		case SUSCRIBE_TEAM:{
			t_suscribe* suscribe_team = recibirSuscripcion(SUSCRIBE_TEAM,socket_cliente);
			log_info(logBroker, "Llego el mensaje SUSCRIBE_TEAM.");
			// atenderSuscripcion(suscribe_team);
			break;
		}
		case SUSCRIBE_GAMECARD:{
			t_suscribe* suscribe_gamecard = recibirSuscripcion(SUSCRIBE_GAMECARD,socket_cliente);
			log_info(logBroker, "Llego el mensaje SUSCRIBE_GAMECARD.");
			// atenderSuscripcion(suscribe_gamecard);
			break;
		}
		case SUSCRIBE_GAMEBOY:{
			t_suscribe* suscribe_gameboy = recibirSuscripcion(SUSCRIBE_GAMEBOY,socket_cliente);
			log_info(logBroker, "Llego el mensaje SUSCRIBE_GAMEBOY.");
			// atenderSuscripcion(suscribe_gameboy);
			break;
		}
		default:
			log_info(logBroker, "La operación no es correcta.");
			break;
	}
}

void inicializarEstructuras(){
	crearConfigBroker();

	cola_new->nodos = list_create();
	cola_new->suscriptores = list_create();

	cola_appeared->nodos = list_create();
	cola_appeared->suscriptores = list_create();

	cola_get->nodos = list_create();
	cola_get->suscriptores = list_create();

	cola_localized->nodos = list_create();
	cola_localized->suscriptores = list_create();

	cola_catch->nodos = list_create();
	cola_catch->suscriptores = list_create();

	cola_caught->nodos = list_create();
	cola_caught->suscriptores = list_create();
}

int main(void){
	logBroker = log_create("broker.log", "LOG", 0, LOG_LEVEL_INFO);
	logBrokerInterno= log_create("brokerInterno.log", "LOG", 0, LOG_LEVEL_INFO);
	inicializarEstructuras();

	int socketServidorBroker = crearSocketServidor(config_broker->ip_broker, config_broker->puerto_broker);

	if(socketServidorBroker == -1){
		printf("No se pudo crear el Servidor Broker.");
		return -1;
	}else{
		printf("Socket Servidor %d.\n", socketServidorBroker);
	}

	while(1){
		int cliente = aceptarCliente(socketServidorBroker);
		atenderCliente(cliente);
	}
	if(socketServidorBroker != -1) close(socketServidorBroker);
}
