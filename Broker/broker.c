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
// ----------- FUNCIONES ENCOLAR -----------------------------------------------------
void encolarNewPokemon(t_new_pokemon* mensaje){
	t_nodo_cola_new* nodo = malloc(sizeof(t_nodo_cola_new));
	nodo->mensaje = mensaje;
	nodo->susc_enviados = list_create();
	nodo->susc_ack = list_create();

	list_add(cola_new->nodos,nodo);
}
void encolarAppearedPokemon(t_appeared_pokemon* mensaje){
	t_nodo_cola_appeared* nodo = malloc(sizeof(t_nodo_cola_appeared));
	nodo->mensaje = mensaje;
	nodo->susc_enviados = list_create();
	nodo->susc_ack = list_create();

	list_add(cola_appeared->nodos,nodo);
}
void encolarCatchPokemon(t_catch_pokemon* mensaje){
	t_nodo_cola_catch* nodo = malloc(sizeof(t_nodo_cola_catch));
	nodo->mensaje = mensaje;
	nodo->susc_enviados = list_create();
	nodo->susc_ack = list_create();

	list_add(cola_catch->nodos,nodo);
}
void encolarCaughtPokemon(t_caught_pokemon* mensaje){
	t_nodo_cola_caught* nodo = malloc(sizeof(t_nodo_cola_caught));
	nodo->mensaje = mensaje;
	nodo->susc_enviados = list_create();
	nodo->susc_ack = list_create();

	list_add(cola_caught->nodos,nodo);
}
void encolarGetPokemon(t_get_pokemon* mensaje){
	t_nodo_cola_get* nodo = malloc(sizeof(t_nodo_cola_get));
	nodo->mensaje = mensaje;
	nodo->susc_enviados = list_create();
	nodo->susc_ack = list_create();

	list_add(cola_get->nodos,nodo);
}
void encolarLocalizedPokemon(t_localized_pokemon* mensaje){
	t_nodo_cola_localized* nodo = malloc(sizeof(t_nodo_cola_localized));
	nodo->mensaje = mensaje;
	nodo->susc_enviados = list_create();
	nodo->susc_ack = list_create();

	list_add(cola_localized->nodos,nodo);
}
// -----------------------------------------------------------------------------------------
int devolverID(int nroSocket){
	uint32_t id = ID_MENSAJES ++; //Sincronizar obviously
	void*stream = malloc(sizeof(uint32_t));

	memcpy(stream, &(id), sizeof(uint32_t));

	int enviado = send(nroSocket, stream, sizeof(uint32_t), 0);

	return enviado;
}
int suscribir(int socket,op_code cola){
	int index = 0;
	switch(cola){
		case NEW_POKEMON:{
			index= list_add(cola_new->suscriptores,&socket);
			break;
		}
		case APPEARED_POKEMON:{
			index= list_add(cola_appeared->suscriptores,&socket);
			break;
		}
		case CATCH_POKEMON:{
			index= list_add(cola_catch->suscriptores,&socket);
			break;
		}
		case CAUGHT_POKEMON:{
			index= list_add(cola_caught->suscriptores,&socket);
			break;
		}
		case GET_POKEMON:{
			index= list_add(cola_get->suscriptores,&socket);
			break;
		}
		case LOCALIZED_POKEMON:{
			index= list_add(cola_localized->suscriptores,&socket);
			break;
		}
		default:
			index = -1;
	}
	return index;
}
//A optimizar / rediseñar
void desuscribir(int index,op_code cola){

	switch(cola){
		case NEW_POKEMON:{
			list_remove(cola_new->suscriptores,index);
			break;
		}
		case APPEARED_POKEMON:{
			list_remove(cola_appeared->suscriptores,index);
			break;
		}
		case CATCH_POKEMON:{
			list_remove(cola_catch->suscriptores,index);
			break;
		}
		case CAUGHT_POKEMON:{
			list_remove(cola_caught->suscriptores,index);
			break;
		}
		case GET_POKEMON:{
			list_remove(cola_get->suscriptores,index);
			break;
		}
		case LOCALIZED_POKEMON:{
			list_remove(cola_localized->suscriptores,index);
			break;
		}
		default:
			break;
	}
}
// --------------- HANDLES DE CADA MENSAJE -----------------------------------------------
void handleSuscribeGameboyMsg(int socket_cliente){
	t_suscribe* suscribe_gameboy = recibirSuscripcion(SUSCRIBE_GAMEBOY,socket_cliente);
	log_info(logBroker, "Llego el mensaje SUSCRIBE_GAMEBOY.");

	int index = suscribir(socket_cliente,suscribe_gameboy->cola_suscribir);
	log_info(logBroker, "Se suscribió al Gameboy, indice %d",index);

	sleep(suscribe_gameboy->timeout);
	desuscribir(index,suscribe_gameboy->cola_suscribir);
}
void handleSuscribeTeamMsg(int socket_cliente){
	t_suscribe* suscribe_team = recibirSuscripcion(SUSCRIBE_TEAM,socket_cliente);
	log_info(logBroker, "Llego el mensaje SUSCRIBE_TEAM.");

	int index = suscribir(socket_cliente,suscribe_team->cola_suscribir);
	log_info(logBroker, "Se suscribió al team, indice %d",index);
}
void handleSuscribeGameCardMsg(int socket_cliente){
	t_suscribe* suscribe_gamecard = recibirSuscripcion(SUSCRIBE_GAMECARD,socket_cliente);
	log_info(logBroker, "Llego el mensaje SUSCRIBE_GAMECARD.");

	int index = suscribir(socket_cliente,suscribe_gamecard->cola_suscribir);
	log_info(logBroker, "Se suscribió al gamecard, indice %d",index);
}

void handleNewPokemonMsg(int socket_cliente){
	t_new_pokemon* new_pokemon = recibirNewPokemon(socket_cliente);
	log_info(logBroker, "Llego el mensaje NEW_POKEMON.");
	encolarNewPokemon(new_pokemon);
	int enviado = devolverID(socket_cliente);
}
void handleAppearedPokemonMsg(int socket_cliente){
	t_appeared_pokemon* app_pokemon = recibirAppearedPokemon(socket_cliente);
	log_info(logBroker, "Llego el mensaje APPEARED_POKEMON.");

	encolarAppearedPokemon(app_pokemon);
	int enviado = devolverID(socket_cliente);
}
void handleCatchPokemonMsg(int socket_cliente){
	t_catch_pokemon* mensaje = recibirCatchPokemon(socket_cliente);
	log_info(logBroker, "Llego el mensaje CATCH_POKEMON.");

	encolarCatchPokemon(mensaje);
	int enviado = devolverID(socket_cliente);
}
void handleCaughtPokemonMsg(int socket_cliente){
	t_caught_pokemon* mensaje = recibirCaughtPokemon(socket_cliente);
	log_info(logBroker, "Llego el mensaje CAUGHT_POKEMON.");

	encolarCaughtPokemon(mensaje);
	int enviado = devolverID(socket_cliente);
}
void handleGetPokemonMsg(int socket_cliente){
	t_get_pokemon* mensaje = recibirGetPokemon(socket_cliente);
	log_info(logBroker, "Llego el mensaje GET_POKEMON.");

	encolarGetPokemon(mensaje);
	int enviado = devolverID(socket_cliente);
}
void handleLocalizedPokemonMsg(int socket_cliente){
	t_localized_pokemon* mensaje = recibirLocalizedPokemon(socket_cliente);
	log_info(logBroker, "Llego el mensaje LOCALIZED_POKEMON.");

	encolarLocalizedPokemon(mensaje);
	int enviado = devolverID(socket_cliente);
}
void atenderCliente(int socket_cliente){
	printf("Atender Cliente %d: \n", socket_cliente);
	op_code cod_op = recibirOperacion(socket_cliente);
	switch(cod_op){
		case NEW_POKEMON:{
			handleNewPokemonMsg(socket_cliente);
			break;
		}
		case APPEARED_POKEMON:{
			handleAppearedPokemonMsg(socket_cliente);
			break;
		}
		case CATCH_POKEMON:{
			handleCatchPokemonMsg(socket_cliente);
			break;
		}
		case CAUGHT_POKEMON:{
			handleCaughtPokemonMsg(socket_cliente);
			break;
		}
		case GET_POKEMON:{
			handleGetPokemonMsg(socket_cliente);
			break;
		}
		case LOCALIZED_POKEMON:{
			handleLocalizedPokemonMsg(socket_cliente);
			break;
		}
		case SUSCRIBE_TEAM:{
			handleSuscribeTeamMsg(socket_cliente);
			break;
		}
		case SUSCRIBE_GAMECARD:{
			handleSuscribeGameCardMsg(socket_cliente);
			break;
		}
		case SUSCRIBE_GAMEBOY:{
			handleSuscribeGameboyMsg(socket_cliente);
			break;
		}
		default:
			log_info(logBroker, "La operación no es correcta.");
			break;
	}
}

void inicializarEstructuras(){
	crearConfigBroker();
	cola_new = malloc(sizeof(t_cola));
	cola_appeared = malloc(sizeof(t_cola));
	cola_get = malloc(sizeof(t_cola));
	cola_localized = malloc(sizeof(t_cola));
	cola_catch = malloc(sizeof(t_cola));
	cola_caught = malloc(sizeof(t_cola));

	cola_new->nodos = list_create();;
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
