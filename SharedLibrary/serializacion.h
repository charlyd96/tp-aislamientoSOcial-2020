/*
 * serializacion.h
 *
 *  Created on: 24 abr. 2020
 *      Author: utnso
 */

#ifndef SERIALIZACION_H_
#define SERIALIZACION_H_

#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <netdb.h>
#include <string.h>
#include <sys/socket.h>
#include <signal.h>
#include "protocolo.h"

void* serializarPaquete(t_paquete* paquete, int *bytes);
t_buffer* serializarNewPokemon(t_new_pokemon pokemon);
void eliminarPaquete(t_paquete* paquete);

#endif /* SERIALIZACION_H_ */
