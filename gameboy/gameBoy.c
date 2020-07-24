/*
 * gameboy.c
 *
 *  Created on: 24 abr. 2020
 *      Author: aislamientoSOcial
 */

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include "protocolo.h"
#include "gameBoy.h"
#include "conexion.h"
#include "serializacion.h"
#include "logger.h"
#include <commons/config.h>

char* getNombreCola(op_code cola){
	char * nombreCola = "";
	switch(cola){
		case NEW_POKEMON:{
			nombreCola = "NEW_POKEMON";
		}
		break;
		case GET_POKEMON:{
			nombreCola = "GET_POKEMON";
		}
		break;
		case APPEARED_POKEMON:{
			nombreCola = "APPEARED_POKEMON";
		}
		break;
		case CATCH_POKEMON:{
			nombreCola = "CATCH_POKEMON";
		}
		break;
		case CAUGHT_POKEMON:{
			nombreCola = "CAUGHT_POKEMON";
		}
		break;
		case LOCALIZED_POKEMON:{
			nombreCola = "LOCALIZED_POKEMON";
		}
		break;
		default:

		break;
	}
	return nombreCola;
}
bool checkCantidadArgumentos(process_code proc, op_code ope, int argc) {
	bool flag = false;
	switch (proc) {
	case P_BROKER:
		switch (ope) {
		case NEW_POKEMON:
			flag = argc == COUNT_ARGS_BROKER_NEW_POKEMON;
			break;

		case APPEARED_POKEMON:
			flag = argc == COUNT_ARGS_BROKER_APPEARD_POKEMON;
			break;

		case CATCH_POKEMON:
			flag = argc == COUNT_ARGS_BROKER_CATCH_POKEMON;
			break;

		case CAUGHT_POKEMON:
			flag = argc == COUNT_ARGS_BROKER_CAUGHT_POKEMON;
			break;

		case GET_POKEMON:
			flag = argc == COUNT_ARGS_BROKER_GET_POKEMON;
			break;

		default:
			flag = false;
			break;
		}
		break;
	case P_TEAM:
		switch (ope) {
		case APPEARED_POKEMON:
			flag = argc == COUNT_ARGS_TEAM_APPEARED;
			break;

		default:
			flag = false;
			break;
		}
		break;
	case P_GAMECARD:
		switch (ope) {
		case NEW_POKEMON:
			flag = argc == COUNT_ARGS_GAMECARD_NEW_POKEMON;
			break;

		case CATCH_POKEMON:
			flag = argc == COUNT_ARGS_GAMECARD_CATCH_POKEMON;
			break;

		case GET_POKEMON:
			flag = argc == COUNT_ARGS_GAMECARD_GET_POKEMON;
			break;

		default:
			flag = false;
			break;
		}
		break;
	case P_SUSCRIPTOR:
		flag = argc == COUNT_ARGS_SUSCRIBE;
		break;
	default:
		flag = false;
		break;
	}
	return flag;
}
/**
 * Parsea los argv, y los serializa en un void* listo para enviar según el protocolo
 */
t_error_codes parsearNewPokemon(process_code proc, char** argv,
		parser_result* result) {
	t_new_pokemon new_pokemon;
	//./gameboy BROKER NEW_POKEMON [POKEMON] [POSX] [POSY] [CANTIDAD]
	//./gameboy GAMECARD NEW_POKEMON [POKEMON] [POSX] [POSY] [CANTIDAD] [ID_MENSAJE]

	if (proc != P_BROKER && proc != P_GAMECARD)
		return ERROR_BAD_REQUEST;

	//por default id 0
	new_pokemon.id_mensaje = 0;

	new_pokemon.nombre_pokemon = argv[3];

	new_pokemon.pos_x = atoi(argv[4]);
	new_pokemon.pos_y = atoi(argv[5]);
	new_pokemon.cantidad = atoi(argv[6]);
	if (proc == P_GAMECARD)
		new_pokemon.id_mensaje = atoi(argv[7]);

	t_buffer* buffer = serializarNewPokemon(new_pokemon);

	result->buffer = buffer;
	return PARSE_SUCCESS;

}
t_error_codes parsearAppearedPokemon(process_code proc, char** argv,
		parser_result* result) {
	//./gameboy BROKER APPEARED_POKEMON [POKEMON] [POSX] [POSY] [ID_MENSAJE_CORRELATIVO]
	//./gameboy TEAM APPEARED_POKEMON [POKEMON] [POSX] [POSY]
	if (proc != P_BROKER && proc != P_TEAM)
		return ERROR_BAD_REQUEST;

	t_appeared_pokemon app_pokemon;

	//por default id 0
	app_pokemon.id_mensaje_correlativo = 0;

	app_pokemon.nombre_pokemon = argv[3];
	app_pokemon.pos_x = atoi(argv[4]);
	app_pokemon.pos_y = atoi(argv[5]);
	if (proc == P_BROKER)
		app_pokemon.id_mensaje_correlativo = atoi(argv[6]);

	t_buffer* buffer = serializarAppearedPokemon(app_pokemon);

	result->buffer = buffer;
	return PARSE_SUCCESS;

}
t_error_codes parsearCatchPokemon(process_code proc, char** argv,
		parser_result* result) {
	//./gameboy GAMECARD CATCH_POKEMON [POKEMON] [POSX] [POSY] [ID_MENSAJE]
	//./gameboy BROKER CATCH_POKEMON [POKEMON] [POSX] [POSY]
	if (proc != P_BROKER && proc != P_GAMECARD)
		return ERROR_BAD_REQUEST;

	t_catch_pokemon catch_pokemon;

	//por default id 0
	catch_pokemon.id_mensaje = 0;

	catch_pokemon.nombre_pokemon = argv[3];
	catch_pokemon.pos_x = atoi(argv[4]);
	catch_pokemon.pos_y = atoi(argv[5]);
	if (proc == P_GAMECARD)
		catch_pokemon.id_mensaje = atoi(argv[6]);

	t_buffer* buffer = serializarCatchPokemon(catch_pokemon);

	result->buffer = buffer;
	return PARSE_SUCCESS;

}
t_error_codes parsearCaughtPokemon(process_code proc, char** argv,
		parser_result* result) {
	//./gameboy BROKER CAUGHT_POKEMON [ID_MENSAJE_CORRELATIVO] [OK/FAIL]

	if (proc != P_BROKER)
		return ERROR_BAD_REQUEST;

	t_caught_pokemon caught_pokemon;
	caught_pokemon.id_mensaje_correlativo = atoi(argv[3]);
	caught_pokemon.atrapo_pokemon = atoi(argv[4]);

	t_buffer* buffer = serializarCaughtPokemon(caught_pokemon);

	result->buffer = buffer;
	return PARSE_SUCCESS;

}
t_error_codes parsearGetPokemon(process_code proc, char** argv,
		parser_result* result) {
	//./gameboy BROKER GET_POKEMON [POKEMON]
	//./gameboy GAMECARD GET_POKEMON [POKEMON] [ID_MENSAJE]

	if (proc != P_BROKER && proc != P_GAMECARD)
		return ERROR_BAD_REQUEST;

	t_get_pokemon get_pokemon;

	get_pokemon.id_mensaje = 0;

	get_pokemon.nombre_pokemon = argv[3];
	if (proc == P_GAMECARD) {
		get_pokemon.id_mensaje = atoi(argv[4]);
	}

	t_buffer* buffer = serializarGetPokemon(get_pokemon);

	result->buffer = buffer;
	return PARSE_SUCCESS;

}
t_error_codes parsearMsgSuscribir(int argc, char** argv, parser_result * result) {
	uint32_t timeout = (uint32_t) atoi(argv[3]);
	result->timeout = timeout;

	return PARSE_SUCCESS;
}
t_error_codes parsearMsgGeneral(process_code proc, int argc, char** argv,
		parser_result *result) {

	switch (result->msg_type) {
	case NEW_POKEMON:
		return parsearNewPokemon(proc, argv, result);
		break;
	case APPEARED_POKEMON:
		return parsearAppearedPokemon(proc, argv, result);
		break;

	case CATCH_POKEMON:
		return parsearCatchPokemon(proc, argv, result);
		break;

	case CAUGHT_POKEMON:
		return parsearCaughtPokemon(proc, argv, result);
		break;

	case GET_POKEMON:
		return parsearGetPokemon(proc, argv, result);
		break;

	default:
		logInfoAux("El argumento no es valido")
		;
		return ERROR_BAD_REQUEST;
		break;
	}
	return ERROR_BAD_REQUEST;
}
process_code getProcessCode(char* arg) {
	process_code p = P_UNKNOWN;

	if (strcmp(arg, "BROKER") == 0)
		p = P_BROKER;
	if (strcmp(arg, "TEAM") == 0)
		p = P_TEAM;
	if (strcmp(arg, "GAMECARD") == 0)
		p = P_GAMECARD;
	if (strcmp(arg, "SUSCRIPTOR") == 0)
		p = P_SUSCRIPTOR;

	return p;
}
op_code getOperationCode(char* arg) {
	op_code op = OP_UNKNOWN;

	if (strcmp(arg, "NEW_POKEMON") == 0)
		op = NEW_POKEMON;
	if (strcmp(arg, "APPEARED_POKEMON") == 0)
		op = APPEARED_POKEMON;
	if (strcmp(arg, "CATCH_POKEMON") == 0)
		op = CATCH_POKEMON;
	if (strcmp(arg, "CAUGHT_POKEMON") == 0)
		op = CAUGHT_POKEMON;
	if (strcmp(arg, "GET_POKEMON") == 0)
		op = GET_POKEMON;
	if (strcmp(arg, "LOCALIZED_POKEMON") == 0)
		op = LOCALIZED_POKEMON;

	return op;
}
t_error_codes parsearComando(int argc, char** argv, parser_result* result) {
	if (argc < 4) {
		logInfo("Revisar los argumentos ingresados.");
		logInfoAux("No se proporcionaron la cantidad minima de argumentos");
		return ERROR_BAD_ARGUMENTS_QUANTITY;
	}
	process_code p_code = getProcessCode(argv[1]);
	if (p_code == P_UNKNOWN) {
		logInfoAux("El argumento proceso no es correcto");
		return ERROR_BAD_REQUEST;
	}

	op_code op_code = getOperationCode(argv[2]);
	if (op_code == OP_UNKNOWN) {
		logInfoAux("El tipo de operación no es correcto");
		return ERROR_BAD_REQUEST;
	}
	result->module = p_code;
	result->msg_type = op_code;

	if (!checkCantidadArgumentos(p_code, op_code, argc)) {
		logInfoAux("La cantidad de argumentos no es válida");
		return ERROR_BAD_REQUEST;
	}
	switch (p_code) {
	case P_SUSCRIPTOR:
		return parsearMsgSuscribir(argc, argv, result);
		break;
	case P_BROKER:
	case P_GAMECARD:
	case P_TEAM:
		return parsearMsgGeneral(result->module, argc, argv, result);

		break;
	default:
		return ERROR_BAD_REQUEST;
		break;
	}

	return ERROR_BAD_REQUEST;
}
t_error_codes enviarMensajeAModulo(process_code proc, op_code ope, t_buffer* buffer) {
	//Levanto el socket
	t_config* config = config_create("../gameBoy.config");
	if (config == NULL) {
		logInfoAux("No se puede leer el archivo de configuración de Game Boy");
		return ERROR_CONFIG_FILE;
	}
	char* ipServidor = "";
	char* puertoServidor = "";
	char* nombreModulo = "";
	if (proc == P_BROKER) {
		ipServidor = config_get_string_value(config, "IP_BROKER");
		puertoServidor = config_get_string_value(config, "PUERTO_BROKER");
		nombreModulo = "BROKER";
	}
	if (proc == P_TEAM) {
		ipServidor = config_get_string_value(config, "IP_TEAM");
		puertoServidor = config_get_string_value(config, "PUERTO_TEAM");
		nombreModulo = "TEAM";
	}
	if (proc == P_GAMECARD) {
		ipServidor = config_get_string_value(config, "IP_GAMECARD");
		puertoServidor = config_get_string_value(config, "PUERTO_GAMECARD");
		nombreModulo = "GAMECARD";
	}
	if (strcmp(ipServidor, "") == 0 || strcmp(puertoServidor, "") == 0) return ERROR_CONFIG_FILE;

	int gameBoyBroker = crearSocketCliente(ipServidor, puertoServidor);
	if (gameBoyBroker != -1) {
		logInfo("Conexión a %s %s : %s.",nombreModulo,ipServidor,puertoServidor);
		t_paquete* paquete = malloc(sizeof(t_paquete));
		paquete->codigo_operacion = ope;
		paquete->tipo_proceso = P_GAMEBOY;
		paquete->id_proceso = (uint32_t)config_get_int_value(config,"ID_PROCESO");
		paquete->buffer = buffer;

		int tamanio_a_enviar;
		void* mensajeSerializado = serializarPaquete(paquete, &tamanio_a_enviar);
		int enviado = send(gameBoyBroker, mensajeSerializado, tamanio_a_enviar, 0);
		if (enviado == -1) {
			logInfoAux("No se envió el mensaje");
			free(mensajeSerializado);
			eliminarPaquete(paquete);
			config_destroy(config);
			close(gameBoyBroker);
			return ERROR_SEND;
		} else {
			logInfoAux("Se enviaron %d bytes a la cola %s", enviado,getNombreCola(paquete->codigo_operacion));
			//recibir id y loguear
			uint32_t id_o_ack;

			recv(gameBoyBroker,&id_o_ack,sizeof(uint32_t),MSG_WAITALL);
			if(strcmp("BROKER",nombreModulo) == 0){
				logInfoAux("Se recibió el ID_MENSAJE: %d",id_o_ack);
			}else{
				logInfoAux("Se recibió ACK: %d",id_o_ack);
			}

			free(mensajeSerializado);
			eliminarPaquete(paquete);
			close(gameBoyBroker);
			config_destroy(config);
			return PARSE_SUCCESS;
		}
	} else {
		logInfo("Fallo de Conexión con %s.", nombreModulo);
		config_destroy(config);
		return ERROR_SEND;
	}
}
void recibirMensajes(int socket){
	int socket_cliente = socket;
	bool caseDefault;
	int enviado;
	bool seguir = true;
	while(seguir){
		caseDefault = false;
		enviado = -1;
		op_code cod_op = recibirOperacion(socket_cliente);
		recibirTipoProceso(socket_cliente);
		recibirIDProceso(socket_cliente);
		logInfoAux("cod_op %d",cod_op);
		switch(cod_op){
			case NEW_POKEMON:{
				t_new_pokemon* new_pokemon = recibirNewPokemon(socket_cliente);
				logInfo("Llegó un Mensaje NEW_POKEMON %s %d %d %d.",new_pokemon->nombre_pokemon,new_pokemon->pos_x,new_pokemon->pos_y,new_pokemon->cantidad);
				break;
			}
			case APPEARED_POKEMON:{
				t_appeared_pokemon* appeared_pokemon = recibirAppearedPokemon(socket_cliente);
				if(appeared_pokemon->id_mensaje_correlativo > 0){
					logInfo("Llegó un Mensaje APPEARED_POKEMON %s %d %d %d.",appeared_pokemon->nombre_pokemon,appeared_pokemon->pos_x,appeared_pokemon->pos_y, appeared_pokemon->id_mensaje_correlativo);
				}else{
					// 3. Llegada de un nuevo mensaje a una cola de mensajes.
					logInfo("Llegó un Mensaje APPEARED_POKEMON %s %d %d.",appeared_pokemon->nombre_pokemon,appeared_pokemon->pos_x,appeared_pokemon->pos_y);
				}
				break;
			}
			case CATCH_POKEMON:{
				t_catch_pokemon* catch_pokemon = recibirCatchPokemon(socket_cliente);
				logInfo("Llegó un Mensaje CATCH_POKEMON %s %d %d.",catch_pokemon->nombre_pokemon,catch_pokemon->pos_x,catch_pokemon->pos_y);

				break;
			}
			case CAUGHT_POKEMON:{
				t_caught_pokemon* caught_pokemon = recibirCaughtPokemon(socket_cliente);
				if(caught_pokemon->id_mensaje_correlativo > 0){
					logInfo("Llegó un Mensaje CAUGHT_POKEMON %d %d.",caught_pokemon->id_mensaje_correlativo,caught_pokemon->atrapo_pokemon);
				}else{
					// 3. Llegada de un nuevo mensaje a una cola de mensajes.
					logInfo("Llegó un Mensaje CAUGHT_POKEMON %d.",caught_pokemon->atrapo_pokemon);
				}
				break;
			}
			case GET_POKEMON:{
				t_get_pokemon* get_pokemon = recibirGetPokemon(socket_cliente);
				logInfo("Llegó un Mensaje GET_POKEMON %s.",get_pokemon->nombre_pokemon);
				break;
			}
			case LOCALIZED_POKEMON:{
				t_localized_pokemon* localized_pokemon = recibirLocalizedPokemon(socket_cliente);
				logInfo("Llegó un Mensaje LOCALIZED_POKEMON %s %d.",localized_pokemon->nombre_pokemon,localized_pokemon->cant_pos);
				break;
			}
			default:{
				caseDefault = true;
				logInfo("Finaliza la suscripción del Game Boy.");
				logInfoAux("No se pudo procesar un mensaje o se desconectó el BROKER.");
				break;
			}
		}
		if(caseDefault == false){
			enviado = enviarACK(socket_cliente);
			if(enviado != -1){
				logInfoAux("Se envió el ACK");
			}else{
				logInfoAux("Error al enviar el ACK");
			}
		}else{
			seguir = false;
		}
	}
}
t_error_codes suscribirse(parser_result result) {
	//Levanto el socket
	t_config* config = config_create("../gameBoy.config");
	if (config == NULL) {
		logInfoAux("No se puede leer el archivo de configuración de Game Boy");
		return ERROR_CONFIG_FILE;
	}
	char* ipServidor = "";
	char* puertoServidor = "";

	ipServidor = config_get_string_value(config, "IP_BROKER");
	puertoServidor = config_get_string_value(config, "PUERTO_BROKER");

	if (strcmp(ipServidor, "") == 0 || strcmp(puertoServidor, "") == 0)
		return ERROR_CONFIG_FILE;

	int gameBoyBroker = crearSocketCliente(ipServidor, puertoServidor);
	if (gameBoyBroker != -1) {
		logInfoAux("Enviando suscripción a Broker %s:%s en socket %d",ipServidor,puertoServidor,gameBoyBroker);
		t_suscribe msgSuscribe = { SUSCRIBE_GAMEBOY, result.msg_type, (uint32_t)config_get_int_value(config,"ID_PROCESO"), result.timeout };
		int enviado = enviarSuscripcion(gameBoyBroker, msgSuscribe);

		if (enviado == -1) {
			logInfoAux("No se envió el mensaje");
			close(gameBoyBroker);
			return ERROR_SEND;
		} else {
			logInfo("Se suscribe el Game Boy %d a la cola %s.", (uint32_t)config_get_int_value(config,"ID_PROCESO"), getNombreCola(result.msg_type),enviado);
			//Recibir mensajes en lo que dura el timeout (en segundos)
			recibirMensajes(gameBoyBroker);
			/*
			pthread_t hiloEscucha;
			pthread_create(&hiloEscucha, NULL, (void*)recibirMensajes, (int*)gameBoyBroker);
			pthread_detach(hiloEscucha);
*/
			return PARSE_SUCCESS;
		}

	} else {
		logInfo("Fallo de Conexión con Broker %s : %s", ipServidor, puertoServidor);
		logInfoAux("Conexión FALLIDA a Broker %s:%s",ipServidor,puertoServidor);
		return ERROR_SEND;
	}
	config_destroy(config);

}
int main(int argc, char** argv) {
	iniciarLogger("../gameBoy.log", "GAMEBOY", true, LOG_LEVEL_TRACE);
	iniciarLoggerAux("../gameBoyAuxiliar.log", "GAMEBOY AUX", true,
			LOG_LEVEL_TRACE);
	
	parser_result result;
	t_error_codes p = parsearComando(argc, argv, &result);

	if (p != PARSE_SUCCESS) {
		logInfo("Error al parsear");
	} else {
		switch (result.module) {
		case P_SUSCRIPTOR:
			p = suscribirse(result);
			break;
		case P_BROKER:
		case P_TEAM:
		case P_GAMECARD:
			p = enviarMensajeAModulo(result.module, result.msg_type,
					result.buffer);
			break;
		default:
			p = ERROR_BAD_REQUEST;
			break;
		}
	}
	destruirLogger();
	destruirLoggerAux();
	return p;
}
