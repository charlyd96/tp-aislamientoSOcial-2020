/*
 * Trainers.h
 *
 *  Created on: 1 jun. 2020
 *      Author: utnso
 */

#ifndef INCLUDE_TRAINERS_H_
#define INCLUDE_TRAINERS_H_

#include "team.h"


sem_t trainer_count;
sem_t trainer_deadlock_count;
extern sem_t using_cpu;
extern t_list *mapped_pokemons;
extern sem_t poklist_sem;
extern sem_t poklist_sem2;

sem_t resolviendo_deadlock;

t_list *deadlock_list;
sem_t deadlock_sem1;
sem_t deadlock_sem2;

extern t_log *internalLogTeam;
extern t_log *logTeam;

extern t_list *ReadyQueue;
extern sem_t qr_sem1;
extern sem_t qr_sem2;


typedef enum {
    OP_EXECUTING_CATCH=0,        //Ejecutando: desplazándose al pokemon a atrapar
    OP_EXECUTING_DEADLOCK=1     //Ejecutando: desplazándose hacia otro entrenador para solucionar un deadlock
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
    char *recibir;
    char *entregar;
    int index_objective;
    uint32_t posx;
    uint32_t posy;
} Deadlock;

extern t_list *trainers;

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
    sem_t trainer_sem;

    /* Posición del entrenador */
    u_int32_t posx;
    u_int32_t posy;

    /* Objetivo actual a ser capturado*/
    mapPokemons actual_objective;

    Deadlock objetivo;

    /*Identificador del entrenador*/
    int index;

    u_int32_t catch_result;
    Config *config;

} Trainer;

/* Error list for debugging */
typedef enum{
    TRAINER_CREATED,
    TRAINER_CREATION_FAILED,
    TRAINER_DELETED,
    TRAINER_DELETION_FAILED,
    TRAINER_OBJECTIVES,
    TRAINER_NO_OBJECTIVES,
    RECOVERY_DEADLOCK_ERROR,
    RECURSIVE_RECOVERY_SUCESS
} trainer_error;

typedef struct
{
	u_int32_t id_corr;
	sem_t trainer_sem;
	mapPokemons actual_objective;
}catch_internal;


void * trainer_routine (void *trainer);

void * trainer_to_catch (void *dato);

void send_trainer_to_ready (t_list *lista, int index, Operation op);

void mover_objetivo_a_lista_auxiliar (mapPokemons *actual_pokemon);

void move_trainer_to_objective (Trainer *train, Operation operacion);

u_int32_t calculate_distance (u_int32_t Tx, u_int32_t Ty, u_int32_t Px, u_int32_t Py );

void remover_objetivo_global_auxiliar(char *name_pokemon);

int send_catch (Trainer *trainer);

bool comparar_listas (t_list *lista1, t_list *lista2); //Algoritmo de detección de deadlock

t_list* duplicar_lista (t_list *lista);

void split_objetivos_capturados (Trainer *trainer, t_list *lista_sobrantes, t_list *lista_faltantes);

void trainer_to_deadlock(Trainer *trainer);

int deadlock_recovery (void); //Algoritmo de recuperación de dadlock 

int intercambiar(Trainer *trainer1, Trainer *trainer2);

bool detectar_deadlock (Trainer* trainer);

void *trainer_exec (void *config);

#endif /* INCLUDE_TRAINERS_H_ */
