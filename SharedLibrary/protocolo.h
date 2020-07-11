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
	NEW_POKEMON,
	APPEARED_POKEMON,
	CATCH_POKEMON,
	CAUGHT_POKEMON,
	GET_POKEMON,
	LOCALIZED_POKEMON,
	SUSCRIBE_TEAM,
	SUSCRIBE_GAMECARD,
	SUSCRIBE_GAMEBOY,
	OP_UNKNOWN
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

typedef struct {
    op_code msg_type;
    process_code module;
    t_buffer* buffer;
    uint32_t timeout;
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
} t_catch_pokemon;

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
	char* posiciones; //Lista de posiciones con el formato de los .config "[2|1,1|1]"
	uint32_t id_mensaje_correlativo;
} t_localized_pokemon;

typedef struct{
	op_code tipo_suscripcion;
	op_code cola_suscribir;
	uint32_t id_proceso;
	uint32_t timeout; //Solo en caso de gameboy
} t_suscribe;
#endif /* PROTOCOLO_H_ */
