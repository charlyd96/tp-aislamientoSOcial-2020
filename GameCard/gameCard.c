/*
 * gameCard.c
 *
 *  Created on: 28 abr. 2020
 *      Author: aislamientoSOcial
 */

#include "gamecard.h"

void levantarPuertoEscucha(void){
	int socketServidorGamecard = crearSocketServidor(config_gamecard->ip_gamecard, config_gamecard->puerto_gamecard);
	
	if(socketServidorGamecard == -1){
		log_info(logInterno, "No se pudo crear el Servidor Game Card.");
	}else{
		log_info(logInterno, "Socket Servidor %d", socketServidorGamecard);
		while(1){ //Verificar la condición de salida de este while
					// log_info(logInterno,"While de aceptar cliente");
					int cliente = aceptarCliente(socketServidorGamecard);
					int enviado = enviarACK(cliente);
					if(enviado > 0){
						log_info(logInterno,"Se envió el ACK al Game Boy (Socket %d).",cliente);
					}else{
						log_error(logInterno,"ERROR: No se pudo enviar el ACK al Game Boy (Socket %d).",cliente);
					}
					pthread_t hiloCliente;
					pthread_create(&hiloCliente, NULL, (void*)atender_cliente, (void*)cliente);

					pthread_detach(hiloCliente);
				}
		close(socketServidorGamecard);
		log_info(logInterno, "Se cerro el Soket Servidor %d.", socketServidorGamecard);
	}
}
int reintentar_conexion(op_code colaSuscripcion)
{
	int socket_cliente = crearSocketCliente (config_gamecard->ip_broker,config_gamecard->puerto_broker);
	t_suscribe suscripcion;
	suscripcion.tipo_suscripcion=SUSCRIBE_GAMECARD;
	suscripcion.cola_suscribir=(int)colaSuscripcion;
	suscripcion.id_proceso= ID_PROCESO;
	suscripcion.timeout=0;

	int enviado=enviarSuscripcion (socket_cliente, suscripcion);

	while (socket_cliente == -1 || enviado==0)
		{
			log_info(logGamecard, "Fallo de Conexión con Broker %s : %s a la cola %s.",config_gamecard->ip_broker,config_gamecard->puerto_broker,colaParaLogs(colaSuscripcion));
			sleep (config_gamecard->tiempo_reintento_conexion);
			socket_cliente = crearSocketCliente (config_gamecard->ip_broker,config_gamecard->puerto_broker);
			enviado=enviarSuscripcion (socket_cliente, suscripcion);
		}
	log_info (logGamecard, "Suscripción del Game Card %d a la cola %s.", suscripcion.id_proceso, colaParaLogs(colaSuscripcion));
	return (socket_cliente);
}
void listen_routine_colas (void *colaSuscripcion){

	int socket_cliente;
	socket_cliente =reintentar_conexion((op_code)colaSuscripcion);

	switch ((op_code)colaSuscripcion)
	{
		case NEW_POKEMON:
		{
			while(1)
			{
				log_info(logInterno, "Recibir NEW_POKEMON.");
				op_code cod_op = recibirOperacion(socket_cliente);
				if (cod_op==OP_UNKNOWN)
					socket_cliente = reintentar_conexion((op_code) colaSuscripcion);
				else
				{
					process_code tipo_proceso = recibirTipoProceso(socket_cliente);
					uint32_t PID = recibirIDProceso(socket_cliente);
					log_info(logInterno,"Se conectó el Proceso %s [%u]",tipoProcesoParaLogs(tipo_proceso),PID);
					t_new_pokemon *new_pokemon =recibirNewPokemon (socket_cliente);
					log_info(logGamecard, "Recepción del Mensaje NEW_POKEMON %s %u %u %u [%u].",new_pokemon->nombre_pokemon,new_pokemon->pos_x,new_pokemon->pos_y,new_pokemon->cantidad,new_pokemon->id_mensaje);
					int enviado = enviarACK(socket_cliente);
					if(enviado > 0){
						log_info(logGamecard,"Se envió el ACK del Mensaje NEW_POKEMON con ID de Mensaje [%u].", new_pokemon->id_mensaje);		
					}else{
						log_warning(logGamecard,"No se pudo enviar el ACK del Mensaje NEW_POKEMON con ID de Mensaje [%u].", new_pokemon->id_mensaje);
					}
					pthread_t thread;
					pthread_create (&thread, NULL, (void *) atender_newPokemon, new_pokemon);
					pthread_detach (thread);
				}

			} break;
		}

		case GET_POKEMON:
		{
			while(1)
			{
				log_info(logInterno, "Recibir GET_POKEMON.");
				op_code cod_op = recibirOperacion(socket_cliente);

				if (cod_op==OP_UNKNOWN)
					socket_cliente = reintentar_conexion((op_code) colaSuscripcion);
				else
				{
					process_code tipo_proceso = recibirTipoProceso(socket_cliente);
					uint32_t PID = recibirIDProceso(socket_cliente);
					log_info(logInterno,"Se conectó el Proceso %s [%u]",tipoProcesoParaLogs(tipo_proceso),PID);
					t_get_pokemon *get_pokemon =recibirGetPokemon (socket_cliente);
					log_info(logGamecard, "Recepción del Mensaje GET_POKEMON %s [%u].",get_pokemon->nombre_pokemon,get_pokemon->id_mensaje);
					int enviado = enviarACK(socket_cliente);
					if(enviado > 0){
						log_info(logGamecard,"Se envió el ACK del Mensaje GET_POKEMON con ID de Mensaje [%u].", get_pokemon->id_mensaje);			
					}else{
						log_info(logGamecard,"No se pudo enviar el ACK del Mensaje GET_POKEMON con ID de Mensaje [%d]",get_pokemon->id_mensaje);
					}
					pthread_t thread;
					pthread_create (&thread, NULL, (void *) atender_getPokemon, get_pokemon);
					pthread_detach (thread);
				}
			} break;
		}

		case CATCH_POKEMON:
		{
			while(1)
			{
				log_info(logInterno, "Recibir CATCH_POKEMON.");
				op_code cod_op = recibirOperacion(socket_cliente);
				if (cod_op==OP_UNKNOWN)
					socket_cliente = reintentar_conexion((op_code) colaSuscripcion);
				else
				{
					process_code tipo_proceso = recibirTipoProceso(socket_cliente);
					uint32_t PID = recibirIDProceso(socket_cliente);
					log_info(logInterno,"Se conectó el Proceso %s [%u]\n",tipoProcesoParaLogs(tipo_proceso),PID);
					t_catch_pokemon *catch_pokemon = recibirCatchPokemon(socket_cliente);
					log_info(logGamecard, "Recepción del Mensaje CATCH_POKEMON %s %u %u [%u].",catch_pokemon->nombre_pokemon,catch_pokemon->pos_x,catch_pokemon->pos_y,catch_pokemon->id_mensaje);
					int enviado = enviarACK(socket_cliente);
					if(enviado > 0){
						log_info(logGamecard,"Se envió el ACK del Mensaje con ID de Mensaje [%d].", catch_pokemon->id_mensaje);	
					}else{
						log_info(logGamecard,"No se pudo enviar el ACK del Mensaje con ID de Mensaje [%d].", catch_pokemon->id_mensaje);
					}
					pthread_t thread;
					pthread_create (&thread, NULL, (void *) atender_catchPokemon, catch_pokemon);
					pthread_detach (thread);
				}
			} break;
		}

		case OP_UNKNOWN:
		{
			log_error (logInterno, "ERROR: Operacion inválida");
			break;
		}

		default: log_error (logInterno, "ERROR: Operacion no reconocida por el sistema");
	}
}
void subscribe()
{
	pthread_t thread1; //OJO. Esta variable se está perdiendo
	pthread_create (&thread1, NULL, (void*)listen_routine_colas , (int*)NEW_POKEMON);
	pthread_detach (thread1);

	pthread_t thread2; //OJO. Esta variable se está perdiendo
	pthread_create (&thread2, NULL, (void*) listen_routine_colas , (int*)GET_POKEMON);
	pthread_detach (thread2);

	pthread_t thread3; //OJO. Esta variable se está perdiendo
	pthread_create (&thread3, NULL, (void*) listen_routine_colas , (int*)CATCH_POKEMON);
	pthread_detach (thread3);
}

int main(int argc, char** argv){
	logGamecard = log_create("gamecard.log", "Game Card", 1, LOG_LEVEL_TRACE);

	if(argc != 2){
		log_error(logGamecard,"Debe ingresar un ID de Proceso. Ejemplo: ./Gamecard 1");
		log_destroy(logGamecard);
		exit(-1);
	}
	ID_PROCESO = atoi(argv[1]);
	sem_init(&mx_file_metadata,0,(unsigned int) 1);
	sem_init(&mx_creacion_archivo,0,(unsigned int) 1);
	sem_init(&mx_w_bloques,0,(unsigned int) 1);
	logInterno = log_create("gamecardInterno.log", "Game Card Interno", 0, LOG_LEVEL_TRACE);

	log_info(logInterno,"ID seteado %d",ID_PROCESO);


	log_trace(logGamecard, "****************************************** PROCESO GAME CARD ******************************************");
	
	crear_config_gamecard();
	leer_FS_metadata(config_gamecard);
	log_info(logInterno,"ip %s:%s\n",config_gamecard->ip_gamecard,config_gamecard->puerto_gamecard);

	//Creo un hilo para el socket de escucha
	pthread_t hiloEscucha;
	pthread_create(&hiloEscucha, NULL, (void*)levantarPuertoEscucha,NULL);

	//Conexión a broker
	subscribe();
	pthread_join(hiloEscucha,NULL);

	sem_destroy(&mx_creacion_archivo);
	sem_destroy(&mx_file_metadata);
	sem_destroy(&mx_w_bloques);
	log_destroy(logInterno);
	log_destroy(logGamecard);

}


void crear_config_gamecard(){

    log_info(logInterno, "Inicializacion del log.");
	config_gamecard = malloc(sizeof(t_configuracion));
	
	config_ruta = config_create(pathGamecardConfig);
	
	if (config_ruta == NULL){
	log_error(logInterno, "ERROR: No se pudo levantar el archivo de configuración.");
	exit (FILE_CONFIG_ERROR);
	}

	config_gamecard->tiempo_reintento_conexion = config_get_int_value(config_ruta, "TIEMPO_DE_REINTENTO_CONEXION");
	config_gamecard->tiempo_reintento_operacion = config_get_int_value(config_ruta, "TIEMPO_DE_REINTENTO_OPERACION");
	config_gamecard->tiempo_retardo_operacion = config_get_int_value(config_ruta, "TIEMPO_RETARDO_OPERACION");
	config_gamecard->punto_montaje = string_duplicate (config_get_string_value(config_ruta, "PUNTO_MONTAJE_TALLGRASS"));
	config_gamecard->ip_gamecard = string_duplicate (config_get_string_value(config_ruta, "IP_GAMECARD"));
	config_gamecard->puerto_gamecard = string_duplicate (config_get_string_value(config_ruta, "PUERTO_GAMECARD"));
	config_gamecard->ip_broker = string_duplicate (config_get_string_value(config_ruta, "IP_BROKER"));
	config_gamecard->puerto_broker = string_duplicate (config_get_string_value(config_ruta, "PUERTO_BROKER"));
	
	config_destroy (config_ruta);
}

void leer_FS_metadata (t_configuracion *config_gamecard){
	char *pathMetadata = string_from_format ("%s/Metadata/Metadata.bin", config_gamecard->punto_montaje);
	if(existe_archivo(pathMetadata)){
		FS_config =malloc (sizeof (t_FS_config));
		FSmetadata = config_create (pathMetadata);

		FS_config->BLOCK_SIZE=config_get_int_value(FSmetadata, "BLOCK_SIZE");
		FS_config->BLOCKS=config_get_int_value(FSmetadata, "BLOCKS");
		FS_config->MAGIC_NUMBER=string_duplicate (config_get_string_value(FSmetadata, "MAGIC_NUMBER"));
		config_destroy (FSmetadata);
		log_info (logGamecard, "Se leyó correctamente el archivo Metadata del File System.");			
	}
	else {
		//config_destroy (FSmetadata);
		log_error(logGamecard, "ERROR: No se pudo leer el archivo Metadata del File System.");
		exit (FILE_FSMETADATA_ERROR);
	}
	free(pathMetadata);
}


bool existe_archivo(char* path){
	log_info(logInterno,"Consulto si existe: %s",path);
	FILE * file = fopen(path, "r");
	if(file!=NULL){
		fclose(file);
		return true;
	} else{
		return false;
	}
}

void atender_cliente(int socket){
	log_info(logInterno, "Atender cliente %d",socket);
	op_code cod_op = recibirOperacion(socket);
	process_code tipo_proceso = recibirTipoProceso(socket);
	uint32_t PID = recibirIDProceso(socket);
	log_info(logInterno,"Se conectó el proceso %s [%u]",tipoProcesoParaLogs(tipo_proceso),PID);
	switch(cod_op){
		case NEW_POKEMON:{
			t_new_pokemon *new_pokemon=recibirNewPokemon(socket);
			atender_newPokemon(new_pokemon);
		break;
		}	
		case GET_POKEMON:{
			t_get_pokemon *get_pokemon=recibirGetPokemon(socket);
			atender_getPokemon(get_pokemon);
			break;
		}

		case CATCH_POKEMON:{
			t_catch_pokemon *catch_pokemon=recibirCatchPokemon(socket);
			atender_catchPokemon(catch_pokemon);
			break;
		}
		default:
			log_info(logInterno,"La operación no es correcta");
			break;
	}
}
int enviarAppearedAlBroker(t_new_pokemon * new_pokemon){
	int socket_cliente = crearSocketCliente (config_gamecard->ip_broker,config_gamecard->puerto_broker);
	if(socket_cliente <= 0){
		log_warning(logGamecard,"No se pudo conectar al Broker para enviar APPEARED_POKEMON.");
		// close(socket_cliente);
		return socket_cliente;
	}else{
		t_appeared_pokemon* appeared_pokemon = malloc(sizeof(t_appeared_pokemon));
		appeared_pokemon->nombre_pokemon = new_pokemon->nombre_pokemon;
		appeared_pokemon->pos_x = new_pokemon->pos_x;
		appeared_pokemon->pos_y = new_pokemon->pos_y;
		appeared_pokemon->id_mensaje_correlativo = new_pokemon->id_mensaje;

		int app_enviado = enviarAppearedPokemon(socket_cliente, *appeared_pokemon, P_GAMECARD, ID_PROCESO);
		if(app_enviado > 0){
			log_info(logGamecard,"Se envió el Mensaje APPEARED_POKEMON %s %u %u [%u].",appeared_pokemon->nombre_pokemon,appeared_pokemon->pos_x,appeared_pokemon->pos_y,appeared_pokemon->id_mensaje_correlativo);
			//Reutilizo esta funcion para el id_mensaje
			uint32_t id_mensaje = recibirIDProceso(socket_cliente);
			if(id_mensaje>0){
				log_info(logInterno,"APPEARED con correlativo [%u] recibió ID_MENSAJE %u",appeared_pokemon->id_mensaje_correlativo,id_mensaje);
			}else{
				log_info(logInterno,"Error al recibir el ID para APPEARED con correlativo [%u]",appeared_pokemon->id_mensaje_correlativo);
			}
		}else{
			log_warning(logGamecard,"No se pudo enviar el Mensaje APPEARED_POKEMON.");
		}
		close(socket_cliente);
		free(appeared_pokemon);
		return app_enviado;
	}
}
int enviarLocalizedAlBroker(t_localized_pokemon * msg_localized){
	int socket_cliente = crearSocketCliente (config_gamecard->ip_broker,config_gamecard->puerto_broker);
	if(socket_cliente <= 0){
		log_warning(logGamecard,"No se pudo conectar al Broker para enviar LOCALIZED_POKEMON.");
		// close(socket_cliente);
		return socket_cliente;
	}else{
		int enviado = enviarLocalizedPokemon(socket_cliente,*msg_localized,P_GAMECARD,ID_PROCESO);
		if(enviado > 0){
			log_info(logGamecard,"Se envió el Mensaje LOCALIZED_POKEMON %s %u %s [%u].",msg_localized->nombre_pokemon,msg_localized->cant_pos,msg_localized->posiciones,msg_localized->id_mensaje_correlativo);
			//Reutilizo esta funcion para el id_mensaje
			uint32_t id_mensaje = recibirIDProceso(socket_cliente);
			if(id_mensaje>0){
				log_info(logInterno,"LOCALIZED con correlativo [%u] recibió ID_MENSAJE %u",msg_localized->id_mensaje_correlativo,id_mensaje);
			}else{
				log_info(logInterno,"Error al recibir el ID para LOCALIZED con correlativo [%u]",msg_localized->id_mensaje_correlativo);
			}
		}else{
			log_warning(logGamecard,"No se pudo enviar el Mensaje LOCALIZED_POKEMON.");
		}
		close(socket_cliente);
		if(msg_localized->cant_pos > 0)
			free(msg_localized->posiciones);
		free(msg_localized->nombre_pokemon);
		free(msg_localized);
		return enviado;
	}
}
int enviarCaughtAlBroker(t_caught_pokemon * msg_caught){
	int socket_cliente = crearSocketCliente (config_gamecard->ip_broker,config_gamecard->puerto_broker);
	if(socket_cliente <= 0){
		log_warning(logGamecard,"No se pudo conectar al Broker para enviar CAUGHT_POKEMON.");
		// close(socket_cliente);
		return socket_cliente;
	}else{
		int enviado = enviarCaughtPokemon(socket_cliente,*msg_caught,P_GAMECARD,ID_PROCESO);
		if(enviado > 0){
			log_info(logGamecard,"Se envió el Mensaje CAUGHT_POKEMON %u [%u].",msg_caught->atrapo_pokemon,msg_caught->id_mensaje_correlativo);
			//Reutilizo esta funcion para el id_mensaje
			uint32_t id_mensaje = recibirIDProceso(socket_cliente);
			if(id_mensaje>0){
				log_info(logInterno,"CAUGHT_POKEMON con correlativo [%u] recibió ID_MENSAJE %u",msg_caught->id_mensaje_correlativo,id_mensaje);
			}else{
				log_info(logInterno,"Error al recibir el ID para CAUGHT_POKEMON con correlativo [%u]",msg_caught->id_mensaje_correlativo);
			}
		}else{
			log_warning(logGamecard,"No se pudo enviar el Mensaje CAUGHT_POKEMON.");
		}
		close(socket_cliente);
		free(msg_caught);
		return enviado;
	}
}
void atender_newPokemon(t_new_pokemon* new_pokemon){
	log_info(logGamecard, "Recepción del Mensaje NEW_POKEMON %s %u %u %u [%u].",new_pokemon->nombre_pokemon,new_pokemon->pos_x,new_pokemon->pos_y,new_pokemon->cantidad,new_pokemon->id_mensaje);

	char* pathMetadata = string_from_format("%s/Files/%s/Metadata.bin", config_gamecard->punto_montaje, new_pokemon->nombre_pokemon);
	
	sem_wait(&mx_creacion_archivo);
	if(existe_archivo(pathMetadata)){
		sem_post(&mx_creacion_archivo);
		//Si existe la metadata del archivo, opero sobre los bloques y la metadata ya existente
		
		//Marcar el archivo abierto en la metadata (ver después el tema de la sincronización)
		//Traer los bloques a memoria y concatenarlos - concatenar_bloques ();
		//Operar los datos y generar un nuevo buffer con los datos actualizados
		//Ir escribiendo otra vez bloque a bloque: write(buffer,size_max_bloque)
		//Actualizar la metadata: si se necesitó grabar un bloque nuevo, agregarlo a la lista de bloques de la metadata asociada
		//Marcar el archivo como cerrado (ver después el tema de la sincronización)
		//Enviar mensaje a la cola APPEARED_POKEMON (ver después)

		//Se puede pasarle una estrucura t_metadata en vez de la ruta al metadata. Esto está asociado al comentario de la línea 212
		
		
		sem_wait(&mx_file_metadata);
		t_config *data_config = config_create (pathMetadata);
		bool archivoAbierto = strcmp(config_get_string_value(data_config,"OPEN"),"Y") == 0;

		//Si el archivo está abierto, espero y reintento luego del delay
		while(archivoAbierto == true){
			sem_post(&mx_file_metadata);
			log_info(logGamecard,"[NEW_POKEMON] Consulto /%s -> ESTÁ ABIERTO -> Se aguarda reintento de conexión.",new_pokemon->nombre_pokemon);
			config_destroy(data_config);

			sleep(config_gamecard->tiempo_reintento_operacion);

			sem_wait(&mx_file_metadata);
			data_config = config_create(pathMetadata);
			archivoAbierto = strcmp(config_get_string_value(data_config,"OPEN"),"Y") == 0;
			
		}
		log_info(logGamecard,"[NEW_POKEMON] Consulto /%s -> ESTÁ CERRADO -> Se realiza la operación.",new_pokemon->nombre_pokemon);
		//Seteo OPEN=Y en el archivo
		config_set_value(data_config,"OPEN","Y");
		config_save(data_config);
		sem_post(&mx_file_metadata);

		//Realizo las operaciones
		char *buffer = getDatosBloques(data_config);
		char* nuevo_buffer = agregar_pokemon(buffer, new_pokemon);
		char **lista_bloques=config_get_array_value(data_config, "BLOCKS");
		t_block* info_block = actualizar_datos(nuevo_buffer, lista_bloques);
		actualizar_metadata(data_config,info_block);

		//Retardo simulando IO
		sleep(config_gamecard->tiempo_retardo_operacion);

		//Cierro archivo		
		config_set_value(data_config,"OPEN","N");
		config_save(data_config);

		config_destroy(data_config);
		free(buffer);
		free_split(lista_bloques);
		destroy_t_block(info_block);
		}
	else{ //Si no existe, creo el directorio del pokemon
		crear_directorio_pokemon(new_pokemon->nombre_pokemon);
		
		char* texto = string_from_format("%lu-%lu=%lu\n",new_pokemon->pos_x,new_pokemon->pos_y,new_pokemon->cantidad);
		t_block* info_block = actualizar_datos(texto, NULL);

		crear_metadata(new_pokemon->nombre_pokemon,info_block);
		sem_post(&mx_creacion_archivo);
		sleep(config_gamecard->tiempo_retardo_operacion);
		destroy_t_block(info_block);
	}
	//Ya sea que agregué o creé el pokemon, respondo el appeared
	//NOTA: Contemplar el caso en que no se pueda agregar el pokemon al FS, en ese caso no devolver appeared
	
	//ENVIAR SIEMPRE AL BROKER, para asegurarme abro una conexión nueva
	enviarAppearedAlBroker(new_pokemon);
	free(pathMetadata);
	free(new_pokemon->nombre_pokemon);	
	free(new_pokemon);
	
}
void atender_getPokemon(t_get_pokemon* get_pokemon){
	/**
	 * 	1) Verificar si el Pokémon existe dentro de nuestro Filesystem. 
	 * 		Para esto se deberá buscar dentro del directorio Pokemon, si existe el archivo con el nombre de 
	 * 		nuestro pokémon. En caso de no existir se deberá informar el mensaje sin posiciones ni cantidades.
		2) Verificar si se puede abrir el archivo (si no hay otro proceso que lo esté abriendo). En caso que 
			el archivo se encuentre abierto se deberá reintentar la operación luego de un tiempo definido por 
			configuración.
		3) Obtener todas las posiciones y cantidades de Pokemon requerido.
		4) Esperar la cantidad de segundos definidos por archivo de configuración
		5) Cerrar el archivo.
		6) Conectarse al Broker y enviar el mensaje con todas las posiciones y su cantidad.
	 */
	// 1)
	char* pathMetadata = string_from_format("%s/Files/%s/Metadata.bin", config_gamecard->punto_montaje, get_pokemon->nombre_pokemon);
	char* posiciones = "";
	uint32_t cant_posiciones = 0;
	if(existe_archivo(pathMetadata)){
		sem_wait(&mx_file_metadata);
		t_config *data_config = config_create (pathMetadata);
		bool archivoAbierto = strcmp(config_get_string_value(data_config,"OPEN"),"Y") == 0;

		// 2) Si el archivo está abierto, espero y reintento luego del delay
		while(archivoAbierto == true){
			log_info(logGamecard,"[GET_POKEMON] Consulto /%s -> ESTÁ ABIERTO -> Se aguarda reintento de conexión.",get_pokemon->nombre_pokemon);
			sem_post(&mx_file_metadata);
			config_destroy(data_config);

			sleep(config_gamecard->tiempo_reintento_operacion);

			sem_wait(&mx_file_metadata);
			data_config = config_create(pathMetadata);
			archivoAbierto = strcmp(config_get_string_value(data_config,"OPEN"),"Y") == 0;
			
		}
		log_info(logGamecard,"[GET_POKEMON] Consulto /%s -> ESTÁ CERRADO -> Se realiza la operación.",get_pokemon->nombre_pokemon);
		//Seteo OPEN=Y en el archivo
		config_set_value(data_config,"OPEN","Y");
		config_save(data_config);
		sem_post(&mx_file_metadata);
		// 3) Obtener todas las posiciones y cantidades de Pokemon requerido.
		char* buffer = getDatosBloques(data_config);
		posiciones = getPosicionesPokemon(buffer,&cant_posiciones);
		// 4) Esperar la cantidad de segundos definidos por archivo de configuración
		sleep(config_gamecard->tiempo_retardo_operacion);
		// 5) Cerrar el archivo.
		config_set_value(data_config,"OPEN","N");
		sem_wait(&mx_file_metadata);
		config_save(data_config);
		sem_post(&mx_file_metadata);

		config_destroy(data_config);
		free(buffer);
	}else{
		//Error -> enviar localized vacio
	//	log_error(logGamecard,"ERROR GET_POKEMON: El directorio del Pokemon %s NO existe.",get_pokemon->nombre_pokemon);
		sleep(config_gamecard->tiempo_retardo_operacion);

	}
	t_localized_pokemon* msg_localized = malloc(sizeof(t_localized_pokemon));
	msg_localized->nombre_pokemon = get_pokemon->nombre_pokemon;
	msg_localized->cant_pos = cant_posiciones;
	msg_localized->posiciones = posiciones;
	msg_localized->id_mensaje_correlativo = get_pokemon->id_mensaje;
	// printf("antes enviar LOCALIZED_POKEMON %s %d %s [%u]\n",msg_localized->nombre_pokemon,msg_localized->cant_pos,msg_localized->posiciones,msg_localized->id_mensaje_correlativo);
	// 6) Conectarse al Broker y enviar el mensaje con todas las posiciones y su cantidad.
	enviarLocalizedAlBroker(msg_localized);
	free(get_pokemon);
	// free(get_pokemon->nombre_pokemon);
	free(pathMetadata);
}
/**
 * Concatena todo el contenido de los bloques y lo devuelve en formato string
 */
char *getDatosBloques(t_config * data_config){
	char **lista_bloques=config_get_array_value(data_config, "BLOCKS");
	int largo_texto = config_get_int_value(data_config, "SIZE");
	char * buffer = concatenar_bloques(largo_texto, lista_bloques); //Apunta buffer a todos los datos del archivo concatenados
	log_info(logGamecard,"Datos Archivo: %s.",buffer);

	free_split(lista_bloques);
	return buffer;
}
void atender_catchPokemon(t_catch_pokemon* catch_pokemon){
	/*
		1) Verificar si el Pokémon existe dentro de nuestro Filesystem. Para esto se deberá buscar 
		dentro del directorio Pokemon, si existe el archivo con el nombre de nuestro pokémon. En caso 
		de no existir se deberá informar un error.
		2) Verificar si se puede abrir el archivo (si no hay otro proceso que lo esté abriendo). 
		En caso que el archivo se encuentre abierto se deberá reintentar la operación luego de un 
		tiempo definido en el archivo de configuración.
		3) Verificar si las posiciones ya existen dentro del archivo. En caso de no existir se debe 
		informar un error.
		4) En caso que la cantidad del Pokémon sea “1”, se debe eliminar la línea. En caso contrario se 
		debe decrementar la cantidad en uno.
		5) Esperar la cantidad de segundos definidos por archivo de configuración
		6) Cerrar el archivo.
		7) Conectarse al Broker y enviar el mensaje indicando el resultado correcto.

	 */
	char* pathMetadata = string_from_format("%s/Files/%s/Metadata.bin", config_gamecard->punto_montaje, catch_pokemon->nombre_pokemon);
	uint32_t atrapo_pokemon = 0;
	//1)
	if(existe_archivo(pathMetadata)){
		sem_wait(&mx_file_metadata);
		
		// 2) 
		t_config *data_config = config_create (pathMetadata);
		bool archivoAbierto = strcmp(config_get_string_value(data_config,"OPEN"),"Y") == 0;
		while(archivoAbierto == true){
			log_info(logGamecard,"[CATCH_POKEMON] Consulto /%s -> ESTÁ ABIERTO -> Se aguarda reintento de conexión.",catch_pokemon->nombre_pokemon);
			sem_post(&mx_file_metadata);
			config_destroy(data_config);

			sleep(config_gamecard->tiempo_reintento_operacion);

			sem_wait(&mx_file_metadata);
			data_config = config_create(pathMetadata);
			archivoAbierto = strcmp(config_get_string_value(data_config,"OPEN"),"Y") == 0;
		}
		log_info(logGamecard,"[CATCH_POKEMON] Consulto /%s -> ESTÁ CERRADO -> Se realiza la operación.",catch_pokemon->nombre_pokemon);
		//Seteo OPEN=Y en el archivo
		config_set_value(data_config,"OPEN","Y");
		config_save(data_config);
		sem_post(&mx_file_metadata);
		// 3) Verificar si la posición existe
		char* buffer = getDatosBloques(data_config);
		char * pos_string = string_from_format("%u-%u=",catch_pokemon->pos_x,catch_pokemon->pos_y);
		int* indice = malloc(sizeof(int));
		int cantidad_existente = cantidadPokemonesEnPosicion(pos_string,buffer,indice);

		if(cantidad_existente == -1){ //-1 si no existe
			log_error(logGamecard,"ERROR CATCH_POKEMON: El Pokemon %s NO existe en la posición %u - %u.",catch_pokemon->nombre_pokemon,catch_pokemon->pos_x,catch_pokemon->pos_y);
		}else{ //Hay 1 o más pokemones
			//Descontar
			char* nuevo_buffer = descontarPokemonEnLinea(*indice,pos_string,buffer,cantidad_existente-1);
			char **lista_bloques=config_get_array_value(data_config, "BLOCKS");
			t_block* info_block = actualizar_datos(nuevo_buffer, lista_bloques);
			actualizar_metadata(data_config,info_block);
			atrapo_pokemon = 1;
			free_split(lista_bloques);
			destroy_t_block(info_block);
		}
		free(pos_string);
		free(indice);
		// 4) Esperar la cantidad de segundos definidos por archivo de configuración
		sleep(config_gamecard->tiempo_retardo_operacion);
		// 5) Cerrar el archivo.
		config_set_value(data_config,"OPEN","N");
		sem_wait(&mx_file_metadata);
		config_save(data_config);
		sem_post(&mx_file_metadata);
		config_destroy(data_config);
		free(buffer);
	}else{
		//Error -> enviar localized vacio
		sleep(config_gamecard->tiempo_retardo_operacion);
		log_error(logGamecard,"ERROR CATCH_POKEMON: El directorio del Pokemon %s NO existe.",catch_pokemon->nombre_pokemon);
		atrapo_pokemon = 0;
	}
	
	t_caught_pokemon* msg_caught = malloc(sizeof(t_caught_pokemon));
	msg_caught->atrapo_pokemon = atrapo_pokemon;
	msg_caught->id_mensaje_correlativo = catch_pokemon->id_mensaje;
	
	enviarCaughtAlBroker(msg_caught);
	free(pathMetadata);
	free(catch_pokemon->nombre_pokemon);
	free(catch_pokemon);
}
/**
 * Recolecta todas las posiciones conocidas del pokemon y arma un string array
 * con el formato de las commons: [1|2,3|32]
 */
char *getPosicionesPokemon(char *buffer, uint32_t* cant_pos){
	char* posiciones_string = string_new();
	char** linea = string_split(buffer,"\n");
	int i = 0;
	string_append(&posiciones_string,"[");
	for (i=0; linea[i] != NULL ; i++) {
		//Cada linea es del formato 10-2=1
		//Split por '=' => ["10-2","1"]
		char** split_aux = string_split(linea[i],"=");
		char* pos_guion = split_aux[0]; //Tomo la primer posicion del char** que devuelve el split
		char** posx_posy = string_split(pos_guion,"-"); //Separo las dos pos. por el guion => ["10","2"]

		char* posicion = string_from_format("%s|%s,",*(posx_posy + 0),*(posx_posy + 1));
		string_append(&posiciones_string,posicion);

		// free(pos_guion);
		free_split(posx_posy);
		free_split(split_aux);
		free(posicion);
	}
	//Elimino la última ","
	//Hago este cambio a variable auxiliar para eliminar memory leak
	char * posiciones_string_fix = string_substring_until(posiciones_string,strlen(posiciones_string)-1);
	free(posiciones_string);
	string_append(&posiciones_string_fix,"]");
	(*cant_pos) = i;
	free_split(linea);
	return posiciones_string_fix;
}
char* escribir_bloque(int block_number, int block_size, char* texto, int* largo_texto){
	char *path_metadata= string_from_format("%s/Blocks/%d.bin",config_gamecard->punto_montaje, block_number);
	FILE *archivo = fopen (path_metadata, "w+");
	
	char* block_texto;
	char* texto_final;
	int bytes_copiados = 0;  
	if((*largo_texto) > block_size){
		bytes_copiados = block_size;
		block_texto = malloc(block_size + 1);
		memcpy(block_texto, texto, block_size);

		//Agrego un \0 porque fputs sino no sabe hasta donde copiar (agrega basura al final)
		*(block_texto + block_size) = '\0';
		}
	else{
		bytes_copiados = *largo_texto;
		block_texto = malloc((*largo_texto) + 1);
		
		memcpy(block_texto, texto, (*largo_texto));
		//Agrego un \0 porque fputs sino no sabe hasta donde copiar (agrega basura al final)
		*(block_texto + (*largo_texto)) = '\0';
	}
	log_info(logGamecard, "Se escribe en el bloque %d -> %s.",block_number, block_texto);

	fputs(block_texto,archivo);
	texto_final = string_substring_from(texto,bytes_copiados);
	free(texto);
	(*largo_texto) = strlen(texto_final);
	// log_info(logInterno, "Path: %s \n Texto: %s \n largo_texto: %d\n",path_metadata,texto_final,(* largo_texto));
	fclose (archivo);
	free(path_metadata);
	free(block_texto);
	return texto_final;
}

void bloques_disponibles(int cantidad,t_list* blocks){
	char* pathBitmap = string_from_format ("%s/Metadata/Bitmap.bin", config_gamecard->punto_montaje);
	int contador = 0;
	//blocks= malloc(sizeof(int)*cantidad);

	char *addr; 
	int fd;
	struct stat file_st;
	char seekchar, newchar;

	seekchar = '0';
	newchar = '1';

	if( -1 == (fd = open(pathBitmap, O_RDWR))) /* open file in read/write mode*/
	{
		perror("Error opened file \n");
	}

	fstat(fd, &file_st); /* Load file status */

	addr = mmap(NULL,file_st.st_size, PROT_WRITE, MAP_SHARED, fd, 0); /* map file  */
	if(addr == MAP_FAILED) /* check mapping successful */
	{
	perror("Error  mapping \n");
	exit(1);
	}
	log_info(logInterno,"cant:%d %c",cantidad,newchar);

	log_info(logInterno,"file contents before: %s", addr); /* write current file contents */

	for(int i = 0; i < file_st.st_size && cantidad >= contador; i++) /* replace characters  */
	{
		int x = i+1;
		if (addr[i] == seekchar) 
		{
			addr[i] = newchar;
			list_add(blocks,&x);
			contador++;
			log_info(logGamecard,"i:%c -s:%c -n:%c - bloque modificado: %d.",addr[i],seekchar,newchar,(int)list_get(blocks,contador));
		}
	}
		
	log_info(logInterno,"file contents after: %s", addr); /* write file contents after modification */
}
 

/*
void modificar_bitmap(char* bitmap, ){
}
*/

char* get_bitmap(){
	char* pathBitmap = string_from_format ("%s/Metadata/Bitmap.bin", config_gamecard->punto_montaje);
	char * addr;
	struct stat file_st;
	int fd;
	if( -1 == (fd = open(pathBitmap, O_RDWR))) /* open file in read/write mode*/
	{
		perror("Error opened file \n");
	}

	fstat(fd, &file_st); /* Load file status */

	addr = mmap(NULL,file_st.st_size, PROT_WRITE, MAP_SHARED, fd, 0); /* map file  */
	if(addr == MAP_FAILED) /* check mapping successful */
	{
		perror("Error  mapping \n");
		exit(1);
	}
	close(fd);
	free(pathBitmap);
	return addr;
}
void unmap_bitmap(char* addr){
	char* pathBitmap = string_from_format ("%s/Metadata/Bitmap.bin", config_gamecard->punto_montaje);
	int fd;
	struct stat file_st;

	if( -1 == (fd = open(pathBitmap, O_RDWR))) /* open file in read/write mode*/
	{
		perror("Error opened file \n");
	}

	fstat(fd, &file_st); /* Load file status */

	munmap(addr,file_st.st_size);
	close(fd);
	free(pathBitmap);
}

void crear_directorio_pokemon(char* pokemon){
	char* pathFiles = string_from_format("%s/Files/%s", config_gamecard->punto_montaje,pokemon);
	int creoDirectorio = mkdir(pathFiles,0700);

	if(creoDirectorio==0){
		log_info(logGamecard, "Se creó el directorio /%s.", pokemon);
		//Crear y llenar bloques
		//Crear metadata en función de los bloques creados
		
	}
	else {
		log_error(logGamecard, "ERROR: No se pudo crear directorio /%s o ya existe.", pokemon); 
		exit (CREATE_DIRECTORY_ERROR);
	}
	free(pathFiles);
}

/**
	Esta función se llama ante un NEW pokemon si este no existe.
	Crea el archivo metadata para el pokemon luego de haber escrito sus bloques.
*/
void crear_metadata (char *pokemon,t_block* info_block) {
	char *path_metadata= string_from_format ("%s/Files/%s/Metadata.bin",config_gamecard->punto_montaje, pokemon);
	char* size = string_from_format("%d",info_block->size);
	FILE *archivo = fopen (path_metadata, "w+");
	fclose (archivo);

	t_config *config_metadata=config_create(path_metadata);
	config_set_value(config_metadata, "DIRECTORY", "N");
	config_set_value(config_metadata, "SIZE",size );
	config_set_value(config_metadata, "BLOCKS", info_block->blocks); 
	config_set_value(config_metadata, "OPEN", "N");
	config_save(config_metadata);
	config_destroy (config_metadata);

	free(path_metadata);
	free(size);
}
/**
	Esta función se llama ante un catch exitoso (descontar o borrar linea) y ante un NEW de un pokemon
	ya existente (aumentar cantidad).
	Establece la metadata de los bloques que se modificaron.
*/
void actualizar_metadata (t_config* config_metadata,t_block* info_block) {
	char* size = string_from_format("%d",info_block->size);

	config_set_value(config_metadata, "SIZE",size );
	config_set_value(config_metadata, "BLOCKS", info_block->blocks); 
	config_save(config_metadata);

	free(size);
}

char* agregar_pokemon(char *buffer, t_new_pokemon* new_pokemon){
	char* posicion = string_from_format("%lu-%lu=",new_pokemon->pos_x,new_pokemon->pos_y);
	log_info(logInterno,"Posición objetivo: %s \n",posicion);

	bool bandera = 0;
	int posicion_remplazo = 0;
	char* nuevo_buffer;
	char ** split = string_split(buffer,"\n");
	char* nueva_linea;
	for (int i=0; *(split+i) != NULL && bandera == 0; i++) {
		//Si la linea empieza con x-y=
		if(string_starts_with(*(split+i),posicion)){
			log_info(logInterno,"Coincidencia con linea %s\n",*(split+i));
			bandera=1;
			nueva_linea= editar_posicion(*(split+i), (int)new_pokemon->cantidad, posicion);
			nuevo_buffer = string_new();

			log_info(logInterno,"La linea modificada queda: %s",nueva_linea);
			posicion_remplazo = i;
		}
	}

	if(bandera==1){
		//Copio las lineas al nuevo buffer, usando la nueva en la posición i
		for (int i=0; *(split+i) != NULL; i++) {
			if(i != posicion_remplazo){
				string_append(&nuevo_buffer,*(split+i));
			}
			else{
				string_append(&nuevo_buffer,nueva_linea);
			}
		}
		// printf("Nuevos datos:\n%s\n",nuevo_buffer);
	}
	else{
		//Si es una linea nueva (la posición no existía)
		nuevo_buffer = string_new(); //me crea un string vacio "\0"
		//copio el buffer original
		string_append(&nuevo_buffer,buffer);
		
		nueva_linea = string_from_format("%s%lu\n",posicion,new_pokemon->cantidad);
		log_info(logInterno,"No hubo coincidencia. Se agrega la linea: %s",nueva_linea);
		string_append(&nuevo_buffer,nueva_linea);
	}
	free_split(split);
	free(nueva_linea);
	free(posicion);
	return nuevo_buffer;
}

char* editar_posicion(char* linea,int cantidad, char* texto_posicion){
	int largo_pos = strlen(texto_posicion);
	
	//linea tiene algo así: 10-2=5
	//texto_posicion algo así: 10-2=
	//Calculo el largo del dato que me interesa (la cantidad de pokemones)
	int length = strlen(linea) - largo_pos;  //largo total - largo posición
	//Substring para recuperar el 5
	char* substring_aux = string_substring(linea, largo_pos, length);
	int cantidad_actual = atoi(substring_aux);
	char* nueva_linea = string_from_format("%s%d\n",texto_posicion,cantidad_actual+cantidad);
	free(substring_aux);
	return nueva_linea;
}
int obtenerCantidadEnLinea(char* linea){
	//Divido "2-2=10" => ["2-2","10"]
	char ** split = string_split(linea,"=");

	int cantidad = atoi(split[1]);
	free_split(split);
	return cantidad;
}
int cantidadPokemonesEnPosicion(char* pos_string,char* buffer,int* indice){
	char** lineas = string_split(buffer,"\n");
	bool encontrado = false;
	int cantidad = -1;
	for(int i = 0; lineas[i] != NULL && encontrado == false; i++){
		if(string_starts_with(lineas[i],pos_string)){
			log_info(logInterno,"Coincidencia con linea %s\n",lineas[i]);
			encontrado = true;
			*indice = i;
			cantidad = obtenerCantidadEnLinea(lineas[i]);
			log_info(logInterno,"La cantidad es: %d\n",cantidad);
		}
	}
	free_split(lineas);
	return cantidad;
}
char* descontarPokemonEnLinea(int indice,char*pos_string,char* buffer,int nueva_cant){
	char** lineas = string_split(buffer,"\n");
	char* nuevo_buffer = string_new();

	for(int i=0; lineas[i] != NULL; i++){
		//Si es la linea que busco, la reemplazo/elimino
		if(i == indice){
			if(nueva_cant>0){
				//Armo nueva linea, concateno la posición "2-2=" con la nueva cantidad "9\n"
				char* nueva_linea = string_new();
				char* cant_string = string_from_format("%u\n",nueva_cant);
				string_append(&nueva_linea,pos_string);
				string_append(&nueva_linea,cant_string);

				string_append(&nuevo_buffer,nueva_linea);
				
				free(cant_string);
				free(nueva_linea);
			}//else no hago nada, se elimina la linea
		}else{
			string_append(&nuevo_buffer,lineas[i]);
			string_append(&nuevo_buffer,"\n");
		}
	}
	free_split(lineas);
	return nuevo_buffer;
}

t_block* actualizar_datos (char* texto,char ** lista_bloques) {
	log_info(logInterno,"Nuevos datos a escribir en los bloques:\n%s\n",texto);
	log_info(logInterno,"Tamaño nuevos datos: %dB, tamaño bloques: %d\n",strlen(texto),FS_config->BLOCK_SIZE);

    int block_size = FS_config->BLOCK_SIZE;
	int total_bloques = FS_config->BLOCKS;
    int largo_texto = strlen(texto);
	int cant_bloques = cantidad_bloques(largo_texto, block_size);
	log_info(logInterno,"cant bloques a ocupar: %d\n",cant_bloques);
	t_block* info_block = malloc(sizeof(t_block));
	char* blocks_text = string_new();
	int contador_avance_bloque = 0;
	int block_number;
	// Guardo el largo del texto para la metadata
	info_block->size = largo_texto;

	string_append(&blocks_text,"[");
	
	
	sem_wait(&mx_w_bloques);
	char *bitmap = get_bitmap();
	log_info(logGamecard,"BITMAP antes de escribir: %s.", bitmap);
	//Para "fusionar" los bloques al escribir una nueva linea, lo más fácil es marcar como libres
	//los bloques de este pokemon, y el 2do for va a escribir todo de nuevo + la nueva linea
	if(lista_bloques != NULL){ //Si se escribe por primera vez, es NULL
		for(int k = 0; lista_bloques[k] != NULL; k++){
			bitmap[atoi(lista_bloques[k])] = '0';
		}
	}
	//NOTA: si se eliminan todas las lineas y el texto queda vacío, no se sobreescriben
	//el bitmap se libera y el contenido de los bloques pasa a ser basura
	//Chequear si esto es perjudicial para los test, se podrían blanquear
	for(int i = 0; i < total_bloques && cant_bloques > contador_avance_bloque; i++)
	{
		if (bitmap[i] == '0') {
			block_number = i;
			texto = escribir_bloque(block_number, block_size, texto, &largo_texto);
			bitmap[i] = '1';
			contador_avance_bloque++;

			if(contador_avance_bloque == cant_bloques){
				char* string_aux =string_from_format("%d",block_number);
				string_append(&blocks_text,string_aux);
				free(string_aux);
			}
			else{
				char* string_aux =string_from_format("%d,",block_number);
				string_append(&blocks_text,string_aux);
				free(string_aux);
			}
		}
		
	}
	sem_post(&mx_w_bloques);
	string_append(&blocks_text,"]");
	log_info(logGamecard,"Bloques escritos: %s.",blocks_text);
	
    info_block->blocks=malloc(strlen(blocks_text)+1);
	strcpy(info_block->blocks,blocks_text);
	log_info(logGamecard,"BITMAP después de escribir: %s.", bitmap);
	free(texto);
	unmap_bitmap(bitmap);
	free(blocks_text);
	return info_block;
}

char* concatenar_lista_char(int largo_texto, char ** lista){
	char* texto_concatenado = malloc(largo_texto);
	for (int i=0; *(lista+i) != NULL; i++) {
		memcpy(*(lista+i),texto_concatenado,strlen(*(lista+i)));
		log_info(logInterno,"texto_concatenado %d: %s",i,texto_concatenado);
	}
	return texto_concatenado;
}

/**==========================================================================================================================
 * Esta función debe llamarse si el pokemon ya existía. Levanta los bloques y devuelve un puntero a esos bloques concatenados
 * ==========================================================================================================================
*/
char* concatenar_bloques(int largo_texto, char ** lista_bloques){ //Acá puede pasársele la estructura t_metadata en vez del path

	char *datos_aux;
	char *datos=malloc (largo_texto +1); //+1 para un \0, sino acarrea basura al final de los datos
	//En este malloc tiene que ir el SIZE que viene del metadata asociado a los bloques
	int offset=0;
	for (int i=0; *(lista_bloques +i) != NULL; i++) {
		
		char *dir_bloque=string_from_format ("%s/Blocks/%s.bin",config_gamecard->punto_montaje,*(lista_bloques+i));
		log_info(logInterno,"path archivo %s\n",dir_bloque);
		
		int archivo=open (dir_bloque, O_RDWR);
		// printf("%c",*(lista_bloques+i));
		if (archivo != -1) {
			struct stat buf;
			fstat (archivo, &buf);
			//mmap devuelve void*
			datos_aux=mmap(NULL, buf.st_size, PROT_WRITE, MAP_SHARED, archivo, 0);
			memcpy(datos+offset,datos_aux,buf.st_size);
			offset= offset + buf.st_size;
			munmap (datos_aux,buf.st_size);
			
		}
		else{
			puts ("No se abrió el bloque");
			exit (OPEN_BLOCK_ERROR);	
		}
		close(archivo);
		free(dir_bloque);
	}
	
	*(datos+offset) = '\0';
	return (datos);
}

int cantidad_bloques(int largo_texto, int block_size){
	float div = (float)largo_texto/(float)block_size;
	double cant = ceil(div);
	return (int)cant;
}
// --------------------------- funciones destroyer -----------------------------------
void destroy_t_block(t_block * block){
    free(block->blocks);
    free(block);
}