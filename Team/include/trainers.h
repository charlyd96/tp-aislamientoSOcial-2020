/*
 * Trainers.h
 *
 *  Created on: 1 jun. 2020
 *      Author: utnso
 */

#ifndef INCLUDE_TRAINERS_H_
#define INCLUDE_TRAINERS_H_

#include "team.h"

typedef enum {
    EXECUTING_CATCH=0,        //Ejecutando: desplazándose al pokemon a atrapar
    EXECUTING_DEADLOCK=1     //Ejecutando: desplazándose hacia otro entrenador para solucionar un deadlock
} Operation;

/* Diagrama de estado de los entrenadores */
typedef enum{
    NEW,                    //Recién creado
    READY,                  //En la cola ready
    BLOCKED_DEADLOCK,       //Bloqueado por deadlock
    BLOCKED_NOTHING_TO_DO,  //Bloquedo porque el planificador por distancia no lo seleccionó aún para atrapar un pokemon
    BLOCKED_WAITING_REPLY,  //Bloquedo esperando la respuesta de un CATCH
    EXEC,                   //Listo para ejecutar
    EXIT                    //Objetivos personales cumplidos
} Status;

typedef struct
{
    /*  Inventario del entrenador - Son los pokemones que ya trae por archivo de configuración */
    t_list *bag;

    /* Objetivo personal de cada entrenador */
    t_list *personal_objective;

    /* Relativo al proceso */
    Status actual_status;
    Operation actual_operation;
    pthread_t thread_id;
    sem_t t_sem;

    /* Posición del entrenador */
    u_int32_t posx;
    u_int32_t posy;

    /* Objetivo actual a ser capturado*/
    mapPokemons actual_objective;

} Trainer;


/* Error list for debugging */
typedef enum{
    TRAINER_CREATED,
    TRAINER_CREATION_FAILED,
    TRAINER_DELETED,
    TRAINER_DELETION_FAILED,
    TRAINER_OBJECTIVES,
    TRAINER_NO_OBJECTIVES
} trainer_error;


void * trainer_routine (void *trainer);

void * Trainer_to_plan_ready (void *this_team);

void send_trainer_to_ready (Team *this_team, u_int32_t index);

void move_trainer_to_pokemon (Trainer *train);

u_int32_t calculate_distance (u_int32_t Tx, u_int32_t Ty, u_int32_t Px, u_int32_t Py );


#endif /* INCLUDE_TRAINERS_H_ */
