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


char* pathConfigBroker = "broker.config";
uint32_t ID_MENSAJES = 0;

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
	char* nombre_pokemon;
	uint32_t pos_x;
	uint32_t pos_y;
	uint32_t cantidad;
} t_new_pokemon_memoria;

typedef struct {
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

typedef struct{
	t_list* nodos; //lista con t_nodos_x
	t_list* suscriptores;
}t_cola;

//Nodos de cada cola
typedef struct{
	t_list* susc_enviados;
	t_list* susc_ack;
	t_new_pokemon* mensaje;
} t_nodo_cola_new;
typedef struct{
	t_list* susc_enviados;
	t_list* susc_ack;
	t_appeared_pokemon* mensaje;
} t_nodo_cola_appeared;
typedef struct{
	t_list* susc_enviados;
	t_list* susc_ack;
	t_get_pokemon* mensaje;
} t_nodo_cola_get;
typedef struct{
	t_list* susc_enviados;
	t_list* susc_ack;
	t_localized_pokemon* mensaje;
} t_nodo_cola_localized;
typedef struct{
	t_list* susc_enviados;
	t_list* susc_ack;
	t_catch_pokemon* mensaje;
} t_nodo_cola_catch;
typedef struct{
	t_list* susc_enviados;
	t_list* susc_ack;
	t_caught_pokemon* mensaje;
} t_nodo_cola_caught;

t_list* particiones;

/* FUNCIONES */

int crearConfigBroker();
bool existeArchivoConfig(char* path);
void atenderCliente(int socket_cliente);

void encolarNewPokemon(t_new_pokemon* msg);
void encolarAppearedPokemon(t_appeared_pokemon* msg);
void encolarGetPokemon(t_get_pokemon* msg);
void encolarLocalizedPokemon(t_localized_pokemon* msg);
void encolarCatchPokemon(t_catch_pokemon* msg);
void encolarCaughtPokemon(t_caught_pokemon* msg);

void handleNewPokemonMsg(int socket);
void handleAppearedPokemonMsg(int socket);
void handleGetPokemonMsg(int socket);
void handleLocalizedPokemonMsg(int socket);
void handleCatchPokemonMsg(int socket);
void handleCaughtPokemonMsg(int socket);

void handleSuscribeTeamMsg(int socket);
void handleSuscribeGameboyMsg(int socket);
void handleSuscribeGameCardMsg(int socket);

t_cola* cola_new;
t_cola* cola_appeared;
t_cola* cola_get;
t_cola* cola_localized;
t_cola* cola_catch;
t_cola* cola_caught;

#endif /* BROKER_H_ */
