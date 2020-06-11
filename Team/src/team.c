/*
 * team.c
 *
 *  Created on: 27 abr. 2020
 *      Author: aislamientoSOcial
 */

#include "../include/team.h"

#include <string.h>
#include <readline/readline.h>
#include <readline/history.h>

#include <trainers.h>





/* Initializing all Team components */
Team * Team_Init(void)
{

    Team *this_team =malloc (sizeof(Team)) ;

    this_team->config = malloc (sizeof (Config));
    this_team->config->team_config = get_config();

    sem_init (&trainer_count, 0, 0);					//*********Mejorar la ubicación de esta instrucción***************//
    sem_init (&using_cpu, 0,1);
    Team_load_global_config(this_team->config);
    Team_load_trainers_config(this_team);



    this_team->ReadyQueue= list_create  ();                 //*********Mejorar la ubicación de esta instrucción***************//
    mapped_pokemons = list_create();             //*********Mejorar la ubicación de esta instrucción***************//
    cola_caught = list_create();
    BlockedQueue = list_create();
    sem_init (&qb_sem1, 0, 0);
    sem_init (&qb_sem2, 0, 1);

    sem_init (&poklist_sem, 0, 0);           //*********Mejorar la ubicación de esta instrucción***************//
    sem_init (&poklist_sem2, 0, 1);          //*********Mejorar la ubicación de esta instrucción***************//
    sem_init (&((this_team)->qr_sem1), 0, 0);               //*********Mejorar la ubicación de esta instrucción***************//
    sem_init (&((this_team)->qr_sem2), 0, 1);
    sem_init (&qcaught1_sem,0,0);
    sem_init (&qcaught1_sem,0,1);

    return (this_team);
}

// ============================================================================================================
//      ***** Función que crea una lista con los pokemones que van a ser enviados como mensaje GET *****
//              *****Toma la lista de objetivos globales y elimina los pokemones repetidos*****
// ============================================================================================================

t_list*  Team_GET_generate (t_list *global_objective)
{
    t_list* list_get = list_create();

    void get_pokemons (void *element)
    {
        bool compare_pokemons (void *string)
        {
        if (strcmp((char*)string,(char*)element)  == 0)
        return (true);
        else return (false);
        }

    if (list_any_satisfy (list_get,compare_pokemons) == false)
    list_add (list_get, element);
    }

    list_iterate (global_objective, get_pokemons);
    return (list_get);
}



// ============================================================================================================
//                               ***** Función para imprimir listas (sólo para testear) *****
// ============================================================================================================

void imprimir_lista (t_list *lista)
{

    void imprimir (void *element)
    {
        puts ((char*)element);
    }

    list_iterate (lista, imprimir);
}

/* Consumidor de cola Ready */
exec_error fifo_exec (Team* this_team)
{
       Team *team= this_team;
       sem_wait (&using_cpu);
       sem_wait ( &(this_team->qr_sem1) );
       sem_wait ( &(this_team->qr_sem2) );

       Trainer* trainer= list_remove (team->ReadyQueue, 0);
       trainer->actual_status= EXEC;
       sem_post ( &(this_team->qr_sem2) );
       sem_post ( &(trainer->trainer_sem) );


}


// ============================================================================================================
//    ***** Función que  crea un hilo por cada uno de los entrenadores existente en la lista del Team*****
//                  ***** Recibe una estructura Team y devuelve un código de error *****
// ============================================================================================================

u_int32_t Trainer_handler_create (Team *this_team)
{
    u_int32_t error;
    void  create_thread (void *train)
    {
        Trainer* trainer= train;
        error = pthread_create( &(trainer->thread_id), NULL, trainer_routine, trainer);
        pthread_detach (trainer->thread_id);

    }

    list_iterate (this_team->trainers, create_thread);
    if (error != 0)
    {
        exit (TRAINER_CREATION_FAILED);
    }

    pthread_create ( &(this_team->trhandler_id), NULL, Trainer_to_plan_ready, this_team );
    pthread_detach (this_team->trhandler_id);
    return (error);

}



// ============================================================================================================
//    ***** Función que está a la espera de la llegada de nuevos pokemons y los guarda en una lista *****
//          ***** Recibe una estructura de tipo Team. Añade nodos con data ingresda por consola ****
// ============================================================================================================



void listen_new_pokemons (Config *config)
{
    pthread_t thread; //OJO. Esta variable se está perdiendo
    pthread_create (&thread, NULL, listen_routine_gameboy , config);
    pthread_detach (thread);
}


void subscribe (Config *configuracion)
{

	conexionColas *conexionAppeared = malloc (sizeof (conexionColas));
	conexionAppeared->broker_IP=configuracion->broker_IP;

	conexionAppeared->broker_port=configuracion->broker_port;
	conexionAppeared->colaSuscripcion=APPEARED_POKEMON;
	pthread_t thread1; //OJO. Esta variable se está perdiendo
	pthread_create (&thread1, NULL, listen_routine_colas , conexionAppeared);
	pthread_detach (thread);

	conexionColas *conexionLocalized = malloc (sizeof (conexionColas));
	conexionLocalized->broker_IP=configuracion->broker_IP;
	conexionLocalized->broker_port=configuracion->broker_port;
	conexionLocalized->colaSuscripcion=LOCALIZED_POKEMON;
	pthread_t thread2; //OJO. Esta variable se está perdiendo
	pthread_create (&thread2, NULL, listen_routine_colas , conexionLocalized);
	pthread_detach (thread);

	conexionColas *conexionCaught = malloc (sizeof (conexionColas));
	conexionCaught->broker_IP=configuracion->broker_IP;
	conexionCaught->broker_port=configuracion->broker_port;
	conexionCaught->colaSuscripcion=CAUGHT_POKEMON;
	pthread_t thread3; //OJO. Esta variable se está perdiendo
	pthread_create (&thread3, NULL, listen_routine_colas , conexionCaught);
	pthread_detach (thread);

}
