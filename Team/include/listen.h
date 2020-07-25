/*
 * listen.h
 *
 *  Created on: 6 jun. 2020
 *      Author: utnso
 */

#ifndef INCLUDE_LISTEN_H_
#define INCLUDE_LISTEN_H_

#include <conexion.h>
#include <protocolo.h>
#include <serializacion.h>
#include <pthread.h>
#include <trainers.h>

#define DEFAULT_CATCH       1 

typedef struct 
{
    uint32_t id;
    bool *resultado;
    sem_t *trainer_sem;
} internal_caught;  


typedef enum 
{
    GUARDAR,
    GUARDAR_AUX,
    DESCARTAR
} nuevo_pokemon;  

extern t_list *mapped_pokemons;

t_list *ID_caught;
pthread_mutex_t ID_caught_sem;

t_list *ID_localized;
pthread_mutex_t ID_localized_sem;

pthread_mutex_t aux_global_sem;
pthread_mutex_t global_sem;

t_list *especies;
pthread_mutex_t especies_sem;

extern uint32_t ID_proceso;

extern t_log *internalLogTeam;
extern t_log *logTeam;

extern Config *config;

sem_t terminar_appeared;

sem_t terminar_caught;

sem_t terminar_localized;

int socketGameboy;

int socketAppeared;

int socketLocalized;

int socketCaught;

int socketGameboyCliente;

extern bool win;

void get_opcode (int socket);

void process_request_recv (op_code cod_op, int socket);

int send_catch (Trainer *trainer);

int reintentar_conexion(op_code colaSuscripcion);

void procesar_appeared(t_appeared_pokemon *mensaje);

void procesar_localized(t_localized_pokemon *mensaje);

void procesar_caught(t_caught_pokemon *mensaje);

char* colaParaLogs(op_code cola);

nuevo_pokemon tratar_nuevo_pokemon (char *nombre_pokemon);

int informarIDcaught(uint32_t id, sem_t *trainer_sem);

void informarIDlocalized(uint32_t id);

void liberar_appeared (t_appeared_pokemon *mensaje);

void agregar_nueva_especie (char *nueva_especie);

bool especie_necesaria (char *nueva_especie);

bool mover_elemento_destino_origen(t_list *destino, t_list *origen, pthread_mutex_t *sem_destino, pthread_mutex_t *sem_origen, char *elemento);

#endif /* INCLUDE_LISTEN_H_ */
