/*
 * broker.c
 *
 *  Created on: 24 abr. 2020
 *      Author: aislamientoSOcial
 */

#include "broker.h"

/* FUNCIONES - INICIALIZACIÓN */

int crearConfigBroker(){
	log_info(logBrokerInterno, "Se inicializó el Log.");

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

void inicializarColas(){
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

void inicializarMemoria(){
	punteroMemoria = malloc(config_broker->tam_memoria);

	algoritmoMemoria = config_broker->algoritmo_memoria;
}

/* FUNCIONES - CONEXIÓN */

void atenderCliente(int socket_cliente){
	printf("Atender Cliente %d: \n", socket_cliente);
	op_code cod_op = recibirOperacion(socket_cliente);
	switch(cod_op){
		case NEW_POKEMON:{
			atenderMensajeNewPokemon(socket_cliente);
			break;
		}
		case APPEARED_POKEMON:{
			atenderMensajeAppearedPokemon(socket_cliente);
			break;
		}
		case CATCH_POKEMON:{
			atenderMensajeCatchPokemon(socket_cliente);
			break;
		}
		case CAUGHT_POKEMON:{
			atenderMensajeCaughtPokemon(socket_cliente);
			break;
		}
		case GET_POKEMON:{
			atenderMensajeGetPokemon(socket_cliente);
			break;
		}
		case LOCALIZED_POKEMON:{
			atenderMensajeLocalizedPokemon(socket_cliente);
			break;
		}
		case SUSCRIBE_TEAM:{
			atenderSuscripcionTeam(socket_cliente);
			break;
		}
		case SUSCRIBE_GAMECARD:{
			atenderSuscripcionGameCard(socket_cliente);
			break;
		}
		case SUSCRIBE_GAMEBOY:{
			atenderSuscripcionGameBoy(socket_cliente);
			break;
		}
		default:{
			log_info(logBrokerInterno, "No se pudo conectar ningún proceso.");
			break;
		}
	}
}

void atenderMensajeNewPokemon(int socket_cliente){
	t_new_pokemon* new_pokemon = recibirNewPokemon(socket_cliente);
	log_info(logBroker, "Llegó el mensaje NEW_POKEMON.");

	encolarNewPokemon(new_pokemon);
	int enviado = devolverID(socket_cliente);
}

void atenderMensajeAppearedPokemon(int socket_cliente){
	t_appeared_pokemon* app_pokemon = recibirAppearedPokemon(socket_cliente);
	log_info(logBroker, "Llegó el mensaje APPEARED_POKEMON.");

	encolarAppearedPokemon(app_pokemon);
	int enviado = devolverID(socket_cliente);
}

void atenderMensajeCatchPokemon(int socket_cliente){
	t_catch_pokemon* mensaje = recibirCatchPokemon(socket_cliente);
	log_info(logBroker, "Llegó el mensaje CATCH_POKEMON.");

	encolarCatchPokemon(mensaje);
	int enviado = devolverID(socket_cliente);
}

void atenderMensajeCaughtPokemon(int socket_cliente){
	t_caught_pokemon* mensaje = recibirCaughtPokemon(socket_cliente);
	log_info(logBroker, "Llegó el mensaje CAUGHT_POKEMON.");

	encolarCaughtPokemon(mensaje);
	int enviado = devolverID(socket_cliente);
}

void atenderMensajeGetPokemon(int socket_cliente){
	t_get_pokemon* mensaje = recibirGetPokemon(socket_cliente);
	log_info(logBroker, "Llegó el mensaje GET_POKEMON.");

	encolarGetPokemon(mensaje);
	int enviado = devolverID(socket_cliente);
}

void atenderMensajeLocalizedPokemon(int socket_cliente){
	t_localized_pokemon* mensaje = recibirLocalizedPokemon(socket_cliente);
	log_info(logBroker, "Llegó el mensaje LOCALIZED_POKEMON.");

	encolarLocalizedPokemon(mensaje);
	int enviado = devolverID(socket_cliente);

	//chachearMensaje(mensaje);
	//hilo-enviarMensajeASuscriptores(mensaje);
	//hilo-recibirACK(socket_cliente);

	//si retorna ACK elimino el mensaje de la COLA
	//si no retorna ACK ?
}

void atenderSuscripcionTeam(int socket_cliente){
	t_suscribe* suscribe_team = recibirSuscripcion(SUSCRIBE_TEAM,socket_cliente);

	int index = suscribir(socket_cliente,suscribe_team->cola_suscribir);
	log_info(logBroker, "Se suscribió al team, indice %d",index);

	//enviar mensajes cacheados (recorrer la lista de particiones)
}

void atenderSuscripcionGameBoy(int socket_cliente){
	t_suscribe* suscribe_gameboy = recibirSuscripcion(SUSCRIBE_GAMEBOY,socket_cliente);

	int index = suscribir(socket_cliente,suscribe_gameboy->cola_suscribir);
	log_info(logBroker, "Se suscribió al Gameboy, indice %d",index);

	sleep(suscribe_gameboy->timeout);
	desuscribir(index,suscribe_gameboy->cola_suscribir);
}

void atenderSuscripcionGameCard(int socket_cliente){
	t_suscribe* suscribe_gamecard = recibirSuscripcion(SUSCRIBE_GAMECARD,socket_cliente);

	int index = suscribir(socket_cliente,suscribe_gamecard->cola_suscribir);
	log_info(logBroker, "Se suscribió al gamecard, indice %d",index);
}

/* FUNCIONES - PROCESAMIENTO */

int suscribir(int socket, op_code cola){
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
			log_info(logBrokerInterno, "No se suscribe ningún proceso.");
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

/* FUNCIONES - COMUNICACIÓN */

int devolverID(int socket){
	uint32_t id = ID_MENSAJE ++; //Sincronizar obviously
	void*stream = malloc(sizeof(uint32_t));

	memcpy(stream, &(id), sizeof(uint32_t));

	int enviado = send(socket, stream, sizeof(uint32_t), 0);

	printf("ID %d\n", id);
	return enviado;
}

int main(void){
	logBroker = log_create("broker.log", "Broker", 0, LOG_LEVEL_INFO);
	logBrokerInterno = log_create("brokerInterno.log", "Broker Interno", 0, LOG_LEVEL_INFO);

	inicializarColas();
	inicializarMemoria();

	socketServidorBroker = crearSocketServidor(config_broker->ip_broker, config_broker->puerto_broker);

	if(socketServidorBroker == -1){
		printf("No se pudo crear el Servidor Broker.");
		return -1;
	}else{
		printf("Socket Servidor %d.\n", socketServidorBroker);
	}

	//while(1){
		cliente = aceptarCliente(socketServidorBroker);
		atenderCliente(cliente);
	//}

	if(socketServidorBroker != -1){
		close(socketServidorBroker);
		log_info(logBrokerInterno, "Se cerró el Socket %d.", socketServidorBroker);
	}

	log_destroy(logBrokerInterno);
	log_destroy(logBroker);
}
