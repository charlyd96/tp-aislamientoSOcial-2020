/*
 * conexion.h
 *
 *  Created on: 24 abr. 2020
 *      Author: aislamientoSOcial
 */

#ifndef CONEXION_H_
#define CONEXION_H_

#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <netdb.h>
#include <string.h>
#include <sys/socket.h>
#include <sys/types.h>
#include <arpa/inet.h>
#include <signal.h>
#include <pthread.h>
#include <commons/log.h>
#include "protocolo.h"
#include "serializacion.h"

//Dado un ip y un puerto, crea un Socket Cliente para la comunicación con un Servidor.
int crearSocketCliente(char* ip, char* puerto);

//Dado un ip y un puert, crea un Socket Servidor para la comunicación con un Cliente.
int crearSocketServidor(char* ip, char* puerto);

//Dado un servidor, acepta al Cliente que se este queriendo conectar a el.
int aceptarCliente(int socket_servidor);

int recibirOperacion(int socket_cliente);


//Dado un Socket, envía un mensaje del tipo NEW_POKEMON.
int enviarNewPokemon(int socket_cliente, t_new_pokemon mensaje);

//Dado un Socket, recibe un mensaje del tipo NEW_POKEMON.
t_new_pokemon* recibirNewPokemon(int socket_cliente);

//Dado un Socket, envía un mensaje del tipo APPEARED_POKEMON.
int enviarAppearedPokemon(int socket_cliente, t_appeared_pokemon mensaje);

//Dado un Socket, recibe un mensaje del tipo APPEARED_POKEMON.
t_appeared_pokemon* recibirAppearedPokemon(int socket_cliente);

/* HAY QUE REVISARLO
void recibirCliente(int* socket);
void procesarSolicitud(int cod_op, int cliente); */

#endif /* CONEXION_H_ */
