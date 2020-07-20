/*
 * gameCard.c
 *
 *  Created on: 28 abr. 2020
 *      Author: aislamientoSOcial
 */

#include "gamecard.h"

void levantarPuertoEscucha(void){
	printf("Puerto escucha\n");
	int socketServidorGamecard = crearSocketServidor(config_gamecard->ip_gamecard, config_gamecard->puerto_gamecard);
	printf("%d",socketServidorGamecard);
	if(socketServidorGamecard == -1){
		log_info(logGamecard, "No se pudo crear el Servidor GameCard");
	}else{
		log_info(logGamecard, "Socket Servidor %d\n", socketServidorGamecard);
		while(1){ //Verificar la condición de salida de este while
					log_info(logInterno,"While de aceptar cliente");
					int cliente = aceptarCliente(socketServidorGamecard);
					pthread_t hiloCliente;
					pthread_create(&hiloCliente, NULL, (void*)atender_cliente, (void*)cliente);

					pthread_detach(hiloCliente);
				}
		close(socketServidorGamecard);
		log_info(logGamecard, "Se cerro el Soket Servidor %d\n", socketServidorGamecard);
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
			log_info (logGamecard, "Fallo de conexión o desconexín broker %s:%s con la cola %s",config_gamecard->ip_broker,config_gamecard->puerto_broker,colaParaLogs(colaSuscripcion));
			sleep (config_gamecard->tiempo_reintento_conexion);
			socket_cliente = crearSocketCliente (config_gamecard->ip_broker,config_gamecard->puerto_broker);
			enviado=enviarSuscripcion (socket_cliente, suscripcion);
		}
	log_info (logGamecard, "Suscripción exitosa con la cola %s", colaParaLogs(colaSuscripcion));
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
				printf("Recibir new");
				op_code cod_op = recibirOperacion(socket_cliente);
				if (cod_op==OP_UNKNOWN)
					socket_cliente = reintentar_conexion((op_code) colaSuscripcion);
				else
				{
					process_code tipo_proceso = recibirTipoProceso(socket_cliente);
					uint32_t PID = recibirIDProceso(socket_cliente);
					printf("Se conectó el proceso %s [%d]\n",tipoProcesoParaLogs(tipo_proceso),PID);
					t_new_pokemon *new_pokemon =recibirNewPokemon (socket_cliente);
					log_info(logGamecard, "Recibi NEW_POKEMON %s %d %d %d [%d]\n",new_pokemon->nombre_pokemon,new_pokemon->pos_x,new_pokemon->pos_y,new_pokemon->cantidad,new_pokemon->id_mensaje);
					int enviado = enviarACK(socket_cliente);
					if(enviado > 0){
						log_info(logGamecard,"Se devolvió el ACK");		
					}else{
						log_info(logGamecard,"ERROR al devolver ACK");
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
				printf("Recibir get");
				op_code cod_op = recibirOperacion(socket_cliente);

				if (cod_op==OP_UNKNOWN)
					socket_cliente = reintentar_conexion((op_code) colaSuscripcion);
				else
				{
					process_code tipo_proceso = recibirTipoProceso(socket_cliente);
					uint32_t PID = recibirIDProceso(socket_cliente);
					printf("Se conectó el proceso %s [%d]\n",tipoProcesoParaLogs(tipo_proceso),PID);
					t_get_pokemon *get_pokemon =recibirGetPokemon (socket_cliente);
					log_info(logGamecard, "Recibi GET_POKEMON %s [%d]\n",get_pokemon->nombre_pokemon,get_pokemon->id_mensaje);
					int enviado = enviarACK(socket_cliente);
					if(enviado > 0){
						log_info(logGamecard,"Se devolvió el ACK");		
					}else{
						log_info(logGamecard,"ERROR al devolver ACK");
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
				printf("Recibir catch");
				op_code cod_op = recibirOperacion(socket_cliente);
				if (cod_op==OP_UNKNOWN)
					socket_cliente = reintentar_conexion((op_code) colaSuscripcion);
				else
				{
					process_code tipo_proceso = recibirTipoProceso(socket_cliente);
					uint32_t PID = recibirIDProceso(socket_cliente);
					printf("Se conectó el proceso %s [%d]\n",tipoProcesoParaLogs(tipo_proceso),PID);
					t_catch_pokemon *catch_pokemon = recibirCatchPokemon(socket_cliente);
					log_info(logGamecard, "Recibi CATCH_POKEMON %s %d %d [%d]\n",catch_pokemon->nombre_pokemon,catch_pokemon->pos_x,catch_pokemon->pos_y,catch_pokemon->id_mensaje);
					int enviado = enviarACK(socket_cliente);
					if(enviado > 0){
						log_info(logGamecard,"Se devolvió el ACK");		
					}else{
						log_info(logGamecard,"ERROR al devolver ACK");
					}
					pthread_t thread;
					pthread_create (&thread, NULL, (void *) atender_catchPokemon, catch_pokemon);
					pthread_detach (thread);
				}
			} break;
		}

		case OP_UNKNOWN:
		{
			log_error (logGamecard, "Operacion inválida");
			break;
		}

		default: log_error (logGamecard, "Operacion no reconocida por el sistema");
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
	logInterno = log_create("gamecardInterno.log", "GamecardInterno", 1, LOG_LEVEL_INFO);
	if(argc != 2){
		log_error(logInterno,"No se ha proporcionado un id. Ej: ./Gamecard 1");
		log_destroy(logInterno);
		exit(-1);
	}
	ID_PROCESO = atoi(argv[1]);
	sem_init(&mx_file_metadata,0,(unsigned int) 1);
	sem_init(&mx_creacion_archivo,0,(unsigned int) 1);
	sem_init(&mx_w_bloques,0,(unsigned int) 1);

	log_info(logInterno,"ID seteado %d",ID_PROCESO);

	logGamecard = log_create("gamecard.log", "Gamecard", 1, LOG_LEVEL_INFO);

	crear_config_gamecard();
	leer_FS_metadata(config_gamecard);
	printf("ip %s:%s\n",config_gamecard->ip_gamecard,config_gamecard->puerto_gamecard);

	//Creo un hilo para el socket de escucha
	pthread_t hiloEscucha;
	pthread_create(&hiloEscucha, NULL, (void*)levantarPuertoEscucha,NULL);

	//Conexión a broker
	subscribe();
	pthread_join(hiloEscucha,NULL);

	sem_destroy(&mx_creacion_archivo);
	sem_destroy(&mx_file_metadata);
	sem_destroy(&mx_w_bloques);

}


void crear_config_gamecard(){

    log_info(logGamecard, "Inicializacion del log.\n");
	config_gamecard = malloc(sizeof(t_configuracion));
	
	config_ruta = config_create(pathGamecardConfig);
	
	if (config_ruta == NULL){
	log_error(logGamecard, "ERROR: No se pudo levantar el archivo de configuración.\n");
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
		log_info (logInterno, "Se leyó correctamente el archivo Metadata del File System");			
	}
	else {
		//config_destroy (FSmetadata);
		log_error (logGamecard, "No se pudo leer el archivo Metadata del File System");
		exit (FILE_FSMETADATA_ERROR);
	}
}


bool existe_archivo(char* path){
	printf("Consulto si existe: %s",path);
	FILE * file = fopen(path, "r");
	if(file!=NULL){
		fclose(file);
		return true;
	} else{
		return false;
	}
}

void atender_cliente(int socket){
	log_info(logGamecard, "Atender cliente %d \n",socket);
	op_code cod_op = recibirOperacion(socket);
	process_code tipo_proceso = recibirTipoProceso(socket);
	uint32_t PID = recibirIDProceso(socket);
	printf("Se conectó el proceso %s [%d]\n",tipoProcesoParaLogs(tipo_proceso),PID);
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
			printf("La operación no es correcta");
			break;
	}
}
int enviarAppearedAlBroker(t_new_pokemon * new_pokemon){
	int socket_cliente = crearSocketCliente (config_gamecard->ip_broker,config_gamecard->puerto_broker);
	if(socket_cliente <= 0){
		// log_info(logGamecard,"No se pudo conectar al broker para enviar APPEARED");
		return socket_cliente;
	}else{
		t_appeared_pokemon* appeared_pokemon = malloc(sizeof(t_appeared_pokemon));
		appeared_pokemon->nombre_pokemon = new_pokemon->nombre_pokemon;
		appeared_pokemon->pos_x = new_pokemon->pos_x;
		appeared_pokemon->pos_y = new_pokemon->pos_y;
		appeared_pokemon->id_mensaje_correlativo = new_pokemon->id_mensaje;

		int app_enviado = enviarAppearedPokemon(socket_cliente, *appeared_pokemon, P_GAMECARD, ID_PROCESO);
		if(app_enviado > 0){
			log_info(logGamecard,"Se devolvió APPEARED_POKEMON %s %d %d [%d]",appeared_pokemon->nombre_pokemon,appeared_pokemon->pos_x,appeared_pokemon->pos_y,appeared_pokemon->id_mensaje_correlativo);
			//Reutilizo esta funcion para el id_mensaje
			uint32_t id_mensaje = recibirIDProceso(socket_cliente);
			if(id_mensaje>0){
				log_info(logGamecard,"APPEARED con correlativo [%d] recibió ID_MENSAJE %d",appeared_pokemon->id_mensaje_correlativo,id_mensaje);
			}else{
				log_info(logGamecard,"Error al recibir el ID para APPEARED con correlativo [%d]",appeared_pokemon->id_mensaje_correlativo);
			}
		}else{
			log_warning(logGamecard,"No se pudo devolver el appeared");
		}
		close(socket_cliente);
		free(appeared_pokemon);
		return app_enviado;
	}
}
int enviarLocalizedAlBroker(t_localized_pokemon * msg_localized){
	int socket_cliente = crearSocketCliente (config_gamecard->ip_broker,config_gamecard->puerto_broker);
	if(socket_cliente <= 0){
		log_info(logGamecard,"No se pudo conectar al broker para enviar el LOCALIZED");
		return socket_cliente;
	}else{
		int enviado = enviarLocalizedPokemon(socket_cliente,*msg_localized,P_GAMECARD,ID_PROCESO);
		if(enviado > 0){
			log_info(logGamecard,"Se devolvió LOCALIZED_POKEMON %s %d %s [%d]",msg_localized->nombre_pokemon,msg_localized->cant_pos,msg_localized->posiciones,msg_localized->id_mensaje_correlativo);
			//Reutilizo esta funcion para el id_mensaje
			uint32_t id_mensaje = recibirIDProceso(socket_cliente);
			if(id_mensaje>0){
				log_info(logGamecard,"LOCALIZED con correlativo [%d] recibió ID_MENSAJE %d",msg_localized->id_mensaje_correlativo,id_mensaje);
			}else{
				log_info(logGamecard,"Error al recibir el ID para LOCALIZED con correlativo [%d]",msg_localized->id_mensaje_correlativo);
			}
		}else{
			log_warning(logGamecard,"No se pudo devolver el LOCALIZED");
		}
		close(socket_cliente);
		free(msg_localized);
		return enviado;
	}
}
void atender_newPokemon(t_new_pokemon* new_pokemon){
	log_info(logGamecard, "Recibi NEW_POKEMON %s %d %d %d [%d]\n",new_pokemon->nombre_pokemon,new_pokemon->pos_x,new_pokemon->pos_y,new_pokemon->cantidad,new_pokemon->id_mensaje);

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
			printf("[NEW] Consulto /%s -> ESTÁ ABIERTO, se aguarda reintento\n",new_pokemon->nombre_pokemon);
			sem_post(&mx_file_metadata);

			sleep(config_gamecard->tiempo_reintento_operacion);

			sem_wait(&mx_file_metadata);
			data_config = config_create(pathMetadata);
			archivoAbierto = strcmp(config_get_string_value(data_config,"OPEN"),"Y") == 0;
			
		}
		printf("[NEW] Consulto /%s -> ESTÁ CERRADO, se realiza la operación\n",new_pokemon->nombre_pokemon);
		//Seteo OPEN=Y en el archivo
		config_set_value(data_config,"OPEN","Y");
		config_save(data_config);
		sem_post(&mx_file_metadata);

		//Realizo las operaciones
		char *buffer = getDatosBloques(data_config);
		char* nuevo_buffer = agregar_pokemon(buffer, new_pokemon);
		char **lista_bloques=config_get_array_value(data_config, "BLOCKS");
		t_block* info_block = actualizar_datos(nuevo_buffer, lista_bloques);
		crear_metadata(new_pokemon->nombre_pokemon,info_block);

		//Retardo simulando IO
		sleep(config_gamecard->tiempo_retardo_operacion);

		//Cierro archivo
		config_set_value(data_config,"SIZE",string_itoa(info_block->size));
		config_set_value(data_config,"BLOCKS",info_block->blocks);
		
		config_set_value(data_config,"OPEN","N");
		config_save(data_config);

		config_destroy(data_config);
		}
	else{ //Si no existe, creo el directorio del pokemon
		crear_directorio_pokemon(new_pokemon->nombre_pokemon);
		
		char* texto = string_from_format("%lu-%lu=%lu\n",new_pokemon->pos_x,new_pokemon->pos_y,new_pokemon->cantidad);
		t_block* info_block = actualizar_datos(texto, NULL);

		crear_metadata(new_pokemon->nombre_pokemon,info_block);
		sem_post(&mx_creacion_archivo);
	}
	//Ya sea que agregué o creé el pokemon, respondo el appeared
	//NOTA: Contemplar el caso en que no se pueda agregar el pokemon al FS, en ese caso no devolver appeared
	
	//ENVIAR SIEMPRE AL BROKER, para asegurarme abro una conexión nueva
	enviarAppearedAlBroker(new_pokemon);	
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
			printf("[GET] Consulto /%s -> ESTÁ ABIERTO, se aguarda reintento\n",get_pokemon->nombre_pokemon);
			sem_post(&mx_file_metadata);

			sleep(config_gamecard->tiempo_reintento_operacion);

			sem_wait(&mx_file_metadata);
			data_config = config_create(pathMetadata);
			archivoAbierto = strcmp(config_get_string_value(data_config,"OPEN"),"Y") == 0;
			
		}
		printf("[GET] Consulto /%s -> ESTÁ CERRADO, se realiza la operación\n",get_pokemon->nombre_pokemon);
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
		config_save(data_config);
		config_destroy(data_config);
	}else{
		//Error -> enviar localized vacio
	}
	t_localized_pokemon* msg_localized = malloc(sizeof(t_localized_pokemon));
	msg_localized->nombre_pokemon = get_pokemon->nombre_pokemon;
	msg_localized->cant_pos = cant_posiciones;
	msg_localized->posiciones = posiciones;
	msg_localized->id_mensaje_correlativo = get_pokemon->id_mensaje;
	// printf("antes enviar LOCALIZED_POKEMON %s %d %s [%d]\n",msg_localized->nombre_pokemon,msg_localized->cant_pos,msg_localized->posiciones,msg_localized->id_mensaje_correlativo);
	// 6) Conectarse al Broker y enviar el mensaje con todas las posiciones y su cantidad.
	enviarLocalizedAlBroker(msg_localized);
}
/**
 * Concatena todo el contenido de los bloques y lo devuelve en formato string
 */
char *getDatosBloques(t_config * data_config){
	char **lista_bloques=config_get_array_value(data_config, "BLOCKS");
	int largo_texto = config_get_int_value(data_config, "SIZE");
	return concatenar_bloques(largo_texto, lista_bloques); //Apunta buffer a todos los datos del archivo concatenados
}
void atender_catchPokemon(t_catch_pokemon* catch_pokemon){
	/*
		Verificar si el Pokémon existe dentro de nuestro Filesystem. Para esto se deberá buscar dentro del directorio Pokemon, si existe el archivo con el nombre de nuestro pokémon. En caso de no existir se deberá informar un error.
		Verificar si se puede abrir el archivo (si no hay otro proceso que lo esté abriendo). En caso que el archivo se encuentre abierto se deberá reintentar la operación luego de un tiempo definido en el archivo de configuración.
		Verificar si las posiciones ya existen dentro del archivo. En caso de no existir se debe informar un error.
		En caso que la cantidad del Pokémon sea “1”, se debe eliminar la línea. En caso contrario se debe decrementar la cantidad en uno.
		Esperar la cantidad de segundos definidos por archivo de configuración
		Cerrar el archivo.
		Conectarse al Broker y enviar el mensaje indicando el resultado correcto.

	 */
	
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
	for (i=0; *(linea+i) != NULL ; i++) {
		//Cada linea es del formato 10-2=1
		//Split por '=' => ["10-2","1"]
		char* pos_guion = *(string_split(*(linea+i),"=")); //Tomo la primer posicion del char** que devuelve el split
		char** posx_posy = string_split(pos_guion,"-"); //Separo las dos pos. por el guion => ["10","2"]

		char* posicion = string_from_format("%s|%s,",*(posx_posy + 0),*(posx_posy + 1));
		string_append(&posiciones_string,posicion);
	}
	//Elimino la última ","
	posiciones_string = string_substring_until(posiciones_string,strlen(posiciones_string)-1);
	string_append(&posiciones_string,"]");
	(*cant_pos) = i;

	return posiciones_string;
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
		block_texto = malloc((*largo_texto));
		
		memcpy(block_texto, texto, (*largo_texto));
		//Agrego un \0 porque fputs sino no sabe hasta donde copiar (agrega basura al final)
		*(block_texto + (*largo_texto)) = '\0';
	}
	log_info(logInterno, "Se escribe en el bloque %d -> %s",block_number, block_texto);

	fputs(block_texto,archivo);
	texto_final = string_substring_from(texto,bytes_copiados);
	(*largo_texto) = strlen(texto_final);
	// log_info(logInterno, "Path: %s \n Texto: %s \n largo_texto: %d\n",path_metadata,texto_final,(* largo_texto));
	fclose (archivo);
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
	printf("cant:%d %c\n",cantidad,newchar);

	printf("\nfile contents before:\n%s \n", addr); /* write current file contents */

	for(int i = 0; i < file_st.st_size && cantidad >= contador; i++) /* replace characters  */
	{
		int x = i+1;
		if (addr[i] == seekchar) 
		{
			addr[i] = newchar;
			list_add(blocks,&x);
			contador++;
			printf("i:%c -s:%c -n:%c -bloque modificado: %d\n",addr[i],seekchar,newchar,(int)list_get(blocks,contador));
		}
	}
		
	printf("\nfile contents after:\n%s \n", addr); /* write file contents after modification */
}
 

/*
void modificar_bitmap(char* bitmap, ){
}
*/

char* get_bitmap(){
	char* pathBitmap = string_from_format ("%s/Metadata/Bitmap.bin", config_gamecard->punto_montaje);
	// puts("get bitmap\n");
	char *addr; 
	int fd;
	struct stat file_st;

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
	
	return addr;
}

void crear_directorio_pokemon(char* pokemon){
	char* pathFiles = string_from_format("%s/Files/%s", config_gamecard->punto_montaje,pokemon);
	int creoDirectorio = mkdir(pathFiles,0700);

	if(creoDirectorio==0){
		log_info(logInterno, "Se creo el directorio /%s", pokemon);
		//Crear y llenar bloques
		//Crear metadata en función de los bloques creados
		
	}
	else {
		log_error(logInterno, "ERROR: No se pudo crear directorio o ya existe /%s", pokemon); 
		exit (CREATE_DIRECTORY_ERROR);
	}
}

/**=================================================================================================================
 * Esta función debería recibir un t_metadata para poder crear/actualizar la metadata luego de modificar los bloques
 * =================================================================================================================
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

}

char* agregar_pokemon(char *buffer, t_new_pokemon* new_pokemon){
	char* posicion = string_from_format("%lu-%lu=",new_pokemon->pos_x,new_pokemon->pos_y);
	printf("Posición objetivo: %s \n",posicion);

	bool bandera = 0;
	int posicion_remplazo = 0;
	char* nuevo_buffer;
	char ** split = string_split(buffer,"\n");
	char* nueva_linea;
	for (int i=0; *(split+i) != NULL && bandera == 0; i++) {
		//Si la linea empieza con x-y=
		if(string_starts_with(*(split+i),posicion)){
			printf("Coincidencia con linea %s\n",*(split+i));
			bandera=1;
			nueva_linea= editar_posicion(*(split+i), (int)new_pokemon->cantidad, posicion);
			nuevo_buffer = string_new();

			printf("La linea modificada queda: %s",nueva_linea);
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
		return nuevo_buffer;
	}
	else{
		//Si es una linea nueva (la posición no existía)
		nuevo_buffer = string_new(); //me crea un string vacio "\0"
		//copio el buffer original
		string_append(&nuevo_buffer,buffer);
		
		nueva_linea = string_from_format("%s%lu\n",posicion,new_pokemon->cantidad);
		printf("No hubo coincidencia. Se agrega la linea: %s",nueva_linea);
		string_append(&nuevo_buffer,nueva_linea);
		return nuevo_buffer;
	}
}

char* editar_posicion(char* linea,int cantidad, char* texto_posicion){
	int largo_pos = strlen(texto_posicion);
	
	//linea tiene algo así: 10-2=5
	//texto_posicion algo así: 10-2=
	//Calculo el largo del dato que me interesa (la cantidad de pokemones)
	int length = strlen(linea) - largo_pos;  //largo total - largo posición
	//Substring para recuperar el 5
	int cantidad_actual = atoi(string_substring(linea, largo_pos, length));
	char* nueva_linea = string_from_format("%s%d\n",texto_posicion,cantidad_actual+cantidad);

	return nueva_linea;
}

t_block* actualizar_datos (char* texto,char ** lista_bloques) {
	printf("Nuevos datos a escribir en los bloques:\n%s\n",texto);
	printf("Tamaño nuevos datos: %dB, tamaño bloques: %d\n",strlen(texto),FS_config->BLOCK_SIZE);

    int block_size = FS_config->BLOCK_SIZE;
	int total_bloques = FS_config->BLOCKS;
    int largo_texto = strlen(texto);
	int cant_bloques = cantidad_bloques(largo_texto, block_size);
	printf("cant bloques a ocupar: %d\n",cant_bloques);
	t_block* info_block = malloc(sizeof(t_block));
	char* blocks_text = string_new();
	int contador_avance_bloque = 0;
	int block_number;
	// Guardo el largo del texto para la metadata
	info_block->size = largo_texto;

	string_append(&blocks_text,"[");
	
	char* bitmap = malloc(total_bloques+1);
	sem_wait(&mx_w_bloques);
	bitmap = get_bitmap();
	printf("\nBITMAP antes de escribir:\n%s \n", bitmap);
	//Para "fusionar" los bloques al escribir una nueva linea, lo más fácil es marcar como libres
	//los bloques de este pokemon, y el 2do for va a escribir todo de nuevo + la nueva linea
	if(lista_bloques != NULL){ //Si se escribe por primera vez, es NULL
		for(int k = 0; lista_bloques[k] != NULL; k++){
			bitmap[atoi(lista_bloques[k])] = '0';
		}
	}
	for(int i = 0; i < total_bloques && cant_bloques > contador_avance_bloque; i++)
	{
		if (bitmap[i] == '0') {
			block_number = i;
			texto = escribir_bloque(block_number, block_size, texto, &largo_texto);
			bitmap[i] = '1';
			contador_avance_bloque++;

			if(contador_avance_bloque == cant_bloques)
				string_append(&blocks_text,string_from_format("%d",block_number));
			else
				string_append(&blocks_text,string_from_format("%d,",block_number));
		}
		
	}
	sem_post(&mx_w_bloques);
	string_append(&blocks_text,"]");
	printf("Bloques escritos: %s\n",blocks_text);
	
    info_block->blocks=malloc(strlen(blocks_text)+1);
	strcpy(info_block->blocks,blocks_text);
	printf("\nBITMAP después de escribir:\n%s \n", bitmap);
	return info_block;
}

char* concatenar_lista_char(int largo_texto, char ** lista){
	char* texto_concatenado = malloc(largo_texto);
	for (int i=0; *(lista+i) != NULL; i++) {
		memcpy(*(lista+i),texto_concatenado,strlen(*(lista+i)));
		printf("texto_concatenado %d: %s",i,texto_concatenado);
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
			
			*(datos+offset) = '\0';
		}
		else{
			puts ("No se abrió el bloque");
			exit (OPEN_BLOCK_ERROR);	
		}
	}
	printf ("Datos archivo:\n%s\n",datos);
	return (datos);
}

int cantidad_bloques(int largo_texto, int block_size){
	float div = (float)largo_texto/(float)block_size;
	double cant = ceil(div);
	return (int)cant;
}
