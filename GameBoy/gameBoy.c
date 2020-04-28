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
	char* ipServidorBroker = config_get_string_value(config, "IP_BROKER");
	char* puertoServidorBroker = config_get_string_value(config, "PUERTO_BROKER");
	char* ipServidorTeam = config_get_string_value(config, "IP_TEAM");
	char* puertoServidorTeam = config_get_string_value(config, "PUERTO_TEAM");

	t_new_pokemon pokemon;
	pokemon.nombre_pokemon = "Pikachu";
	pokemon.pos_x = 4;
	pokemon.pos_y = 5;
	pokemon.cantidad = 2;

	if(config == NULL){
		printf("No se puede leer el archivo de configuración de Game Boy\n");
	}else{
		printf("Leí la IP %s del PUERTO %s\n", ipServidorBroker, puertoServidorBroker);
	}

	int gameBoyBroker = crearSocketCliente(ipServidorBroker, puertoServidorBroker);

	enviarNewPokemon(gameBoyBroker, pokemon);

/*	PARA QUE EL SERVIDOR TEAM RECIBA EL MENSAJE HABRÁ QUE HACER UN SWITCH
 	t_appeared_pokemon pokemon1;
	pokemon1.nombre_pokemon = "Nidoran";
	pokemon1.pos_x = 10;
	pokemon1.pos_y = 3;

	int gameBoyTeam = crearSocketCliente(ipServidorTeam, puertoServidorTeam);
	printf("Socket Cliente %d\n", gameBoyTeam);

	enviarAppearedPokemon(gameBoyTeam, pokemon1); */
}
