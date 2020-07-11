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
    int socket;
    op_code op;
} data_listen;  

extern t_list *mapped_pokemons;
t_list *cola_caught;
sem_t qcaught1_sem;
sem_t qcaught2_sem;

extern t_log *internalLogTeam;
extern t_log *logTeam;

extern Config *config;

extern int ciclos_cpu;
void* get_opcode (int socket);

void process_request_recv (void *data);

int send_catch (Trainer *trainer);

int search_caught(u_int32_t id_corr, sem_t *trainer_sem);

void * send_catch_routine (void * train);

int reintentar_conexion(op_code colaSuscripcion);

char* colaParaLogs(op_code cola);
#endif /* INCLUDE_LISTEN_H_ */
