/*
 * gameBoy.h
 *
 *  Created on: 25 may. 2020
 *      Author: utnso
 */

#ifndef GAMEBOY_H_
#define GAMEBOY_H_

//Broker
#define COUNT_ARGS_BROKER_NEW_POKEMON         7
#define COUNT_ARGS_BROKER_APPEARD_POKEMON     7
#define COUNT_ARGS_BROKER_CATCH_POKEMON       6
#define COUNT_ARGS_BROKER_CAUGHT_POKEMON      5
#define COUNT_ARGS_BROKER_GET_POKEMON         4

//Team
#define COUNT_ARGS_TEAM_APPEARED   6

//GameCard
#define COUNT_ARGS_GAMECARD_NEW_POKEMON       8
#define COUNT_ARGS_GAMECARD_CATCH_POKEMON     7
#define COUNT_ARGS_GAMECARD_GET_POKEMON       5

#define COUNT_ARGS_SUSCRIBE      4

typedef enum{
    PARSE_SUCCESS,
    ERROR_BAD_ARGUMENTS_QUANTITY,
    ERROR_BAD_REQUEST,
    ERROR_BAD_TYPE_PARSE,
	ERROR_CONFIG_FILE,
	ERROR_SEND
}t_error_codes;

char* getNombreCola(op_code cola);

#endif /* GAMEBOY_H_ */
