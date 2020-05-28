/*
 * broker.h
 *
 *  Created on: 2 may. 2020
 *      Author: utnso
 */

#ifndef BROKER_H_
#define BROKER_H_

#include <pthread.h>

char* pathConfigBroker = "broker.config";

typedef struct {
	int tam_memoria;
	int tam_minimo_particion;
	char* algoritmo_memoria;
	char* algoritmo_reemplazo;
	char* algoritmo_particion_libre;
	char* ip_broker;
	char* puerto_broker;
	int frecuencia_compatacion;
} t_configuracion;

t_configuracion* config_broker;
t_config* config_ruta;

int crearConfigBroker();
bool existeArchivoConfig(char* path);

#endif /* BROKER_H_ */
