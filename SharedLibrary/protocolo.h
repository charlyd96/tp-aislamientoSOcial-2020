/*
 * protocolo.h
 *
 *  Created on: 24 abr. 2020
 *      Author: aislamientoSOcial
 */

#ifndef PROTOCOLO_H_
#define PROTOCOLO_H_

#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <netdb.h>
#include <string.h>

pthread_t thread;

typedef enum {
	NEW_POKEMON = 1,
	APPEARED_POKEMON = 2,
	CATCH_POKEMON = 3,
	CAUGHT_POKEMON = 4,
	GET_POKEMON = 5,
	LOCALIZED_POKEMON = 6,
	OP_UNKNOWN = 7
} op_code;
typedef enum {
	P_BROKER,
	P_TEAM,
	P_GAMECARD,
	P_UNKNOWN,
	P_SUSCRIPTOR
} process_code;

typedef struct {
	int size;
	void* stream;
} t_buffer;

typedef struct
{
    op_code 	 msg_type;
    process_code module;
    t_buffer*        buffer;
    uint32_t     timeout;
} parser_result;

typedef struct {
	op_code codigo_operacion;
	t_buffer* buffer;
} t_paquete;

typedef struct {
	char* nombre_pokemon;
	uint32_t pos_x;
	uint32_t pos_y;
	uint32_t cantidad;
	uint32_t id_mensaje;
} t_new_pokemon;

typedef struct {
	char* nombre_pokemon;
	uint32_t id_mensaje;
} t_get_pokemon;
typedef struct {
	char* nombre_pokemon;
	uint32_t pos_x;
	uint32_t pos_y;
	uint32_t id_mensaje;
}t_catch_pokemon;

typedef struct {
	char* nombre_pokemon;
	uint32_t pos_x;
	uint32_t pos_y;
	uint32_t id_mensaje_correlativo;
} t_appeared_pokemon;

typedef struct {
	uint32_t atrapo_pokemon;
	uint32_t id_mensaje_correlativo;
} t_caught_pokemon;

typedef struct {
	char* nombre_pokemon;
	uint32_t cant_pos;
	uint32_t pos_x;
	uint32_t pos_y;
	uint32_t id_mensaje_correlativo;
} t_localized_pokemon;

#endif /* PROTOCOLO_H_ */
