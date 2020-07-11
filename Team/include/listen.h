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

extern t_list *mapped_pokemons;
t_list *ID_caught;
pthread_mutex_t ID_caught_sem;

extern t_log *internalLogTeam;
extern t_log *logTeam;

extern Config *config;

t_list *ID_caught;

extern int ciclos_cpu;
void* get_opcode (int socket);

void process_request_recv (op_code cod_op, int socket);

int send_catch (Trainer *trainer);

int procesar_caught(void *id_corr);

void * send_catch_routine (void * train);

int reintentar_conexion(op_code colaSuscripcion);

void procesar_appeared (void *mensaje);

char* colaParaLogs(op_code cola);

int informarID(uint32_t id, sem_t *trainer_sem);
#endif /* INCLUDE_LISTEN_H_ */
