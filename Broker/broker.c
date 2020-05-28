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
#include "broker.h"

int crearConfigBroker(){

	t_log* logBrokerInterno = log_create("broker.txt", "LOG", 0, LOG_LEVEL_INFO);

    log_info(logBrokerInterno, "Se hizo el Log.\n");

	//Valido que exista la ruta de configuración

	if (!existeArchivoConfig(pathConfigBroker)) {
		log_error(logBrokerInterno, "Verificar path del archivo \n");
	    return -1;
	}

	config_broker = malloc(sizeof(t_configuracion));
	config_ruta = config_create(pathConfigBroker);

	if (config_ruta != NULL){
	    config_broker->tam_memoria = config_get_int_value(config_ruta, "TAMANO_MEMORIA");
		config_broker->tam_minimo_particion = config_get_int_value(config_ruta, "TAMANO_MINIMO_PARTICION");
	    config_broker->algoritmo_memoria = config_get_string_value(config_ruta, "ALGORITMO_MEMORIA");
	    config_broker->algoritmo_reemplazo = config_get_string_value(config_ruta, "ALGORITMO_REEMPLAZO");
	    config_broker->algoritmo_particion_libre = config_get_string_value(config_ruta, "ALGORITMO_PARTICION_LIBRE");
	    config_broker->ip_broker = config_get_string_value(config_ruta, "IP_BROKER");
	    config_broker->puerto_broker = config_get_string_value(config_ruta, "PUERTO_BROKER");
	    config_broker->frecuencia_compatacion = config_get_int_value(config_ruta, "FRECUENCIA_COMPACTACION");
	}

	if(config_broker == NULL){
	    log_error(logBrokerInterno, "ERROR: No se pudo levantar el archivo de configuración.\n");
	    return -1;
	}
	return 0;
}

bool existeArchivoConfig(char* path){
	FILE * file = fopen(path, "r");
	if(file!=NULL){
		fclose(file);
		return true;
	} else{
		return false;
	}
}

int main(void){
	crearConfigBroker();

	int socketServidorBroker = crearSocketServidor(config_broker->ip_broker, config_broker->puerto_broker);

	if(socketServidorBroker == -1){
		printf("No se pudo crear el Servidor Broker");
	}else{
		printf("Socket Servidor %d\n", socketServidorBroker);
	}

	int cliente = aceptarCliente(socketServidorBroker);

	char* recibido = recibirNewPokemon(cliente);
	printf("Mensaje recibido %s", recibido);
}
