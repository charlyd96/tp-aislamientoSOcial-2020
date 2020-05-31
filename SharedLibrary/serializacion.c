/*
 * serializacion.c
 *
 *  Created on: 24 abr. 2020
 *      Author: aislamientoSOcial
 */

#include "serializacion.h"

void* serializarPaquete(t_paquete* paquete, int *bytes)
{
	int largo = sizeof(op_code) + sizeof(uint32_t) + paquete->buffer->size;
	int offset = 0;
	void * serializado = malloc(largo);

	memcpy(serializado + offset, &(paquete->codigo_operacion), sizeof(paquete->codigo_operacion));
	offset += sizeof(paquete->codigo_operacion);
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
	if(mensaje.id_mensaje != 0) buffer->size += sizeof(uint32_t);

	void* stream = malloc(buffer->size);
	int offset = 0;

	memcpy(stream + offset, &(mensaje.atrapo_pokemon), sizeof(uint32_t));
	offset += sizeof(uint32_t);

	memcpy(stream + offset, &(mensaje.id_mensaje_correlativo), sizeof(uint32_t));
	offset += sizeof(uint32_t);

	if(mensaje.id_mensaje != 0){
		memcpy(stream + offset, &(mensaje.id_mensaje), sizeof(uint32_t));
		offset += sizeof(uint32_t);
	}

	buffer->stream = stream;
	return buffer;
}
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

void eliminarPaquete(t_paquete* paquete)
{
	free(paquete->buffer->stream);
	free(paquete->buffer);
	free(paquete);
}
