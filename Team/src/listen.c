/*
 * listen.c
 *
 *  Created on: 2 jun. 2020
 *      Author: utnso
 */

#include "listen.h"



void* listen_gameboy_routine (void *this_team)
{

    int socket= crearSocketServidor ("127.0.0.2", "5020");

    while (1) //Este while en realidad debería llevar como condición, que el proceso Team no haya ganado.
		{
		sleep (2);
		pthread_t thread;
		int socket_cliente = aceptarCliente (socket);

		pthread_create (&thread, NULL, (void *) get_opcode, &socket_cliente);
		pthread_detach (thread);
		printf ("Socket cliente: %d\n", socket_cliente);
		}
}


void* get_opcode (int *socket)
{
	int cod_op;
	cod_op=recibirOperacion(*socket);
	process_request(cod_op, socket);
}

void process_request (int cod_op, int *socket)
{
	int socket_cliente= *socket;
    switch (cod_op)
    	{
			case APPEARED_POKEMON:
				{
				mapPokemons *pokemon_to_add = malloc (sizeof(mapPokemons));
				t_appeared_pokemon* mensaje_appeared= recibirAppearedPokemon(socket_cliente);
				pokemon_to_add-> name = mensaje_appeared->nombre_pokemon;
				pokemon_to_add-> posx = mensaje_appeared->pos_x;
				pokemon_to_add-> posy= mensaje_appeared->pos_y;
				//pokemon_to_add->cantidad = 1;
				sem_wait(&poklist_sem2);
				list_add(mapped_pokemons, pokemon_to_add );
				sem_post(&poklist_sem);
				sem_post(&poklist_sem2);
				break;
				}
    	}

}

int send_catch (Trainer *trainer)
{
	sem_post(&using_cpu);
	t_catch_pokemon message;

	int socket = crearSocketCliente("127.0.01","5007");
	printf ("socket vale %d\n", socket);

	if (socket != -1)
	{

		message.nombre_pokemon=trainer->actual_objective.name;
		message.pos_x=trainer->actual_objective.posx;
		message.pos_y=trainer->actual_objective.posy;
		message.id_mensaje=0;

		enviarCatchPokemon (socket, message);
		sleep (5);

		recv (socket,&(message.id_mensaje),sizeof(uint32_t),MSG_WAITALL); //Recibir ID
		close (socket);

		int response;
		push_to_caugth (imessage.id_mensaje, &response); //Comunicar a la cola caught el ID a esperar
		sem_wait (trainer->trainer_sem); //Bloqueo el hilo hasta que llegue la respuesta
		return (response);

	} else
		 {
		 puts ("Fallo al conectarse al Broker");
		 return (DEFAULT_CATCH);
		 }



}

// ********************************MODIFICAR*******************************************
/*
 * Es conveniente que cuando el entrenador hace el catch, se cree un nuevo hilo que atienda todo
 * lo necesario y se bloquee al entrenador
 *
 * */

















search_caught_msg (catch_internal mensaje_catch)
{


		bool compare_id_corr (void *element)
		{
		catch_internal *mensaje_catch = element;

		if (id_corr  ==  (int*)element )
		return (true);
		else return (false);
		}

	    if (list_any_satisfy (cola_caught,compare_id_corr) == True)
	    //Encontré un mensaje con ese ID

	    return ();
}



void listen_caught_queue (void *)
{

	// Crear servidor y recibir mensajes del broker. Crear un hilo para tratar cada mensaje
}

