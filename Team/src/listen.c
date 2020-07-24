/*
 * listen.c
 *
 *  Created on: 2 jun. 2020
 *      Author: utnso
 */

#include "listen.h"



void  listen_routine_gameboy ()
{
	
    socketGameboy= crearSocketServidor (config->team_IP, config->team_port);
	if (socketGameboy == -1)
	{
		log_error(internalLogTeam, "No se pudo crear el servidor de escucha del Gameboy");
	}
	else
	{
		while (1) //Este while en realidad debería llevar como condición, que el proceso Team no haya ganado.
			{
			pthread_t thread;
			socketGameboyCliente = aceptarCliente (socketGameboy);
			if (win)
			{
				log_info(internalLogTeam,"fin escucha");
				break;
			} 
			log_info(internalLogTeam,"Socket cliente: %d\n", socketGameboyCliente);
			get_opcode(socketGameboyCliente);
			pthread_create (&thread, NULL, (void *) get_opcode, (int*)socketGameboyCliente);
			pthread_detach (thread);
			} 
	}
}


void get_opcode (int socket)
{
	
	op_code cod_op=recibirOperacion(socket);
	recibirTipoProceso(socket);
	recibirIDProceso(socket);
	process_request_recv(cod_op, socket);
}

void process_request_recv (op_code cod_op, int socket_cliente)
{
    switch (cod_op)
    	{
			case APPEARED_POKEMON:
				{
				t_appeared_pokemon* mensaje_appeared= recibirAppearedPokemon(socket_cliente);
				enviarACK(socket_cliente);
				log_info (internalLogTeam, "Mensaje recibido: %s %s %d %d",colaParaLogs((int)cod_op),mensaje_appeared->nombre_pokemon,mensaje_appeared->pos_x,mensaje_appeared->pos_y);
				procesar_appeared(mensaje_appeared);
				break;
				}
			case CAUGHT_POKEMON:
				{
				t_caught_pokemon* mensaje_caught= recibirCaughtPokemon(socket_cliente);
				enviarACK(socket_cliente);
				log_info (internalLogTeam, "Mensaje recibido: %s %d %d",colaParaLogs((int)cod_op),mensaje_caught->atrapo_pokemon, mensaje_caught->id_mensaje_correlativo);
				procesar_caught(mensaje_caught);
				break;					
				} 

			case OP_UNKNOWN: 
				{ 
				log_error (internalLogTeam, "Operacion desconocidaaa");
				break;
				}

			default: log_error (internalLogTeam, "Operacion no reconocida por el sistema");
		}
}


int send_catch (Trainer *trainer)
{	
	trainer->actual_status = BLOCKED_WAITING_REPLY;
		
	char *IP= config->broker_IP;
	char *puerto= config->broker_port;
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

		enviarCatchPokemon (socket, message, P_TEAM, ID_proceso);
    	trainer->ejecucion=FINISHED;
		sem_post(&using_cpu); //Disponibilizar la CPU para otro entrenador
		recv (socket,&(message.id_mensaje),sizeof(uint32_t),MSG_WAITALL); //Recibir ID
		close (socket);
		log_info(internalLogTeam,"\t\t\t\t\t\t\tEl Id devuelto fue: %d. Pertenece al pokemon %s\t\t\t\t\t\t\n", message.id_mensaje, message.nombre_pokemon);
		
		int resultado = informarIDcaught(message.id_mensaje, &(trainer->trainer_sem));
								 												  //Buscar en la cola caught con el ID correlativo
		return (resultado);														  //Esta función bloquea a este hilo hasta que 
																			 	  //el planificador de la CPU lo habilite nuevamente 																	
	}	
	log_info (logTeam,"Fallo en la conexion al Broker. Se efectuará acción por defecto");
	trainer->ejecucion=FINISHED;
	sem_post(&using_cpu); //Disponibilizar la CPU para otro entrenador

	return (DEFAULT_CATCH);	 
}

void listen_routine_colas (void *colaSuscripcion)

{	
	switch ((op_code)colaSuscripcion)
	{
		case APPEARED_POKEMON:
		{
			socketAppeared=-1;
			while(!win)
			{
				op_code cod_op = recibirOperacion(socketAppeared);
				if (cod_op==OP_UNKNOWN)
				{
					sem_wait(&terminar_appeared);
					socketAppeared = reintentar_conexion((op_code) colaSuscripcion);
					sem_post(&terminar_appeared);
				}
				else
				{
					recibirTipoProceso(socketAppeared);
					recibirIDProceso(socketAppeared);
					t_appeared_pokemon* mensaje_appeared= recibirAppearedPokemon(socketAppeared);
					enviarACK(socketAppeared);
					log_info (internalLogTeam, "Mensaje recibido: %s %s %d %d",colaParaLogs((int)cod_op),mensaje_appeared->nombre_pokemon,mensaje_appeared->pos_x,mensaje_appeared->pos_y);
					pthread_t thread;
					pthread_create (&thread, NULL, (void *) procesar_appeared, mensaje_appeared);
					pthread_detach (thread);	
				}
			}
			log_info(internalLogTeam,"cerrando appearead"); 
			break;
		}

		case LOCALIZED_POKEMON:
		{						

			socketLocalized=-1;					
			while(!win)
			{
				op_code cod_op = recibirOperacion(socketLocalized);
				if (cod_op==OP_UNKNOWN)
				{
					sem_wait(&terminar_localized);
					socketLocalized = reintentar_conexion((op_code) colaSuscripcion);
					sem_post(&terminar_localized);
				}
				else
				{
					recibirTipoProceso(socketLocalized);
					recibirIDProceso(socketLocalized);
					t_localized_pokemon* mensaje_localized= recibirLocalizedPokemon(socketLocalized);
					enviarACK(socketLocalized);
					log_info (internalLogTeam, "Mensaje recibido: %s %s %d %s [%d]",colaParaLogs((int)cod_op),mensaje_localized->nombre_pokemon,mensaje_localized->cant_pos,mensaje_localized->posiciones,mensaje_localized->id_mensaje_correlativo);
					pthread_t thread;
					pthread_create (&thread, NULL, (void *) procesar_localized, mensaje_localized);
					pthread_detach (thread);
				}					
			}log_info(internalLogTeam,"cerrando caught"); break;
		}
		case CAUGHT_POKEMON:
		{	
			socketCaught=-1;					
			while(!win)
			{
				op_code cod_op = recibirOperacion(socketCaught);
				if (cod_op==OP_UNKNOWN)
				{
					sem_wait(&terminar_caught);
					socketCaught = reintentar_conexion((op_code) colaSuscripcion);
					sem_post(&terminar_caught);
				}
				else
				{
					recibirTipoProceso(socketCaught);
					recibirIDProceso(socketCaught);
					t_caught_pokemon* mensaje_caught= recibirCaughtPokemon(socketCaught);
					enviarACK(socketCaught);
					log_info (internalLogTeam, "Mensaje recibido: %s %d %d",colaParaLogs((int)cod_op),mensaje_caught->atrapo_pokemon, mensaje_caught->id_mensaje_correlativo);
					pthread_t thread;
					pthread_create (&thread, NULL, (void *) procesar_caught, mensaje_caught);
					pthread_detach (thread);
				}					
			}log_info(internalLogTeam,"cerrando caught"); break;
		}

		case OP_UNKNOWN:
		{
			log_error (internalLogTeam, "Operacion inválida");
			break;
		}

		default: log_error (internalLogTeam, "Operacion no reconocida por el sistema");
	}
}


int reintentar_conexion(op_code colaSuscripcion)
{
	int socket_cliente = crearSocketCliente (config->broker_IP,config->broker_port);
	t_suscribe colaAppeared;
	colaAppeared.tipo_suscripcion=SUSCRIBE_TEAM;
	colaAppeared.cola_suscribir=(int)colaSuscripcion;
	colaAppeared.id_proceso= ID_proceso;
	colaAppeared.timeout=0;
	int enviado=enviarSuscripcion (socket_cliente, colaAppeared);
	
	while ( (socket_cliente == -1 || enviado==0) && !win )
		{
			log_info (internalLogTeam, "Falló la conexión al broker con la cola %s",colaParaLogs(colaSuscripcion));
			sleep (config->reconnection_time);
			socket_cliente = crearSocketCliente (config->broker_IP,config->broker_port);
			enviado=enviarSuscripcion (socket_cliente, colaAppeared);
		}
	if (!win) log_info (internalLogTeam, "Conexión exitosa con la cola %s", colaParaLogs(colaSuscripcion));
	return (socket_cliente);
}

void procesar_appeared(t_appeared_pokemon *mensaje_appeared)
{
	nuevo_pokemon operacion = tratar_nuevo_pokemon (mensaje_appeared->nombre_pokemon);
	log_info(internalLogTeam,"Operacion: %d - Pokemon:%s",operacion,mensaje_appeared->nombre_pokemon);

	switch (operacion)
	{
		case GUARDAR:
		{
			agregar_nueva_especie(mensaje_appeared->nombre_pokemon);
			mapPokemons *pokemon_to_add = malloc (sizeof(mapPokemons));
			pokemon_to_add-> name = string_duplicate (mensaje_appeared->nombre_pokemon);
			pokemon_to_add-> posx = mensaje_appeared->pos_x;
			pokemon_to_add-> posy= mensaje_appeared->pos_y;
			sem_wait(&poklist_sem2);
			list_add(mapped_pokemons, pokemon_to_add );
			sem_post(&poklist_sem);
			sem_post(&poklist_sem2);
			liberar_appeared(mensaje_appeared);
			break;
		}

		case GUARDAR_AUX:
		{
			agregar_nueva_especie(mensaje_appeared->nombre_pokemon);
			mapPokemons *pokemon_to_add = malloc (sizeof(mapPokemons));
			pokemon_to_add-> name = string_duplicate (mensaje_appeared->nombre_pokemon);
			pokemon_to_add-> posx = mensaje_appeared->pos_x;
			pokemon_to_add-> posy= mensaje_appeared->pos_y;
			sem_wait(&poklistAux_sem2); //Verificar si es necesario el esquema de productor consumidor
			list_add(mapped_pokemons_aux, pokemon_to_add );
			sem_post(&poklistAux_sem1);
			sem_post(&poklistAux_sem2);
			liberar_appeared(mensaje_appeared);
			break;
		}

		case DESCARTAR:
		{
			liberar_appeared(mensaje_appeared);
			break;
		}

		default: 
		{
			liberar_appeared(mensaje_appeared);
			log_error (internalLogTeam, "Operación desconocida al recibir appeared");
		}
	}
}

void procesar_localized(t_localized_pokemon *mensaje_localized)
{
	bool agregar=false;
	bool buscar_id_corr (void *id_corr)
	{
		uint32_t id= *(uint32_t*)id_corr;
		if (id == mensaje_localized->id_mensaje_correlativo)
		{
		agregar=true;
		return true;
		}
		else return false;
	}
	pthread_mutex_lock (&ID_localized_sem); 
	void * elemento = list_remove_by_condition(ID_localized, buscar_id_corr);
	pthread_mutex_unlock (&ID_localized_sem); 
	if (elemento != NULL)
	free(elemento);


	if (mensaje_localized->cant_pos == 0 || agregar==false)
	{
		//liberar_localized(mensaje_localized);
	}
	else
	{
		log_info(internalLogTeam,"Cantidad posiciones: %d\n", mensaje_localized->cant_pos);
		char **coordenadas;
		char **posiciones = string_get_string_as_array(mensaje_localized->posiciones);
		for (int i=0; *(posiciones+i) != NULL; i++)
		{
			coordenadas= string_split(*(posiciones+i), "|" );
			uint32_t pos_x= atoi (*(coordenadas));
			uint32_t pos_y= atoi (*(coordenadas +1));
			t_appeared_pokemon *mensaje_appeared = malloc (sizeof (t_appeared_pokemon));
			mensaje_appeared->nombre_pokemon = mensaje_localized->nombre_pokemon;
			mensaje_appeared->pos_x=pos_x;
			mensaje_appeared->pos_y=pos_y;
			procesar_appeared(mensaje_appeared);
		}
		free_split(posiciones);
		free_split(coordenadas);
	}
	//else liberar_localized(mensaje_localized);
}

void procesar_caught (t_caught_pokemon *mensaje_caught)
{	
	uint32_t id_corr = mensaje_caught->id_mensaje_correlativo;
	uint32_t resultado = mensaje_caught->atrapo_pokemon;

	bool buscar_id_corr (void *element)
	{
		internal_caught *nodoCaught = element;
		if (id_corr  ==  nodoCaught->id)
		{
			if (resultado==1)
			*(nodoCaught->resultado)=true;
			else *(nodoCaught->resultado)=false;
			sem_post(nodoCaught->trainer_sem);
			return (true);	
		}
		else return (false);
	}
	pthread_mutex_lock (&ID_caught_sem); 
	void *nodo= list_remove_by_condition(ID_caught, buscar_id_corr);
	pthread_mutex_unlock (&ID_caught_sem);
	if (nodo !=NULL)
	free(nodo);
}

int informarIDcaught(uint32_t id, sem_t *trainer_sem)
{
	bool resultado;
	internal_caught *nodoCaught = malloc(sizeof(internal_caught));
	
	nodoCaught->trainer_sem=trainer_sem;
	nodoCaught->id=id;
	nodoCaught->resultado=&resultado;
	
    pthread_mutex_lock (&ID_caught_sem);   
	list_add(ID_caught, nodoCaught);
	pthread_mutex_unlock (&ID_caught_sem);

	sem_wait(trainer_sem); //Bloqueo al entrenador
	return(resultado);
}

void informarIDlocalized(uint32_t id)
{
	uint32_t *id_corr = malloc (sizeof(uint32_t));
	*id_corr=id;
    pthread_mutex_lock (&ID_localized_sem);   
	list_add(ID_localized, id_corr);
	pthread_mutex_unlock (&ID_localized_sem);
}

void agregar_nueva_especie (char *nueva_especie)
{
	bool comparar_especie (void *especie_atrapada)
	{
		if (!strcmp(especie_atrapada,nueva_especie))
		return true; else return false;
	}
	pthread_mutex_lock(&especies_sem);
	if (!list_any_satisfy(especies,comparar_especie))
	list_add(especies,nueva_especie);
	pthread_mutex_unlock(&especies_sem);
}

bool especie_necesaria (char *nueva_especie)
{
	bool retorno;
	bool comparar_especie (void *especie_atrapada)
	{
		if (!strcmp(especie_atrapada,nueva_especie))
		return true; else return false;
	}
	pthread_mutex_lock(&especies_sem);
	retorno = !list_any_satisfy(especies,comparar_especie);
	pthread_mutex_unlock(&especies_sem);
	return (retorno);
}

nuevo_pokemon tratar_nuevo_pokemon (char *nombre_pokemon)
{
	char *a_lista_auxiliar=NULL;
	bool buscar (void *nombre)
	{
		if (!strcmp( (char*)nombre,nombre_pokemon))
		{
			return true;
		} else return false;
	}
	pthread_mutex_lock(&new_global_sem);
	a_lista_auxiliar= list_remove_by_condition(new_global_objective, buscar); //Busco si el pokemon recibido está en la lista de objetivos globales
	pthread_mutex_unlock(&new_global_sem);

	if (a_lista_auxiliar!=NULL) //Si está en la lista global, lo muevo a la auxiliar
	{
		pthread_mutex_lock(&aux_new_global_sem);
		list_add(aux_new_global_objective, a_lista_auxiliar);
		pthread_mutex_unlock(&aux_new_global_sem);
		return (GUARDAR); //Guardar en el mapa principal
	}
	else //Si no está en la lista global, lo busco en la lista global auxiliar
	{
		pthread_mutex_lock(&aux_new_global_sem);
		if ( list_any_satisfy(aux_new_global_objective, buscar) )
		{
			pthread_mutex_unlock(&aux_new_global_sem);
			return (GUARDAR_AUX);
		}
		else
		{
			pthread_mutex_unlock(&aux_new_global_sem);
			return DESCARTAR;
		} 
	} 
}

void liberar_appeared (t_appeared_pokemon *mensaje)

{
	//free (mensaje->nombre_pokemon);
	//free (mensaje);
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