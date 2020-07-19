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
#define LIBERAR 1


/* Recursos compartidos */

sem_t using_cpu;
extern sem_t trainer_count;
extern sem_t trainer_deadlock_count;

extern t_list *cola_caught;
extern sem_t qcaught1_sem;
extern sem_t qcaught2_sem;

uint32_t ID_proceso;

extern t_list *ID_localized;

bool win;

extern t_list *deadlock_list;
extern sem_t deadlock_sem1;
extern sem_t deadlock_sem2;

t_list *global_objective;
//sem_t global_sem1;
//sem_t global_sem2;
pthread_mutex_t global_sem;

t_list *aux_global_objective;
//sem_t auxglobal_sem1;
//sem_t auxglobal_sem2;
pthread_mutex_t auxglobal_sem;

t_list *new_global_objective;
pthread_mutex_t new_global_sem;

t_list *aux_new_global_objective;
pthread_mutex_t aux_new_global_sem;

t_list *mapped_pokemons;
sem_t poklist_sem;
sem_t poklist_sem2;

t_list *mapped_pokemons_aux;
sem_t poklistAux_sem1;
sem_t poklistAux_sem2;



extern t_log *internalLogTeam;
extern t_log *logTeam;

t_list *ReadyQueue;
sem_t qr_sem1;
sem_t qr_sem2;

sem_t terminar_ejecucion;

/* Errores para identificar estado en la ejecución de los hilos - para RR y SJF - específico de cada ráfaga*/
typedef enum
{
    FINISHED,       //Finalizo su ráfaga de ejecución correctamente
    PENDING,        //Fue desalojado por el planificador y aún tiene instrucciones por ejecutar de la ŕafaga
    EXECUTING       //Se encuentra ejecutando su ráfaga
} exec_error;



extern Config *config;

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

// Lista de entrenadores
t_list *trainers;


/*Genera lista con strings de pokemones que conforman el objetivo global*/
t_list* Team_GET_generate (void);

int Trainer_handler_create(void);


/*  Inicializa el proceso Team   */
void Team_Init(void);

void* listen_routine_gameboy (void);

void send_trainer_to_exec (void);

void subscribe (void) ;

void listen_new_pokemons (void);

void imprimir_lista (t_list *get_list);

void enviar_mensajes_get (t_list *get_list);

void* listen_routine_colas (void *conexion);

void informarIDlocalized(uint32_t id);

void SJFSD_exec (void);

void SJFCD_exec (void);

double  actualizar_estimacion (); //Ver cómo agregar el parámetro Trainer *

void ordenar_lista_ready(void);

void inicializar_listas(void); 

void inicializar_semaforos(void);  
#endif /* TEAM_H_ */
