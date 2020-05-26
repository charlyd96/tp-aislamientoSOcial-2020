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
	//Argumentos ["gameboy","PROCESO","NEW_POKEMON","NOMBRE","POSX","POSY","CANTIDAD"]

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
t_parser_error_codes parsearMsgBroker(int argc,char** argv,parser_result *result){
	t_parser_error_codes err = PARSE_SUCCESS;

	switch (result->msg_type)
	    {
	    case NEW_POKEMON:
	    	if(argc != COUNT_ARGS_BROKER_NEW_POKEMON){
	    		printf("La cantidad de argumentos no es válida\n");
	    		err = ERROR_BAD_ARGUMENTS_QUANTITY;
	    	}else{
				//Envío P_BROKER para manejar el caso especial de newPokemon para gameCard (lleva un id de más)
				return parsearNewPokemon(P_BROKER, argv,result);
	    	}
		break;

	    case APPEARED_POKEMON:
	    	if(argc != COUNT_ARGS_BROKER_APPEARD_POKEMON){
				printf("La cantidad de argumentos no es válida\n");
				err = ERROR_BAD_ARGUMENTS_QUANTITY;
			}
	        break;

	    case CATCH_POKEMON:
	    	if(argc != COUNT_ARGS_BROKER_CATCH_POKEMON){
				printf("La cantidad de argumentos no es válida\n");
				err = ERROR_BAD_ARGUMENTS_QUANTITY;
			}
	        break;

	    case CAUGHT_POKEMON:
	    	if(argc != COUNT_ARGS_BROKER_CAUGHT_POKEMON){
				printf("La cantidad de argumentos no es válida\n");
				err = ERROR_BAD_ARGUMENTS_QUANTITY;
			}
	        break;

	    case GET_POKEMON:
	    	if(argc != COUNT_ARGS_BROKER_GET_POKEMON){
				printf("La cantidad de argumentos no es válida\n");
				err = ERROR_BAD_ARGUMENTS_QUANTITY;
			}
	        break;

	    default:
	    	printf("El argumento no es valido\n");
	    	err = ERROR_BAD_REQUEST;
		break;
	    }
	return err;
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
		printf("La cantidad de argumentos no es válida");
		return ERROR_BAD_REQUEST;
	}
	printf("comando %s\n",argv[1]);
    switch(p_code){

		case P_SUSCRIPTOR:

			break;
		case P_BROKER:
			return parsearMsgBroker(argc,argv,result);

			break;
		case P_GAMECARD:


			break;
		case P_TEAM:

			break;
		default:

			break;
    }

    return ERROR_BAD_REQUEST;
}
void enviarMensajeAModulo(int socket,op_code ope,t_buffer* buffer){
	t_paquete* paquete = malloc(sizeof(t_paquete));
	paquete->codigo_operacion = ope;
	paquete->buffer = buffer;
	int tamanio_a_enviar;
	void* mensajeSerializado = serializarPaquete(paquete, &tamanio_a_enviar);
	int enviado = send(socket, mensajeSerializado, tamanio_a_enviar, 0);

	if(enviado == -1){
		printf("No se envió el mensaje serializado");
	}else{
		printf("Se conectó y se envía el mensaje");
	}

	free(mensajeSerializado);
	eliminarPaquete(paquete);

}
int main(int argc, char** argv){
//-------Levantar config---------------
	t_config* config = config_create("./gameBoy.config");
	char* ipServidorBroker = config_get_string_value(config, "IP_BROKER");
	char* puertoServidorBroker = config_get_string_value(config, "PUERTO_BROKER");
	char* ipServidorTeam = config_get_string_value(config, "IP_TEAM");
	char* puertoServidorTeam = config_get_string_value(config, "PUERTO_TEAM");

	if(config == NULL){
		printf("No se puede leer el archivo de configuración de Game Boy\n");
	}else{
		printf("Leí la IP %s del PUERTO %s\n", ipServidorBroker, puertoServidorBroker);
	}
//------------------------------------
	printf("El comando es %s",argv[1]);
	parser_result result;
	t_parser_error_codes p = parsearComando(argc,argv, &result);
	if(p != PARSE_SUCCESS){
		printf("Error al parsear\n");
		return ERROR_BAD_REQUEST;
	}
	else{
		int gameBoyBroker = crearSocketCliente(ipServidorBroker, puertoServidorBroker);
		switch(result.module){
		case P_BROKER:

			enviarMensajeAModulo(gameBoyBroker,result.msg_type,result.buffer);
			break;
		}
	}

/*
	t_new_pokemon pokemon;
		pokemon.nombre_pokemon = "Pikachu";
		pokemon.pos_x = 4;
		pokemon.pos_y = 5;
		pokemon.cantidad = 2;

	int gameBoyBroker = crearSocketCliente(ipServidorBroker, puertoServidorBroker);

	enviarNewPokemon(gameBoyBroker, pokemon);

 	PARA QUE EL SERVIDOR TEAM RECIBA EL MENSAJE HABRÁ QUE HACER UN SWITCH
 	t_appeared_pokemon pokemon1;
	pokemon1.nombre_pokemon = "Nidoran";
	pokemon1.pos_x = 10;
	pokemon1.pos_y = 3;

	int gameBoyTeam = crearSocketCliente(ipServidorTeam, puertoServidorTeam);
	printf("Socket Cliente %d\n", gameBoyTeam);

	enviarAppearedPokemon(gameBoyTeam, pokemon1); */
}
