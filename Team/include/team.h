/*
 * Team.h
 *
 *  Created on: 1 jun. 2020
 *      Author: utnso
 */

#ifndef TEAM_H_
#define TEAM_H_

#include <commons/collections/list.h>
#include <semaphore.h>
#include <pthread.h>
#include <unistd.h>
#include "teamConfig.h"

#define WORLD_POSITION db_world_pos
#define PRINT_TEST 0
#define LIBERAR 0




/* Recursos compartidos */
t_list *BlockedQueue;
sem_t qb_sem1;
sem_t qb_sem2;
sem_t using_cpu;
extern sem_t trainer_count;
extern cola_caught;
extern sem_t qcaught_sem;


sem_t poklist_sem;
sem_t poklist_sem2;
t_list *mapped_pokemons;

/* Errores para identificar estado en la ejecución de los hilos */
typedef enum{
    FINISHED,       //Finalizo su ráfaga de ejecución correctamente
    PENDING,        //Fue desalojado por el planificador y aún tiene instrucciones por ejecutar
} exec_error;


/* Pokemones en el mapa interno del Team*/
typedef struct
{
    char *name;
    u_int32_t cant;
    u_int32_t posx;
    u_int32_t posy;
} mapPokemons;


typedef struct
{
    /* Configuraciones globales */
    Config *config;

    /* Sobre el Team */
    u_int32_t team_size;
    t_list *global_objective;
    t_list *trainers;
    t_list *ReadyQueue;
    sem_t qr_sem1;
    sem_t qr_sem2;

    pthread_t trhandler_id;

} Team;

/*Genera lista con strings de pokemones que conforman el objetivo global*/
t_list* Team_GET_generate (t_list *global_objective);

u_int32_t Trainer_handler_create(Team *this_team);


/*  Inicializa el proceso Team   */
Team* Team_Init(void);

void listen_new_pokemons (Team *this_team);

void* listen_routine (void *pokemons_in_map);

void send_trainer_to_exec (Team* this_team, char *planning_algorithm);

exec_error fifo_exec (Team* this_team);


void liberar_listas(Team *this_team);
void imprimir_lista (t_list *get_list);

#endif /* TEAM_H_ */
