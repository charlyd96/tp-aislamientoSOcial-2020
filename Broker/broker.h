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

typedef enum {PARTICIONES, BS} t_tipo_particionado;
typedef enum {FIFO, LRU} t_algoritmo_reemplazo;

typedef enum {FF, BF} t_algoritmo_particion_libre;

typedef struct {
	uint32_t tam_memoria;
	uint32_t tam_minimo_particion;
	t_tipo_particionado algoritmo_memoria;
	t_algoritmo_reemplazo algoritmo_reemplazo;
	t_algoritmo_particion_libre algoritmo_particion_libre;
	char* ip_broker;
	char* puerto_broker;
	int frecuencia_compatacion;
	char* log_file;
} t_configuracion;

typedef struct {
	int socket_suscriptor;
	uint32_t id_suscriptor;
} t_suscriptor;

typedef struct {
	t_list* nodos;
	t_list* suscriptores;
} t_cola;

typedef struct {
	t_list* susc_ack;
	t_new_pokemon* mensaje;
} t_nodo_cola_new;
	// t_list* susc_enviados;
	// t_list* susc_no_ack;

typedef struct {
	t_list* susc_ack;
	t_appeared_pokemon* mensaje;
} t_nodo_cola_appeared;
	// t_list* susc_enviados;
	// t_list* susc_no_ack;

typedef struct {
	t_list* susc_ack;
	t_get_pokemon* mensaje;
} t_nodo_cola_get;
	// t_list* susc_enviados;
	// t_list* susc_no_ack;

typedef struct {
	t_list* susc_ack;
	t_localized_pokemon* mensaje;
} t_nodo_cola_localized;
	// t_list* susc_enviados;
	// t_list* susc_no_ack;

typedef struct {
	t_list* susc_ack;
	t_catch_pokemon* mensaje;
} t_nodo_cola_catch;
	// t_list* susc_enviados;
	// t_list* susc_no_ack;

typedef struct {
	t_list* susc_ack;
	t_caught_pokemon* mensaje;
} t_nodo_cola_caught;
	// t_list* susc_enviados;
	// t_list* susc_no_ack;

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
	t_list* susc_enviados;
	uint32_t id;
	uint32_t base;
	uint32_t tamanio;
	uint32_t buddy_i; //para Buddy System
	struct timeval time_creacion;
	struct timeval time_ultima_ref;
	sem_t sem_particion;
} t_particion;

typedef struct {
	int socket;
	t_new_pokemon* mensaje;
	uint32_t id_mensaje;
	uint32_t id_suscriptor;
	t_nodo_cola_new* nodo;
	sem_t *sem_new;
	sem_t *mutex;
	int *cant_suscriptores;
} t_new_aux;

typedef struct {
	int socket;
	t_appeared_pokemon* mensaje;
	uint32_t id_mensaje;
	uint32_t id_suscriptor;
	t_nodo_cola_appeared* nodo;
	sem_t *sem_appeared;
	sem_t *mutex;
	int *cant_suscriptores;
} t_appeared_aux;

typedef struct {
	int socket;
	t_catch_pokemon* mensaje;
	uint32_t id_mensaje;
	uint32_t id_suscriptor;
	t_nodo_cola_catch* nodo;
	sem_t *sem_catch;
	sem_t *mutex;
	int *cant_suscriptores;
} t_catch_aux;

typedef struct {
	int socket;
	t_caught_pokemon* mensaje;
	uint32_t id_mensaje;
	uint32_t id_suscriptor;
	t_nodo_cola_caught* nodo;
	sem_t *sem_caught;
	sem_t *mutex;
	int *cant_suscriptores;
} t_caught_aux;

typedef struct {
	int socket;
	t_get_pokemon* mensaje;
	uint32_t id_mensaje;
	uint32_t id_suscriptor;
	t_nodo_cola_get* nodo;
	sem_t *sem_get;
	sem_t *mutex;
	int *cant_suscriptores;
} t_get_aux;

typedef struct {
	int socket;
	t_localized_pokemon* mensaje;
	uint32_t id_mensaje;
	uint32_t id_suscriptor;
	t_nodo_cola_localized* nodo;
	sem_t *sem_localized;
	sem_t *mutex;
	int *cant_suscriptores;
} t_localized_aux;

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

pthread_mutex_t sem_nodo_new;
pthread_mutex_t sem_nodo_appeared;
pthread_mutex_t sem_nodo_catch;
pthread_mutex_t sem_nodo_caught;
pthread_mutex_t sem_nodo_get;
pthread_mutex_t sem_nodo_localized;

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
void desuscribir(int index, op_code cola, uint32_t id_proceso);
void agregarSuscriptor(uint32_t id_mensaje, uint32_t id_suscriptor);

t_nodo_cola_new* encolarNewPokemon(t_new_pokemon* msg);
t_nodo_cola_appeared* encolarAppearedPokemon(t_appeared_pokemon* msg);
t_nodo_cola_catch* encolarCatchPokemon(t_catch_pokemon* msg);
t_nodo_cola_caught* encolarCaughtPokemon(t_caught_pokemon* msg);
t_nodo_cola_get* encolarGetPokemon(t_get_pokemon* msg);
t_nodo_cola_localized* encolarLocalizedPokemon(t_localized_pokemon* msg);

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

void partirBuddy(int indice);
int obtenerHuecoBuddy(uint32_t i);
int buscarHuecoBuddy(uint32_t i);
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

void enviarNewASuscriptor(t_new_aux* aux);
void enviarAppearedASuscriptor(t_appeared_aux* aux);
void enviarCatchASuscriptor(t_catch_aux* aux);
void enviarCaughtASuscriptor(t_caught_aux* aux);
void enviarGetASuscriptor(t_get_aux* aux);
void enviarLocalizedASuscriptor(t_localized_aux* aux);

void enviarNewPokemonCacheados(int socket, t_suscribe* suscriptor);
void enviarAppearedPokemonCacheados(int socket, t_suscribe* suscriptor);
void enviarCatchPokemonCacheados(int socket, t_suscribe* suscriptor);
void enviarCaughtPokemonCacheados(int socket, t_suscribe* suscriptor);
void enviarGetPokemonCacheados(int socket, t_suscribe* suscriptor);
void enviarLocalizedPokemonCacheados(int socket, t_suscribe* suscriptor);

void confirmacionDeRecepcionTeam(int socket, t_suscribe* suscribe_team, uint32_t id_mensaje);
void confirmacionDeRecepcionGameCard(int socket, t_suscribe* suscribe_gamecard, uint32_t id_mensaje);
void confirmacionDeRecepcionGameBoy(int ack, t_suscribe* suscribe_gameboy, uint32_t id_mensaje);

void enviarAppearedASuscriptor(t_appeared_aux* aux);

#endif /* BROKER_H_ */
