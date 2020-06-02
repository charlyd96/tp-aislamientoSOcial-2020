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
#include <conexion.h>
#include <serializacion.h>
#include <protocolo.h>
#include <trainers.h>





/* Initializing all Team components */
Team * Team_Init(void)
{

    Team *this_team =malloc (sizeof(Team)) ;
    this_team->config = malloc (sizeof (Config));
    this_team->config->team_config = get_config();

    Team_load_trainers_config(this_team);
    Team_load_global_config(this_team->config);

    this_team->ReadyQueue= list_create  ();                 //*********Mejorar la ubicación de esta instrucción***************//
    this_team->mapped_pokemons = list_create();             //*********Mejorar la ubicación de esta instrucción***************//
    sem_init (&((this_team)->poklist_sem), 0, 0);           //*********Mejorar la ubicación de esta instrucción***************//
    sem_init ((&(this_team)->poklist_sem2), 0, 1);          //*********Mejorar la ubicación de esta instrucción***************//
    sem_init (&((this_team)->q_sem1), 0, 0);                 //*********Mejorar la ubicación de esta instrucción***************//
    sem_init (&((this_team)->q_sem2), 0, 1);                 //*********Mejorar la ubicación de esta instrucción***************//

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


exec_error fifo_exec (Team* this_team)
{
       Team *team= this_team;
       sem_wait ( &(this_team->q_sem1) );
       sem_wait ( &(this_team->q_sem2) );
       Trainer* trainer= list_get(team->ReadyQueue, 0);
       trainer->actual_status= EXECUTING_CATCH;
       sem_post ( &(this_team->q_sem2) );
       sem_post ( &(trainer->t_sem) );
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

void listen_new_pokemons (Team *this_team)
{
    pthread_t thread;
    pthread_create (&thread, NULL, listen_routine , this_team);
    pthread_detach (thread);
}

void* listen_routine (void *this_team)
{


    mapPokemons *pokemon_to_add = malloc (sizeof(mapPokemons));
    Team* team= this_team;
    int socket= crearSocketServidor ("127.0.0.2", "5002");

while (1)
    {
    int socket_cliente = aceptarCliente (socket);
    op_code opcode;
    opcode= recibirOperacion(socket_cliente);
    printf ("Codigo de operación: %d\n", opcode);
    switch (opcode) {
        case APPEARED_POKEMON:{
            t_appeared_pokemon* mensaje_appeared= recibirAppearedPokemon(socket_cliente);
            pokemon_to_add-> name = mensaje_appeared->nombre_pokemon;
            pokemon_to_add-> posx = mensaje_appeared->pos_x;
            pokemon_to_add-> posy= mensaje_appeared->pos_y;
            //pokemon_to_add-> cantidad = 1;
            sem_wait( &(team->poklist_sem2) );
            list_add( team->mapped_pokemons, pokemon_to_add );
            sem_post( &(team->poklist_sem) );
            sem_post( &(team->poklist_sem2) );


            break;}
    }


    /*
    pokemon_to_add-> name = readline("Ingrese nombre del pokemon: ");
    pokemon_to_add-> posx = atoi ( readline ("Ingrese pos_x del pokemon: " ) );
    pokemon_to_add-> posy= atoi ( readline ("Ingrese pos_y del pokemon: " ) );
    pokemon_to_add-> cant = atoi ( readline ("Ingrese cantidad del pokemon: " ) );
   */



    }
}
