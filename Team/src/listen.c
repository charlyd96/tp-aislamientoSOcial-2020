/*
 * listen.c
 *
 *  Created on: 2 jun. 2020
 *      Author: utnso
 */
#include <conexion.h>
#include <team.h>


void* listen_routine (void *this_team)
{


    mapPokemons *pokemon_to_add = malloc (sizeof(mapPokemons));
    Team* team= this_team;
    int socket= crearSocketServidor ("127.0.0.2", "5010");

while (1)
    {
	sleep (2);
    int socket_cliente = aceptarCliente (socket);
    op_code opcode;
    opcode= recibirOperacion(socket_cliente);
    printf ("Socket cliente: %d\n", socket_cliente);
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
