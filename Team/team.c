/*
 * team.c
 *
 *  Created on: 27 abr. 2020
 *      Author: aislamientoSOcial
 */

#include "protocolo.h"
#include "conexion.h"
#include "serializacion.h"
#include <commons/config.h>

int main(void){
	t_config* config = config_create("team.config");
	char* ipTeam = config_get_string_value(config, "IP_TEAM");
	char* puertoTeam = config_get_string_value(config, "PUERTO_TEAM");

	printf("Leí la IP %s del PUERTO %s\n", ipTeam, puertoTeam);

	int socketServidorTeam = crearSocketServidor(ipTeam, puertoTeam);

	if(socketServidorTeam == -1){
		printf("No se pudo crear el Servidor Team");
	}else{
		printf("Socket Servidor %d\n", socketServidorTeam);
	}

	int cliente = aceptarCliente(socketServidorTeam);

	char* recibido = recibirAppearedPokemon(cliente);
	printf("Mensaje recibido %s", recibido);
}
