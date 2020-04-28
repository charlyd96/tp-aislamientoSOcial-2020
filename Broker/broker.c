/*
 * broker.c
 *
 *  Created on: 24 abr. 2020
 *      Author: aislamientoSOcial
 */

#include "protocolo.h"
#include "conexion.h"
#include "serializacion.h"
#include <commons/config.h>

int main(void){
	t_config* config = config_create("broker.config");
	char* ipBroker = config_get_string_value(config, "IP_BROKER");
	char* puertoBroker = config_get_string_value(config, "PUERTO_BROKER");

	printf("Leí la IP %s del PUERTO %s\n", ipBroker, puertoBroker);

	int socketServidorBroker = crearSocketServidor(ipBroker, puertoBroker);

	if(socketServidorBroker == -1){
		printf("No se pudo crear el Servidor Broker");
	}else{
		printf("Socket Servidor %d\n", socketServidorBroker);
	}

	int cliente = aceptarCliente(socketServidorBroker);

	char* recibido = recibirNewPokemon(cliente);
	printf("Mensaje recibido %s", recibido);
}
