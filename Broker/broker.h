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
#include <semaphore.h>
#include <time.h>
#include <sys/time.h>
#include <math.h>

/* STRUCTS */

typedef enum {PD, BUDDY} t_tipo_particionado;
typedef enum {FIFO, LRU} t_algoritmo_reemplazo;

typedef enum {FF, BF} t_algoritmo_particion_libre;

typedef struct {
	int tam_memoria;
	int tam_minimo_particion;
	t_tipo_particionado algoritmo_memoria;
	t_algoritmo_reemplazo algoritmo_reemplazo;
	t_algoritmo_particion_libre algoritmo_particion_libre;
	char* ip_broker;
	char* puerto_broker;
	int frecuencia_compatacion;
	char* log_file;
} t_configuracion;

typedef struct {
	uint32_t socket_suscriptor;
	uint32_t id_suscriptor;
} t_suscriptor;

typedef struct {
	t_list* nodos;
	t_list* suscriptores;
} t_cola;

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
	op_code tipo_mensaje;
	uint32_t id;
	uint32_t base;
	uint32_t tamanio;
	uint32_t buddy_i; //para Buddy System
	struct timeval time_creacion;
	struct timeval time_ultima_ref;
} t_particion;

/* VARIABLES GLOBALES */

uint32_t ID_MENSAJE = 0;

t_configuracion* config_broker;
t_config* config_ruta;

t_log* logBrokerInterno;
t_log* logBroker;

char* pathConfigBroker = "/home/utnso/workspace/tp-2020-1c-aislamientoSOcial/Broker/broker.config";
int socketServidorBroker;
int cliente;

void* cache;
char* algoritmo_mem;
char* algoritmo_reemplazo;
char* algoritmo_libre;

t_cola* cola_new;
t_cola* cola_appeared;
t_cola* cola_get;
t_cola* cola_localized;
t_cola* cola_catch;
t_cola* cola_caught;

t_nodo_cola_new* nodo_new;
t_nodo_cola_appeared* nodo_appeared;
t_nodo_cola_get* nodo_get;
t_nodo_cola_localized* nodo_localized;
t_nodo_cola_catch* nodo_catch;
t_nodo_cola_caught* nodo_caught;

t_list* particiones;
uint32_t buddy_U; //U es el exponente máximo de partición (la memoria completa)

pthread_mutex_t sem_cola_new;
pthread_mutex_t sem_cola_appeared;
pthread_mutex_t sem_cola_catch;
pthread_mutex_t sem_cola_caught;
pthread_mutex_t sem_cola_get;
pthread_mutex_t sem_cola_localized;

sem_t mensajes_new;
sem_t mensajes_appeared;
sem_t mensajes_catch;
sem_t mensajes_caught;
sem_t mensajes_get;
sem_t mensajes_localized;

sem_t mx_particiones;

sem_t identificador;

/* FUNCIONES */

/// INICIALIZACIÓN
int crearConfigBroker();
bool existeArchivoConfig(char* path);

void inicializarColas();
void inicializarMemoria();
void inicializatSemaforos(); 

/// CONEXIÓN
void atenderCliente(int socket_cliente);

void atenderMensajeNewPokemon(int socket);
void atenderMensajeAppearedPokemon(int socket);
void atenderMensajeCatchPokemon(int socket);
void atenderMensajeCaughtPokemon(int socket);
void atenderMensajeGetPokemon(int socket);
void atenderMensajeLocalizedPokemon(int socket);

void atenderSuscripcionTeam(int socket);
void atenderSuscripcionGameCard(int socket);
void atenderSuscripcionGameBoy(int socket);

/// PROCESAMIENTO
int suscribir(t_suscriptor* suscriptor, op_code cola);
void desuscribir(int index, op_code cola);

void encolarNewPokemon(t_new_pokemon* msg);
void encolarAppearedPokemon(t_appeared_pokemon* msg);
void encolarCatchPokemon(t_catch_pokemon* msg);
void encolarCaughtPokemon(t_caught_pokemon* msg);
void encolarGetPokemon(t_get_pokemon* msg);
void encolarLocalizedPokemon(t_localized_pokemon* msg);

/// MEMORIA
int buscarParticionLibre(uint32_t largo_stream);
void buscarParticionYAlocar(int largo_stream, void* stream, op_code tipo_msg, uint32_t id);
void eliminarParticion();

void algoritmoFIFO();
void algoritmoLRU();

void compactarParticiones();
void liberarParticion(int indice);

void cachearNewPokemon(t_new_pokemon* msg);
void cachearAppearedPokemon(t_appeared_pokemon* msg);
void cachearCatchPokemon(t_catch_pokemon* msg);
void cachearCaughtPokemon(t_caught_pokemon* msg);
void cachearGetPokemon(t_get_pokemon* msg);
void cachearLocalizedPokemon(t_localized_pokemon* msg);

int victimaSegunFIFO();
int victimaSegunLRU();
// BUDDY
void partirBuddy(int indice);
int obtenerHuecoBuddy(int i);
int buscarHuecoBuddy(int i);
void eliminarParticionBuddy();

t_new_pokemon* descachearNewPokemon(void* stream, uint32_t id);
t_appeared_pokemon descachearAppearedPokemon(void* stream, uint32_t id);
t_catch_pokemon descachearCatchPokemon(void* stream, uint32_t id);
t_caught_pokemon descachearCaughtPokemon(void* stream, uint32_t id);
t_get_pokemon* descachearGetPokemon(void* stream, uint32_t id);
t_localized_pokemon descachearLocalizedPokemon(void* stream, uint32_t id);

char* fecha_y_hora_actual();
void dump_cache();
void controlador_de_seniales(int signal);

/// COMUNICACIÓN
void tipoYIDProceso(int socket);
int devolverID(int socket,uint32_t*id);

void enviarNewPokemonCacheados(int socket, op_code tipo_mensaje);
void enviarAppearedPokemonCacheados(int socket, op_code tipo_mensaje);
void enviarCatchPokemonCacheados(int socket, op_code tipo_mensaje);
void enviarCaughtPokemonCacheados(int socket, op_code tipo_mensaje);
void enviarGetPokemonCacheados(int socket, op_code tipo_mensaje);
void enviarLocalizedPokemonCacheados(int socket, op_code tipo_mensaje);

void confirmacionDeRecepcionTeam(int socket, t_suscribe* suscribe_team);
void confirmacionDeRecepcionGameCard(int socket, t_suscribe* suscribe_gamecard);

#endif /* BROKER_H_ */
