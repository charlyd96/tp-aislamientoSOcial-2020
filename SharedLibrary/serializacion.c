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

void eliminarPaquete(t_paquete* paquete)
{
	free(paquete->buffer->stream);
	free(paquete->buffer);
	free(paquete);
}
