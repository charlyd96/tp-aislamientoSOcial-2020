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
#include <protocolo.h>
#include <commons/log.h>

#define PRINT_TEST 0
#define LIBERAR 0




/* Recursos compartidos */
t_list *BlockedQueue;
sem_t qb_sem1;
sem_t qb_sem2;
sem_t using_cpu;
extern sem_t trainer_count;
extern t_list *cola_caught;
extern sem_t qcaught1_sem;
extern sem_t qcaught2_sem;

t_list *global_objective;

sem_t poklist_sem;
sem_t poklist_sem2;
t_list *mapped_pokemons;

extern t_log *internalLogTeam;
extern t_log *logTeam;

/* Errores para identificar estado en la ejecución de los hilos - para RR y SJF */
typedef enum
{
    FINISHED,       //Finalizo su ráfaga de ejecución correctamente
    PENDING,        //Fue desalojado por el planificador y aún tiene instrucciones por ejecutar
} exec_error;



typedef struct
{
	char *broker_IP;
	char *broker_port;
    u_int32_t tiempo_reconexion;
	op_code colaSuscripcion;   
} conexionColas;


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
    char *name;
    uint32_t cant;
} global_pokemons;

typedef struct
{
    /* Configuraciones globales */
    Config *config;

    /* Sobre el Team */
    u_int32_t team_size;

    t_list *trainers;
    t_list *ReadyQueue;
    sem_t qr_sem1;
    sem_t qr_sem2;

    pthread_t trhandler_id;

} Team;

/*Genera lista con strings de pokemones que conforman el objetivo global*/
t_list* Team_GET_generate (void);

u_int32_t Trainer_handler_create(Team *this_team);


/*  Inicializa el proceso Team   */
Team* Team_Init(void);



void* listen_routine_gameboy (void *config);

void send_trainer_to_exec (Team* this_team, char *planning_algorithm);

exec_error fifo_exec (Team* this_team);

void subscribe (Config *config) ;

void listen_new_pokemons (Config *config);

void liberar_listas(Team *this_team);

void imprimir_lista (t_list *get_list);

void enviar_mensajes_get (Config *config, t_list *get_list);

void* listen_routine_colas (void *conexion);
#endif /* TEAM_H_ */
