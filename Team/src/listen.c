/*
 * listen.c
 *
 *  Created on: 2 jun. 2020
 *      Author: utnso
 */

#include "listen.h"



void * listen_routine_gameboy (void *retorno)
{

    int socket= crearSocketServidor (config->team_IP, config->team_port);
    while (1) //Este while en realidad debería llevar como condición, que el proceso Team no haya ganado.
		{
		pthread_t thread;
		int socket_cliente = aceptarCliente (socket);
		printf ("Socket cliente: %d\n", socket_cliente);
		get_opcode(socket_cliente);
		//pthread_create (&thread, NULL, (void *) get_opcode, (int*)socket_cliente);
		//pthread_detach (thread);
		}
}


void * get_opcode (int socket)
{
	op_code cod_op=recibirOperacion(socket);
	data_listen *data = malloc (sizeof (data_listen));
	data->socket=socket;
	data->op=cod_op;
	process_request_recv(data);
}

void process_request_recv (void *data)
{
	int socket_cliente=((data_listen*)data)->socket;
	int cod_op=((data_listen*)data)->op;
	free (data);
    switch (cod_op)
    	{
			case APPEARED_POKEMON:
				{
				mapPokemons *pokemon_to_add = malloc (sizeof(mapPokemons));
				t_appeared_pokemon* mensaje_appeared= recibirAppearedPokemon(socket_cliente);
				log_info (internalLogTeam, "Mensaje recibido: %s %s %d %d",colaParaLogs((int)cod_op),mensaje_appeared->nombre_pokemon,mensaje_appeared->pos_x,mensaje_appeared->pos_y);
				pokemon_to_add-> name = mensaje_appeared->nombre_pokemon;
				pokemon_to_add-> posx = mensaje_appeared->pos_x;
				pokemon_to_add-> posy = mensaje_appeared->pos_y;
				//pokemon_to_add->cantidad = 1;
				sem_wait(&poklist_sem2);
				list_add(mapped_pokemons, pokemon_to_add );
				sem_post(&poklist_sem);
				sem_post(&poklist_sem2);
				printf ("Se recibió un %s\n", mensaje_appeared->nombre_pokemon);
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
	pthread_join(thread,&retorno); //Se bloquea el hilo del entrenador

	return (*(int*)retorno);
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
	//printf ("Socket: %d\n", socket);
	log_info (logTeam,"Se enviará un mensaje CATCH %s %d %d", trainer->actual_objective.name, 
	trainer->actual_objective.posx, trainer->actual_objective.posy);
	
	

	if (socket != -1)
	{

		message.nombre_pokemon=trainer->actual_objective.name;
		message.pos_x=trainer->actual_objective.posx;
		message.pos_y=trainer->actual_objective.posy;
		message.id_mensaje=0;

		enviarCatchPokemon (socket, message);
		
		puts ("esperando ID");
		recv (socket,&(message.id_mensaje),sizeof(uint32_t),MSG_WAITALL); //Recibir ID
		close (socket);
		printf ("\t\t\t\t\t\t\tEl Id devuelto fue: %d\t\t\t\t\t\t\n", message.id_mensaje);
		int *retorno=malloc (sizeof (int));
		*retorno=search_caught (message.id_mensaje, &(trainer->trainer_sem) ); //Buscar en la cola caught con el ID correlativo
																			  //Esta función bloquea a este hilo hasta que 
																			  //el planificador de la CPU lo habilite nuevamente 
															
		return (retorno);

	} else
		 
			log_info (logTeam,"Fallo en la conexion al Broker. Se efectuará acción por defecto");
			int *retorno=malloc (sizeof (int));
			*retorno=DEFAULT_CATCH;
		 	return (retorno);
		 

}

void* listen_routine_colas (void *colaSuscripcion)

{	
	int socket_cliente =reintentar_conexion((op_code)colaSuscripcion);

	while (1)
	{
		op_code cod_op = recibirOperacion(socket_cliente);
		if (cod_op==OP_UNKNOWN)
		{
			socket_cliente = reintentar_conexion((op_code) colaSuscripcion);
		}
		else
		{
			data_listen *data= malloc (sizeof(data_listen));
			printf ("operacion:%d\n",cod_op);
			switch (cod_op)
			{
					case APPEARED_POKEMON:
					{
						data->socket=socket_cliente;
						data->op=cod_op;
						mapPokemons *pokemon_to_add = malloc (sizeof(mapPokemons));
						t_appeared_pokemon* mensaje_appeared= recibirAppearedPokemon(socket_cliente);
						log_info (internalLogTeam, "Mensaje recibido: %s %s %d %d",colaParaLogs((int)cod_op),mensaje_appeared->nombre_pokemon,mensaje_appeared->pos_x,mensaje_appeared->pos_y);
						pokemon_to_add-> name = mensaje_appeared->nombre_pokemon;
						pokemon_to_add-> posx = mensaje_appeared->pos_x;
						pokemon_to_add-> posy= mensaje_appeared->pos_y;
						//pokemon_to_add->cantidad = 1;
						sem_wait(&poklist_sem2);
						list_add(mapped_pokemons, pokemon_to_add );
						sem_post(&poklist_sem);
						sem_post(&poklist_sem2);
						printf ("Se recibió un %s\n", mensaje_appeared->nombre_pokemon);

						break;	
					}

					case LOCALIZED_POKEMON:
					{
											
						//*****PROCESAR LOCALIZED POKEMON ******
						//pthread_t thread;
						//pthread_create (&thread, NULL, (void *) get_opcode, (int*)socket);
						//pthread_detach (thread);	
						break;
					}

					case CAUGHT_POKEMON:
					{
											
						//*****PROCESAR CAUGHT POKEMON ******
						//pthread_t thread;
						//pthread_create (&thread, NULL, (void *) get_opcode, (int*)socket);
						//pthread_detach (thread);	
						break;
					}
					default: puts ("Operación inválida");
				}

		}
	}

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

int reintentar_conexion(op_code colaSuscripcion)
{
	int socket_cliente = crearSocketCliente (config->broker_IP,config->broker_port);
	t_suscribe colaAppeared;
	colaAppeared.tipo_suscripcion=SUSCRIBE_TEAM;
	colaAppeared.cola_suscribir=(int)colaSuscripcion;
	colaAppeared.id_proceso= 35;
	colaAppeared.timeout=0;
	int enviado=enviarSuscripcion (socket_cliente, colaAppeared);
	

	while (socket_cliente == -1)
		{
			log_info (internalLogTeam, "Falló la conexión al broker con la cola %s",colaParaLogs(colaSuscripcion));
			sleep (config->reconnection_time);
			socket_cliente = crearSocketCliente (config->broker_IP,config->broker_port);
			enviado=enviarSuscripcion (socket_cliente, colaAppeared);
			
		}
	log_info (internalLogTeam, "Conexión exitosa con la cola %s",colaParaLogs(colaSuscripcion));
	return (socket_cliente);
}



char* colaParaLogs(op_code cola){
	char* colaLog;

 	switch(cola){
		case NEW_POKEMON:{
			colaLog = "NEW_POKEMON";
			break;
		}
		case APPEARED_POKEMON:{
			colaLog = "APPEARED_POKEMON";
			break;
		}
		case CATCH_POKEMON:{
			colaLog = "CATCH_POKEMON";
			break;
		}
		case CAUGHT_POKEMON:{
			colaLog = "CAUGHT_POKEMON";
			break;
		}
		case GET_POKEMON:{
			colaLog = "GET_POKEMON";
			break;
		}
		case LOCALIZED_POKEMON:{
			colaLog = "LOCALIZED_POKEMON";
			break;
		}
		default:{
			colaLog = "NO ASIGNADA";
			break;
		}
	}

 	return colaLog;
}


void procesar_appearead(t_appearead)
{

}