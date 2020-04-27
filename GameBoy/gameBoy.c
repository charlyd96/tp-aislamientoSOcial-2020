/*
 * gameboy.c
 *
 *  Created on: 24 abr. 2020
 *      Author: aislamientoSOcial
 */

#include <stdio.h>
#include <stdlib.h>
#include "protocolo.h"
#include "conexion.h"
#include "serializacion.h"
#include <commons/config.h>

int main(void){
	t_config* config = config_create("./gameBoy.config");
	char* ipServidor = config_get_string_value(config, "IP_BROKER");
	char* puertoServidor = config_get_string_value(config, "PUERTO_BROKER");

	t_new_pokemon pokemon;

	pokemon.nombre_pokemon = "Pikachu";
	pokemon.pos_x = 4;
	pokemon.pos_y = 5;
	pokemon.cantidad = 2;

	if(config == NULL){
		printf("No se puede leer el archivo de configuración de Broker\n");
	}else{
		printf("Leí la IP %s del PUERTO %s\n", ipServidor, puertoServidor);
	}

	int gameBoy = crearSocketCliente(ipServidor, puertoServidor);
	printf("Socket Cliente %d\n", gameBoy);

	enviarNewPokemon(gameBoy, pokemon);
}
