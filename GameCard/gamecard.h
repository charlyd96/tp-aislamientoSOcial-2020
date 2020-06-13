/*
 * gamecard.h
 *
 *  Created on: 5 jun. 2020
 *      Author: utnso
 */

#ifndef GAMECARD_H_
#define GAMECARD_H_

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <sys/stat.h>
#include "protocolo.h"
#include "conexion.h"
#include "serializacion.h"
#include <commons/bitarray.h>
#include <commons/config.h>
#include <commons/string.h>
#include <commons/txt.h>
#include <pthread.h>

char* pathGamecardConfig = "gameCard.config";

typedef struct{
	char* directory;
	int size;
	int* blocks;
	char* open;
}t_metadata;

typedef struct{
	int tiempo_reintento_conexion;
	int tiempo_reintento_operacion;
	char* punto_montaje_tall_grass;
	char* ip_gamecard;
	char* puerto_gamecard;
} t_configuracion;

t_configuracion* config_gamecard;
t_config* config_ruta;
t_log* logGamecard;
t_log* logInterno;

t_metadata* data_pokemon;
t_config* metadata;

int crear_config_gamecard();
int crear_metadata_pokemon(char* path);

void iniciar_directorio_basico();
void crear_directorio_pokemon(char* nombre_pokemon);
bool existe_archivo(char* path);

void atender_cliente(int* socket_cliente);
void atender_newPokemon(int socket);
void atender_getPokemon(int socket);
void atender_catchPokemon(int socket);

#endif /* GAMECARD_H_ */
