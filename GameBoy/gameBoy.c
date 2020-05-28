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
#include <commons/config.h>

bool checkCantidadArgumentos(process_code proc,op_code ope, int argc){
	bool flag = false;
	 switch(proc)
	    {
	    case P_BROKER:
	        switch(ope)
	        {
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
	        switch(ope)
	        {
				case APPEARED_POKEMON:
					flag = argc == COUNT_ARGS_TEAM_APPEARED;
					break;

				default:
					flag = false;
					break;
	        }
            break;
		case P_GAMECARD:
			switch (ope)
			{
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

	    default:
	        flag = false;
            break;
	}
	 return flag;
}
/**
 * Parsea los argv, y los serializa en un void* listo para enviar según el protocolo
 */
t_parser_error_codes parsearNewPokemon(process_code proc,char** argv,parser_result* result){
	t_new_pokemon new_pokemon;
	//./gameboy BROKER NEW_POKEMON [POKEMON] [POSX] [POSY] [CANTIDAD]
	//./gameboy GAMECARD NEW_POKEMON [POKEMON] [POSX] [POSY] [CANTIDAD] [ID_MENSAJE]

	if(proc != P_BROKER && proc != P_GAMECARD) return ERROR_BAD_REQUEST;

	//por default id 0
	new_pokemon.id = 0;

	new_pokemon.nombre_pokemon = argv[3];

	new_pokemon.pos_x = atoi(argv[4]);
	new_pokemon.pos_y = atoi(argv[5]);
	new_pokemon.cantidad = atoi(argv[6]);
	if(proc == P_GAMECARD) new_pokemon.id = atoi(argv[7]);

	t_buffer* buffer = serializarNewPokemon(new_pokemon);

	result->buffer = buffer;
	return PARSE_SUCCESS;

}
t_parser_error_codes parsearAppearedPokemon(process_code proc,char** argv,parser_result* result){
	//./gameboy BROKER APPEARED_POKEMON [POKEMON] [POSX] [POSY] [ID_MENSAJE_CORRELATIVO]
	//./gameboy TEAM APPEARED_POKEMON [POKEMON] [POSX] [POSY]
	if(proc != P_BROKER && proc != P_TEAM) return ERROR_BAD_REQUEST;

	t_appeared_pokemon app_pokemon;

	//por default id 0
	app_pokemon.id = 0;

	app_pokemon.nombre_pokemon = argv[3];
	app_pokemon.pos_x = atoi(argv[4]);
	app_pokemon.pos_y = atoi(argv[5]);
	if(proc == P_BROKER) app_pokemon.id = atoi(argv[6]);

	t_buffer* buffer = serializarAppearedPokemon(app_pokemon);

	result->buffer = buffer;
	return PARSE_SUCCESS;

}
t_parser_error_codes parsearCatchPokemon(process_code proc,char** argv,parser_result* result){
	//./gameboy GAMECARD CATCH_POKEMON [POKEMON] [POSX] [POSY] [ID_MENSAJE]
	//./gameboy BROKER CATCH_POKEMON [POKEMON] [POSX] [POSY]
	if(proc != P_BROKER && proc != P_GAMECARD) return ERROR_BAD_REQUEST;

	t_catch_pokemon catch_pokemon;

	//por default id 0
	catch_pokemon.id = 0;

	catch_pokemon.nombre_pokemon = argv[3];
	catch_pokemon.pos_x = atoi(argv[4]);
	catch_pokemon.pos_y = atoi(argv[5]);
	if(proc == P_GAMECARD) catch_pokemon.id = atoi(argv[6]);

	t_buffer* buffer = serializarCatchPokemon(catch_pokemon);

	result->buffer = buffer;
	return PARSE_SUCCESS;

}
t_parser_error_codes parsearCaughtPokemon(process_code proc,char** argv,parser_result* result){
	//./gameboy BROKER CAUGHT_POKEMON [ID_MENSAJE_CORRELATIVO] [OK/FAIL]

	if(proc != P_BROKER) return ERROR_BAD_REQUEST;

	t_caught_pokemon caught_pokemon;

	caught_pokemon.id = atoi(argv[3]);
	caught_pokemon.atrapo_pokemon = atoi(argv[4]);

	t_buffer* buffer = serializarCaughtPokemon(caught_pokemon);

	result->buffer = buffer;
	return PARSE_SUCCESS;

}
t_parser_error_codes parsearGetPokemon(process_code proc,char** argv,parser_result* result){
	//./gameboy BROKER GET_POKEMON [POKEMON]
	//./gameboy GAMECARD GET_POKEMON [POKEMON] [ID_MENSAJE]

	if(proc != P_BROKER && proc != P_GAMECARD) return ERROR_BAD_REQUEST;

	t_get_pokemon get_pokemon;

	get_pokemon.id = 0;

	get_pokemon.nombre_pokemon = argv[3];
	if(proc == P_GAMECARD){
		get_pokemon.id = atoi(argv[4]);
	}

	t_buffer* buffer = serializarGetPokemon(get_pokemon);

	result->buffer = buffer;
	return PARSE_SUCCESS;

}
t_parser_error_codes parsearMsgGeneral(process_code proc,int argc,char** argv,parser_result *result){

	switch (result->msg_type)
	    {
	    case NEW_POKEMON:
			return parsearNewPokemon(proc, argv,result);
		break;

	    	return parsearAppearedPokemon(proc,argv,result);
		break;

	    case CATCH_POKEMON:
	    	return parsearCatchPokemon(proc,argv,result);
	        break;

	    case CAUGHT_POKEMON:
	    	return parsearCaughtPokemon(proc,argv,result);
	        break;

	    case GET_POKEMON:
	    	return parsearGetPokemon(proc,argv,result);
	        break;

	    default:
	    	printf("El argumento no es valido\n");
	    	return ERROR_BAD_REQUEST;
		break;
	    }
	return ERROR_BAD_REQUEST;
}
process_code getProcessCode(char* arg){
	process_code p = P_UNKNOWN;

	if(strcmp(arg,"BROKER") == 0) 	 p = P_BROKER;
	if(strcmp(arg,"TEAM") == 0)  	 p = P_TEAM;
	if(strcmp(arg,"GAMECARD") == 0)	 p = P_GAMECARD;
	if(strcmp(arg,"SUSCRIPTOR") ==0) p = P_SUSCRIPTOR;

	return p;
}
op_code getOperationCode(char* arg){
	op_code op = OP_UNKNOWN;

	if(strcmp(arg,"NEW_POKEMON") == 0) 	 		op = NEW_POKEMON;
	if(strcmp(arg,"APPEARED_POKEMON") == 0)  	op = APPEARED_POKEMON;
	if(strcmp(arg,"CATCH_POKEMON") == 0)	 	op = CATCH_POKEMON;
	if(strcmp(arg,"CAUGHT_POKEMON") == 0) 		op = CAUGHT_POKEMON;
	if(strcmp(arg,"GET_POKEMON") == 0)  	 	op = GET_POKEMON;
	if(strcmp(arg,"LOCALIZED_POKEMON") == 0)	op = LOCALIZED_POKEMON;

	return op;
}
t_parser_error_codes parsearComando(int argc, char** argv, parser_result* result)
{
    if(argc < 4)
    {
        printf("No se proporcionaron la cantidad minima de argumentos\n");
    }
    process_code p_code = getProcessCode(argv[1]);
    if(p_code == P_UNKNOWN){
		printf("El argumento proceso no es correcto\n");
    	return ERROR_BAD_REQUEST;
    }

    op_code op_code = getOperationCode(argv[2]);
	if(op_code == OP_UNKNOWN){
		printf("El tipo de operación no es correcto\n");
		return ERROR_BAD_REQUEST;
	}
	result->module 		= p_code;
	result->msg_type 	= op_code;

	if(!checkCantidadArgumentos(p_code,op_code,argc)){
		printf("La cantidad de argumentos no es válida\n");
		return ERROR_BAD_REQUEST;
	}
	printf("comando %s\n",argv[1]);
    switch(p_code){

		case P_SUSCRIPTOR:

			break;
		case P_BROKER:
		case P_GAMECARD:
		case P_TEAM:
			return parsearMsgGeneral(result->module,argc,argv,result);

			break;
		default:
			return ERROR_BAD_REQUEST;
			break;
    }

    return ERROR_BAD_REQUEST;
}
t_parser_error_codes enviarMensajeAModulo(process_code proc,op_code ope,t_buffer* buffer){
	//Levanto el socket
	t_config* config = config_create("./gameBoy.config");
	if(config == NULL){
		printf("No se puede leer el archivo de configuración de Game Boy\n");
		return ERROR_CONFIG_FILE;
	}
	char* ipServidor = "";
	char* puertoServidor ="";
	if(proc == P_BROKER){
		ipServidor = config_get_string_value(config, "IP_BROKER");
		puertoServidor = config_get_string_value(config, "PUERTO_BROKER");
	}
	if(proc == P_TEAM){
		ipServidor = config_get_string_value(config, "IP_TEAM");
		puertoServidor = config_get_string_value(config, "PUERTO_TEAM");

	}
	if(strcmp(ipServidor,"")==0 || strcmp(puertoServidor,"") == 0) return ERROR_CONFIG_FILE;

	int gameBoyBroker = crearSocketCliente(ipServidor, puertoServidor);

	t_paquete* paquete = malloc(sizeof(t_paquete));
	paquete->codigo_operacion = ope;
	paquete->buffer = buffer;
	int tamanio_a_enviar;
	void* mensajeSerializado = serializarPaquete(paquete, &tamanio_a_enviar);
	int enviado = send(gameBoyBroker, mensajeSerializado, tamanio_a_enviar, 0);

	if(enviado == -1){
		printf("No se envió el mensaje serializado\n");
		return ERROR_SEND;
	}else{
		printf("Se conectó y se enviaron %d bytes\n",enviado);
		return PARSE_SUCCESS;
	}

	free(mensajeSerializado);
	eliminarPaquete(paquete);

}
int main(int argc, char** argv){

	parser_result result;
	t_parser_error_codes p = parsearComando(argc,argv, &result);
	if(p != PARSE_SUCCESS){
		printf("Error al parsear\n");
		return p;
	}
	else{
		switch(result.module){
		case P_SUSCRIPTOR:
			printf("suscripcion aún no desarrollada\n");
		break;
		case P_BROKER:
		case P_TEAM:
		case P_GAMECARD:
			enviarMensajeAModulo(result.module,result.msg_type,result.buffer);
		break;
		default:
			return ERROR_BAD_REQUEST;
		break;
		}
	}
}
