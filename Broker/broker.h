/*
 * broker.h
 *
 *  Created on: 2 may. 2020
 *      Author: utnso
 */

#ifndef BROKER_H_
#define BROKER_H_

#include "protocolo.h"
#include "conexion.h"
#include "serializacion.h"
#include <commons/config.h>
#include <commons/collections/list.h>
#include <commons/log.h>
#include <pthread.h>

/* STRUCTS */

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

typedef struct {
	t_list* suscriptores_yaEnviados;
	t_list* suscriptores_retornaronACK;
} t_mensaje_suscriptor;

typedef struct {
	pthread_t hilo;
	int socket;
} t_nodo;

typedef struct {
	uint32_t largo_nombre;
	char* nombre_pokemon;
	uint32_t pos_x;
	uint32_t pos_y;
	uint32_t cantidad;
} t_new_pokemon_memoria;

typedef struct {
	uint32_t largo_nombre;
	char* nombre_pokemon;
	uint32_t pos_x;
	uint32_t pos_y;
} t_appeared_pokemon_memoria, t_catch_pokemon_memoria;

typedef struct {
	uint32_t atrapo_pokemon;
} t_caught_pokemon_memoria;

typedef struct {
	uint32_t largo_nombre;
	char* nombre_pokemon;
} t_get_pokemon_memoria;

typedef struct {
	uint32_t largo_nombre;
	char* nombre_pokemon;
	uint32_t cant_pos;
	uint32_t pos_x;
	uint32_t pos_y;
} t_localized_pokemon_memoria;

typedef struct {
	bool libre;
} t_particion_memoria;

typedef enum {FIFO, LRU} t_algoritmo_reemplazo;

typedef enum {FF, BF} t_algoritmo_particion_libre;

/* VARIABLES GLOBALES */

t_configuracion* config_broker;
t_config* config_ruta;

t_log* logBrokerInterno;
t_log* logBroker;

char* pathConfigBroker = "broker.config";
int socketServidorBroker;
int cliente;
void* punteroMemoria;
char* algoritmoMemoria;

t_list* colaNewPokemon;
t_list* colaAppearedPokemon;
t_list* colaCatchPokemon;
t_list* colaCaughtPokemon;
t_list* colaGetPokemon;
t_list* colaLocalizedPokemon;

t_list* particiones;

/* FUNCIONES */

int crearConfigBroker();
bool existeArchivoConfig(char* path);
void atenderCliente(int socket_cliente);

void atenderSuscripcion(t_suscribe* suscriptor, int socket_cliente);

void mostrar();
#endif /* BROKER_H_ */
