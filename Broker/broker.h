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
	t_list* nodos;
	t_list* suscriptores;
}t_cola;

typedef struct {
	t_list* susc_enviados;
	t_list* susc_ack;
	t_new_pokemon* mensaje;
} t_nodo_cola_new;

typedef struct {
	t_list* susc_enviados;
	t_list* susc_ack;
	t_appeared_pokemon* mensaje;
} t_nodo_cola_appeared;

typedef struct {
	t_list* susc_enviados;
	t_list* susc_ack;
	t_get_pokemon* mensaje;
} t_nodo_cola_get;

typedef struct {
	t_list* susc_enviados;
	t_list* susc_ack;
	t_localized_pokemon* mensaje;
} t_nodo_cola_localized;

typedef struct {
	t_list* susc_enviados;
	t_list* susc_ack;
	t_catch_pokemon* mensaje;
} t_nodo_cola_catch;

typedef struct {
	t_list* susc_enviados;
	t_list* susc_ack;
	t_caught_pokemon* mensaje;
} t_nodo_cola_caught;

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

uint32_t ID_MENSAJE = 0;

t_configuracion* config_broker;
t_config* config_ruta;

t_log* logBrokerInterno;
t_log* logBroker;

char* pathConfigBroker = "broker.config";
int socketServidorBroker;
int cliente;
void* punteroMemoria;
char* algoritmoMemoria;

t_cola* cola_new;
t_cola* cola_appeared;
t_cola* cola_get;
t_cola* cola_localized;
t_cola* cola_catch;
t_cola* cola_caught;

t_list* particiones;

/* FUNCIONES */

/// INICIALIZACIÓN
int crearConfigBroker();
bool existeArchivoConfig(char* path);

void inicializarColas();
void inicializarMemoria();

/// CONEXIÓN
void atenderCliente(int socket_cliente);

void atenderMensajeNewPokemon(int socket);
void atenderMensajeAppearedPokemon(int socket);
void atenderMensajeCatchPokemon(int socket);
void atenderMensajeCaughtPokemon(int socket);
void atenderMensajeGetPokemon(int socket);
void atenderMensajeLocalizedPokemon(int socket);

void atenderSuscripcionTeam(int socket);
void atenderSuscripcionGameBoy(int socket);
void atenderSuscripcionGameCard(int socket);

/// PROCESAMIENTO
int suscribir(int socket, op_code cola);
void desuscribir(int index, op_code cola);

void encolarNewPokemon(t_new_pokemon* msg);
void encolarAppearedPokemon(t_appeared_pokemon* msg);
void encolarCatchPokemon(t_catch_pokemon* msg);
void encolarCaughtPokemon(t_caught_pokemon* msg);
void encolarGetPokemon(t_get_pokemon* msg);
void encolarLocalizedPokemon(t_localized_pokemon* msg);

/// COMUNICACIÓN
int devolverID(int socket);

#endif /* BROKER_H_ */
