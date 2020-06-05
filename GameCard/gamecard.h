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
#include "protocolo.h"
#include "conexion.h"
#include "serializacion.h"
#include <commons/bitarray.h>
#include <commons/config.h>
#include <pthread.h>

char* pathGamecardConfig = "gameCard.config";

typedef struct{
	int tiempo_reintento_conexion;
	int tiempo_reintento_operacion;
	char* punto_montaje_tall_grass;
	char* ip_gamecard;
	char* puerto_gamecard;
} t_configuracion;

t_configuracion* config_gamecard;
t_config* config_ruta;

#endif /* GAMECARD_H_ */
