/*
 * listen.h
 *
 *  Created on: 6 jun. 2020
 *      Author: utnso
 */

#ifndef INCLUDE_LISTEN_H_
#define INCLUDE_LISTEN_H_

#include <conexion.h>
#include <team.h>
#include <protocolo.h>
#include <serializacion.h>
#include <pthread.h>
#include <trainers.h>

#define DEFAULT_CATCH       1 

extern t_list *mapped_pokemons;
t_list *cola_caught;
sem_t qcaught1_sem;
sem_t qcaught2_sem;



void* get_opcode (int *socket);

void process_request_recv (op_code cod_op, int socket_cliente);

int send_catch (Trainer *trainer);

int search_caught(u_int32_t id_corr, sem_t *trainer_sem);




void * send_catch_routine (void * train);
#endif /* INCLUDE_LISTEN_H_ */
