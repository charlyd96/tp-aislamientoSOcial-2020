/*
 * serializacion.c
 *
 *  Created on: 24 abr. 2020
 *      Author: aislamientoSOcial
 */

#include "serializacion.h"

void* serializarPaquete(t_paquete* paquete, int *bytes)
{
	int largo = sizeof(op_code) + sizeof(process_code) + 2 * sizeof(uint32_t) + paquete->buffer->size;
	int offset = 0;
	void * serializado = malloc(largo);

	memcpy(serializado + offset, &(paquete->codigo_operacion), sizeof(paquete->codigo_operacion));
	offset += sizeof(paquete->codigo_operacion);
	memcpy(serializado + offset, &(paquete->tipo_proceso), sizeof(paquete->tipo_proceso));
	offset += sizeof(paquete->tipo_proceso);
	memcpy(serializado + offset, &(paquete->id_proceso), sizeof(paquete->id_proceso));
	offset += sizeof(paquete->id_proceso);
	memcpy(serializado + offset, &(paquete->buffer->size), sizeof(paquete->buffer->size));
	offset += sizeof(paquete->buffer->size);
	memcpy(serializado + offset, paquete->buffer->stream, paquete->buffer->size);
	offset += paquete->buffer->size;

	(*bytes) = largo;
	return serializado;
}
/**
 * El campo id_mensaje debe iniciarse en 0 en caso de no querer serializarlo
 */
t_buffer* serializarNewPokemon(t_new_pokemon mensaje){
	t_buffer* buffer = malloc(sizeof(t_buffer));

	uint32_t largo_nombre  = strlen(mensaje.nombre_pokemon) + 1;
	buffer->size = 4* sizeof(uint32_t) + largo_nombre;
	if(mensaje.id_mensaje != 0) buffer->size += sizeof(uint32_t);

	void* stream = malloc(buffer->size);

	int offset = 0;

	memcpy(stream + offset, &(largo_nombre), sizeof(uint32_t));
	offset += sizeof(uint32_t);

	memcpy(stream + offset, mensaje.nombre_pokemon, largo_nombre);
	offset += largo_nombre;

	memcpy(stream + offset, &(mensaje.pos_x), sizeof(uint32_t));
	offset += sizeof(uint32_t);

	memcpy(stream + offset, &(mensaje.pos_y), sizeof(uint32_t));
	offset += sizeof(uint32_t);

	memcpy(stream + offset, &(mensaje.cantidad), sizeof(uint32_t));
	offset += sizeof(uint32_t);

	//Si se cargó un id != 0, se serializa
	if(mensaje.id_mensaje != 0){
		memcpy(stream + offset, &(mensaje.id_mensaje), sizeof(uint32_t));
		offset += sizeof(uint32_t);
	}

	buffer->stream = stream;
	return buffer;
}
/**
 * el campo id_mensaje_correlativo debe iniciarse en 0 en caso de no querer serializarlo
 */
t_buffer* serializarAppearedPokemon(t_appeared_pokemon mensaje){
	t_buffer* buffer = malloc(sizeof(t_buffer));
	uint32_t largo_nombre  = strlen(mensaje.nombre_pokemon) + 1;

	buffer->size = 3* sizeof(uint32_t) + largo_nombre;
	if(mensaje.id_mensaje_correlativo != 0) buffer->size += sizeof(uint32_t);

	void* stream = malloc(buffer->size);

	int offset = 0;

	memcpy(stream + offset, &(largo_nombre), sizeof(uint32_t));
	offset += sizeof(uint32_t);

	memcpy(stream + offset, mensaje.nombre_pokemon, largo_nombre);
	offset += largo_nombre;

	memcpy(stream + offset, &(mensaje.pos_x), sizeof(uint32_t));
	offset += sizeof(uint32_t);

	memcpy(stream + offset, &(mensaje.pos_y), sizeof(uint32_t));
	offset += sizeof(uint32_t);

	if(mensaje.id_mensaje_correlativo != 0){
		memcpy(stream + offset, &(mensaje.id_mensaje_correlativo), sizeof(uint32_t));
		offset += sizeof(uint32_t);
	}

	buffer->stream = stream;

	return buffer;
}
t_buffer* serializarCatchPokemon(t_catch_pokemon mensaje){
	t_buffer* buffer = malloc(sizeof(t_buffer));
	uint32_t largo_nombre  = strlen(mensaje.nombre_pokemon) + 1;

	buffer->size = 3* sizeof(uint32_t) + largo_nombre;
	if(mensaje.id_mensaje != 0) buffer->size += sizeof(uint32_t);

	void* stream = malloc(buffer->size);

	int offset = 0;

	memcpy(stream + offset, &(largo_nombre), sizeof(uint32_t));
	offset += sizeof(uint32_t);

	memcpy(stream + offset, mensaje.nombre_pokemon, largo_nombre);
	offset += largo_nombre;

	memcpy(stream + offset, &(mensaje.pos_x), sizeof(uint32_t));
	offset += sizeof(uint32_t);

	memcpy(stream + offset, &(mensaje.pos_y), sizeof(uint32_t));
	offset += sizeof(uint32_t);

	//Ultimo id en caso de enviarse al gamecard
	if(mensaje.id_mensaje != 0){
		memcpy(stream + offset, &(mensaje.id_mensaje), sizeof(uint32_t));
		offset += sizeof(uint32_t);
	}

	buffer->stream = stream;
	return buffer;
}
t_buffer* serializarCaughtPokemon(t_caught_pokemon mensaje){
	t_buffer* buffer = malloc(sizeof(t_buffer));
	buffer->size = 2* sizeof(uint32_t);

	void* stream = malloc(buffer->size);
	int offset = 0;

	memcpy(stream + offset, &(mensaje.atrapo_pokemon), sizeof(uint32_t));
	offset += sizeof(uint32_t);

	memcpy(stream + offset, &(mensaje.id_mensaje_correlativo), sizeof(uint32_t));
	offset += sizeof(uint32_t);

	buffer->stream = stream;
	return buffer;
}
/**
 * inicializar id_mensaje en 0 en caso de no enviarse
 */
t_buffer* serializarGetPokemon(t_get_pokemon mensaje){
	t_buffer* buffer = malloc(sizeof(t_buffer));
	uint32_t largo_nombre  = strlen(mensaje.nombre_pokemon) + 1;

	buffer->size = sizeof(uint32_t) + largo_nombre;
	if(mensaje.id_mensaje != 0) buffer->size += sizeof(uint32_t);

	void* stream = malloc(buffer->size);
	int offset = 0;

	memcpy(stream + offset, &(largo_nombre), sizeof(uint32_t));
	offset += sizeof(uint32_t);

	memcpy(stream + offset, mensaje.nombre_pokemon, largo_nombre);
	offset += largo_nombre;

	if(mensaje.id_mensaje != 0){
		memcpy(stream + offset, &(mensaje.id_mensaje), sizeof(uint32_t));
		offset += sizeof(uint32_t);
	}

	buffer->stream = stream;
	return buffer;
}
/**
 * inicializar id_mensaje_correlativo en 0 en caso de no enviarse
 * (flujo Broker -> Team)
 */
t_buffer* serializarLocalizedPokemon(t_localized_pokemon mensaje){
	t_buffer* buffer = malloc(sizeof(t_buffer));
	uint32_t largo_nombre  = (uint32_t)strlen(mensaje.nombre_pokemon) + 1;
	uint32_t i = 0;
	uint32_t pos_x, pos_y;
	buffer->size = 2* sizeof(uint32_t) + largo_nombre + 2*(sizeof(uint32_t) * mensaje.cant_pos);

	if(mensaje.id_mensaje_correlativo != 0) buffer->size += sizeof(uint32_t);

	void* stream = malloc(buffer->size);
	int offset = 0;

	memcpy(stream + offset, &(largo_nombre), sizeof(uint32_t));
	offset += sizeof(uint32_t);

	memcpy(stream + offset, mensaje.nombre_pokemon, largo_nombre);
	offset += largo_nombre;

	memcpy(stream + offset, &(mensaje.cant_pos), sizeof(uint32_t));
	offset += sizeof(uint32_t);

	//Serializo un par de coordenadas por cada elemento de la lista de posiciones
	char** pos_list = string_get_string_as_array(mensaje.posiciones);
	for(i=0; i<mensaje.cant_pos; i++){
		char* pos_actual = pos_list[i];
		char** pos_pair = string_split(pos_actual,"|");
		pos_x = atoi(pos_pair[0]);
		pos_y = atoi(pos_pair[1]);

		memcpy(stream + offset, &(pos_x), sizeof(uint32_t));
		offset += sizeof(uint32_t);
		memcpy(stream + offset, &(pos_y), sizeof(uint32_t));
		offset += sizeof(uint32_t);
		free(pos_actual);
		free(pos_pair);
	}
	free(pos_list);
	if(mensaje.id_mensaje_correlativo != 0){
		memcpy(stream + offset, &(mensaje.id_mensaje_correlativo), sizeof(uint32_t));
		offset += sizeof(uint32_t);
	}

	buffer->stream = stream;
	return buffer;
}
void eliminarPaquete(t_paquete* paquete)
{
	free(paquete->buffer->stream);
	free(paquete->buffer);
	free(paquete);
}
