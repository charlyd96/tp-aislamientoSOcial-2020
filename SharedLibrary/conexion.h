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

op_code recibirOperacion(int socket_cliente);

int enviarMensaje(int nroSocket,op_code operacion,t_buffer* buffer);

//Dado un Socket, envía un mensaje del tipo NEW_POKEMON.
int enviarNewPokemon(int socket_cliente, t_new_pokemon mensaje);
int enviarGetPokemon(int socket_cliente, t_get_pokemon mensaje);
int enviarCatchPokemon(int socket_cliente, t_catch_pokemon mensaje);
int enviarAppearedPokemon(int socket_cliente, t_appeared_pokemon mensaje);
int enviarLocalizedPokemon(int socket_cliente, t_localized_pokemon mensaje);
int enviarCaughtPokemon(int socket_cliente, t_caught_pokemon mensaje);
int enviarSuscripcion(int socket_cliente,t_suscribe mensaje);

//Dado un Socket, recibe un mensaje del tipo NEW_POKEMON.
t_new_pokemon* recibirNewPokemon(int socket_cliente);
t_appeared_pokemon* recibirAppearedPokemon(int socket_cliente);
t_catch_pokemon* recibirCatchPokemon(int socket_cliente);
t_caught_pokemon* recibirCaughtPokemon(int socket_cliente);
t_get_pokemon* recibirGetPokemon(int socket_cliente);
t_localized_pokemon* recibirLocalizedPokemon(int socket_cliente);
t_suscribe* recibirSuscripcion(op_code tipo,int socket_cliente);

int enviarACK(int socket_destino);
int recibirACK(int socket_origen);

char* colaParaLogs(op_code cola);

/* HAY QUE REVISARLO
void recibirCliente(int* socket);
void procesarSolicitud(int cod_op, int cliente); */

#endif /* CONEXION_H_ */
