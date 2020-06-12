/*
 * listen.c
 *
 *  Created on: 2 jun. 2020
 *      Author: utnso
 */

#include "listen.h"



void * listen_routine_gameboy (void *configuracion)
{
	Config *config=configuracion;

    int socket= crearSocketServidor (config->team_IP, config->team_port);
    printf ("Puerto= %s\n",config->team_port);
    printf ("IP= %s\n",config->team_IP);
    while (1) //Este while en realidad debería llevar como condición, que el proceso Team no haya ganado.
		{
		sleep (2);
		pthread_t thread;
		int socket_cliente = aceptarCliente (socket);
		printf ("Socket cliente: %d\n", socket_cliente);
		pthread_create (&thread, NULL, (void *) get_opcode, &socket_cliente);
		pthread_detach (thread);
		}
}


void * get_opcode (int *socket)
{
	op_code cod_op;
	cod_op=recibirOperacion(*socket);
	process_request_recv(cod_op, *socket);
}

void process_request_recv (op_code cod_op, int socket_cliente)
{

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
			case LOCALIZED_POKEMON:{ puts ("recibi localized");break;}

			case CAUGHT_POKEMON:{ puts ("recibi caught");break;}

			case OP_UNKNOWN: { puts ("operacion desconocida");break;}
			/*
			*Desarrollar
			*/
		break;
		}
}

int send_catch (Trainer *trainer)
{
	pthread_t thread;
	void *retorno;
	pthread_create(&thread, NULL, (void *) send_catch_routine, trainer);
	pthread_join(thread,&retorno);
	//sem_wait (&(trainer->trainer_sem));//Bloqueo el hilo hasta que llegue la respuesta
	//return (trainer->catch_result); 
	//Si esto funciona, sacar esta línea

	return (* (int*)retorno);
}

void * send_catch_routine (void * train)
{	
	Trainer *trainer=train;
	trainer->actual_status = BLOCKED_WAITING_REPLY;
	sem_post(&using_cpu); //Disponibilizar la CPU para otro entrenador
	
	char *IP= trainer->config->broker_IP;
	char *puerto= trainer->config->broker_port;
	t_catch_pokemon message;

	int socket = crearSocketCliente (IP,puerto);

	if (socket != -1)
	{

		message.nombre_pokemon=trainer->actual_objective.name;
		message.pos_x=trainer->actual_objective.posx;
		message.pos_y=trainer->actual_objective.posy;
		message.id_mensaje=0;

		printf ("Enviado: %d\n",enviarCatchPokemon (socket, message));
		puts ("catch enviado");
		recv (socket,&(message.id_mensaje),sizeof(uint32_t),MSG_WAITALL); //Recibir ID
		close (socket);

		int retorno;
		retorno=search_caught (message.id_mensaje, &(trainer->trainer_sem) ); //Buscar en la cola caught con el ID correlativo
																			  //Esta función bloquea a este hilo hasta que 
																			  //el planificador de la CPU lo habilite nuevamente 
																			
		return ((int *)retorno);

	} else
		 {
		 puts ("Fallo al conectarse al Broker");
		 return ((int*)DEFAULT_CATCH);
		 }

}

void* listen_routine_colas (void *con)

{
	conexionColas *conexion=con;
	int socket;

	socket = crearSocketCliente (conexion->broker_IP,conexion->broker_port);
	while (socket == -1)
	{
		sleep (conexion->tiempo_reconexion);
	    socket = crearSocketCliente (conexion->broker_IP,conexion->broker_port);
	}

	t_suscribe colaAppeared;
	colaAppeared.tipo_suscripcion=SUSCRIBE_TEAM;
	colaAppeared.cola_suscribir=conexion->colaSuscripcion;
	colaAppeared.timeout=0;
	
	enviarSuscripcion (socket, colaAppeared);
	puts ("me voy a bloqear");
	uint32_t dato;
	recv (socket, &dato,sizeof(uint32_t),MSG_WAITALL);
	printf ("dato= %d\n");
	//op_code cod_op = recibirOperacion(socket);
	puts ("me bloqueé");
	//process_request_recv(cod_op, socket);
	

	//Procesar espera de mensajes de cada cola

}

// ********************************MODIFICAR*******************************************
/*
 * Es conveniente que cuando el entrenador hace el catch, se cree un nuevo hilo que atienda todo
 * lo necesario y se bloquee al entrenador
 *
 * */




int search_caught (u_int32_t id_corr, sem_t *trainer_sem)
{


	bool compare_id_corr (void *element)
	{
		if (id_corr  ==  *(u_int32_t*)element)
		return (true);
		else return (false);
	}

	if (list_any_satisfy (cola_caught,compare_id_corr) == true)
	{
		sem_wait (trainer_sem);
		return (true);//Encontré un mensaje con ese ID
	} 
	else 
	{
		sem_wait (trainer_sem);
		return (false);//No encontré un mensaje con ese ID
	} 
	
}
/**
 * Preguntar si los mensajes CAUGHT pueden guardarse todos, sin tener en cuenta el ID correlativo
 * El motivo es que si cuando el broker me devuelve el ID, el planificador del SO tarda un rato en volver
 * a planificar a ese hilo que debe informar a CAUGHT, el broker podría haber enviado la respueta CAUGHT 
 * correspondiente antes de que yo le haya avisado a CAUGHT qué mensaje filtrar
 * 
*/

/*
void listen_caught_queue (void *)
{

	// Crear servidor y recibir mensajes del broker. Crear un hilo para tratar cada mensaje
}

*/
