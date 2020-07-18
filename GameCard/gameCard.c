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
					pthread_create(&hiloCliente, NULL, (void*)atender_cliente, &cliente);

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
void* listen_routine_colas (void *colaSuscripcion){

	int socket_cliente;
	socket_cliente =reintentar_conexion((op_code)colaSuscripcion);

	switch ((op_code)colaSuscripcion)
	{
		case NEW_POKEMON:
		{
			while(1)
			{
				op_code cod_op = recibirOperacion(socket_cliente);
				if (cod_op==OP_UNKNOWN)
					socket_cliente = reintentar_conexion((op_code) colaSuscripcion);
				else
				{
					process_code tipo_proceso = recibirTipoProceso(socket_cliente);
					uint32_t PID = recibirIDProceso(socket_cliente);

					pthread_t thread;
					pthread_create (&thread, NULL, (void *) atender_newPokemon, &socket_cliente);
					pthread_detach (thread);
				}

			} break;
		}

		case GET_POKEMON:
		{
			while(1)
			{
				op_code cod_op = recibirOperacion(socket_cliente);

				if (cod_op==OP_UNKNOWN)
					socket_cliente = reintentar_conexion((op_code) colaSuscripcion);
				else
				{
					process_code tipo_proceso = recibirTipoProceso(socket_cliente);
					uint32_t PID = recibirIDProceso(socket_cliente);
					
					pthread_t thread;
					pthread_create (&thread, NULL, (void *) atender_getPokemon, &socket_cliente);
					pthread_detach (thread);
				}
			} break;
		}

		case CATCH_POKEMON:
		{
			while(1)
			{
				op_code cod_op = recibirOperacion(socket_cliente);
				if (cod_op==OP_UNKNOWN)
					socket_cliente = reintentar_conexion((op_code) colaSuscripcion);
				else
				{
					process_code tipo_proceso = recibirTipoProceso(socket_cliente);
					uint32_t PID = recibirIDProceso(socket_cliente);

					pthread_t thread;
					pthread_create (&thread, NULL, (void *) atender_catchPokemon, &socket_cliente);
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
	pthread_create (&thread1, NULL, listen_routine_colas , (int*)NEW_POKEMON);
	pthread_detach (thread1);

	pthread_t thread2; //OJO. Esta variable se está perdiendo
	pthread_create (&thread2, NULL, listen_routine_colas , (int*)GET_POKEMON);
	pthread_detach (thread2);

	pthread_t thread3; //OJO. Esta variable se está perdiendo
	pthread_create (&thread3, NULL, listen_routine_colas , (int*)CATCH_POKEMON);
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
	FILE * file = fopen(path, "r");
	if(file!=NULL){
		fclose(file);
		return true;
	} else{
		return false;
	}
}

void atender_cliente(int* socket){
	int socket_cliente = *socket;
	log_info(logGamecard, "Atender cliente %d \n",socket_cliente);
	op_code cod_op = recibirOperacion(socket_cliente);
	process_code tipo_proceso = recibirTipoProceso(socket_cliente);
	uint32_t PID = recibirIDProceso(socket_cliente);
	switch(cod_op){
		case NEW_POKEMON:
			atender_newPokemon(&socket_cliente);
			break;
		case GET_POKEMON:
			atender_getPokemon(&socket_cliente);
			break;
		case CATCH_POKEMON:
			atender_catchPokemon(&socket_cliente);
			break;
		default:
			printf("La operación no es correcta");
			break;
	}
}
int enviarAppearedAlBroker(t_new_pokemon * new_pokemon){
	int socket_cliente = crearSocketCliente (config_gamecard->ip_broker,config_gamecard->puerto_broker);
	if(socket_cliente <= 0){
		log_warning(logGamecard,"No se pudo conectar al broker para enviar APPEARED");
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
		}else{
			log_warning(logGamecard,"No se pudo devolver el appeared");
		}
		close(socket_cliente);
		free(appeared_pokemon);
		return app_enviado;
	}
}
void atender_newPokemon(int *socket){
	t_new_pokemon* new_pokemon = recibirNewPokemon(*socket);
	log_info(logGamecard, "Recibi NEW_POKEMON %s %d %d %d [%d]\n",new_pokemon->nombre_pokemon,new_pokemon->pos_x,new_pokemon->pos_y,new_pokemon->cantidad,new_pokemon->id_mensaje);
	int enviado = enviarACK(*socket);
	if(enviado > 0){
		log_info(logGamecard,"Se devolvió el ACK");		
	}else{
		log_info(logGamecard,"ERROR al devolver ACK");
	}
	char* pathMetadata = string_from_format("%s/Files/%s/Metadata.bin", config_gamecard->punto_montaje, new_pokemon->nombre_pokemon);
	printf ("pathmetadata:%s\n", pathMetadata);
	
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
		printf("open %d\n",archivoAbierto);

		//Si el archivo está abierto, espero y reintento luego del delay
		while(archivoAbierto == true){
			sem_post(&mx_file_metadata);

			sleep(config_gamecard->tiempo_reintento_operacion);

			sem_wait(&mx_file_metadata);
			data_config = config_create(pathMetadata);
			archivoAbierto = strcmp(config_get_string_value(data_config,"OPEN"),"Y") == 0;
			printf("open %d\n",archivoAbierto);
		}
		//Seteo OPEN=Y en el archivo
		config_set_value(data_config,"OPEN","Y");
		config_save(data_config);
		sem_post(&mx_file_metadata);
		//Retardo simulando IO
		sleep(config_gamecard->tiempo_retardo_operacion);

		//Realizo las operaciones
		char **lista_bloques=config_get_array_value(data_config, "BLOCKS");
		int largo_texto = config_get_int_value(data_config, "SIZE");
		char *buffer=concatenar_bloques(largo_texto, lista_bloques); //Apunta buffer a todos los datos del archivo concatenados

		char* nuevo_buffer = agregar_pokemon(buffer, new_pokemon);

		t_block* info_block = actualizar_datos(nuevo_buffer, lista_bloques);
		crear_metadata(new_pokemon->nombre_pokemon,info_block);
		//Cierro archivo
		config_set_value(data_config,"OPEN","N");
		config_save(data_config);

		config_destroy(data_config);

		}
	else{ //Si no existe, creo el directorio del pokemon
		crear_directorio_pokemon(new_pokemon->nombre_pokemon);
		
		t_block* info_block = crear_blocks(new_pokemon);
		crear_metadata(new_pokemon->nombre_pokemon,info_block);
		sem_post(&mx_creacion_archivo);
	}
	//Ya sea que agregué o creé el pokemon, respondo el appeared
	//NOTA: Contemplar el caso en que no se pueda agregar el pokemon al FS, en ese caso no devolver appeared
	
	//ENVIAR SIEMPRE AL BROKER, para asegurarme abro una conexión
	enviarAppearedAlBroker(new_pokemon);
	
}

t_block* crear_blocks(t_new_pokemon* new_pokemon){
    int block_size = FS_config->BLOCK_SIZE;
	int total_bloques = FS_config->BLOCKS;
    int largo_texto;
	int cant_bloques;
	t_block* info_block = malloc(sizeof(t_block));
	char* blocks_text;
	int contador_avance_bloque = 0;
	int block_number;
    
    printf("escribir: ");
    char* texto = string_from_format("%lu-%lu=%lu\n",new_pokemon->pos_x,new_pokemon->pos_y,new_pokemon->cantidad);
	largo_texto = strlen(texto);
	log_info(logInterno, "Se escribe %s -> Tamaño: %d", texto, largo_texto);
	// Guardo el largo del texto para la metadata
	info_block->size = largo_texto;

	cant_bloques = cantidad_bloques(largo_texto, block_size);
	// int largo = sizeof(int) * cant_bloques; //???
	// printf("largo string: %d",largo);
	// blocks_text = malloc(largo*(sizeof(int)+1)+1);
	blocks_text = string_new();
	// printf("largo texto bloque: %d\n",largo*(sizeof(int)+1)+1); //???

	string_append(&blocks_text,"[");
	// printf("block text: %s\n",blocks_text);
	// printf("cantidad de bloques %d\n", cant_bloques);
	char* bitmap = malloc(total_bloques+1);
	bitmap = get_bitmap(cant_bloques);
	printf("\nBitmap antes de operar:\n%s \n", bitmap);

	for(int i = 0; i < total_bloques && cant_bloques > contador_avance_bloque; i++) /* replace characters  */
	{
		printf("i: %d, contador_avance_bloque: %d\n",i,contador_avance_bloque);
		if (bitmap[i] == '0') {
			block_number = i;
			printf("bitmap[%d]:%c -bloque victima: %d\n",i,bitmap[i],block_number);
			texto = escribir_bloque(block_number, block_size, texto, &largo_texto);

			bitmap[i] = '1';
			contador_avance_bloque++;

			if(contador_avance_bloque == cant_bloques)
				string_append(&blocks_text,string_from_format("%d",block_number));
			else
				string_append(&blocks_text,string_from_format("%d,",block_number));
		}
		
	}
	string_append(&blocks_text,"]");
	printf("bloques usados %s\n",blocks_text);
	printf("\nBitmap despues de operar:\n%s \n", bitmap);


    info_block->blocks=malloc(strlen(blocks_text)+1);
	strcpy(info_block->blocks,blocks_text);
	return info_block;
}

char* escribir_bloque(int block_number, int block_size, char* texto, int* largo_texto){
	char *path_metadata= string_from_format("%s/Blocks/%d.bin",config_gamecard->punto_montaje, block_number);
	FILE *archivo = fopen (path_metadata, "w+");
	printf("texto: %s largo_texto: %d  block_size:%d \n",texto,(*largo_texto),block_size);
	char* block_texto;
	char* texto_final;  
	if((*largo_texto) > block_size){
		puts("se usa largo bloque\n");
		block_texto = malloc(block_size);
		memcpy(block_texto, texto, block_size);
		}
	else{
		block_texto = malloc((*largo_texto));
		
		memcpy(block_texto, texto, (*largo_texto));
	}
	log_info(logInterno, "Se escribe en el bloque %d -> %s -> Tamaño: %d",block_number, block_texto, (*largo_texto));

	fputs(block_texto,archivo);
	texto_final = string_substring_from(texto,strlen(block_texto));
	(*largo_texto) = strlen(texto_final);
	log_info(logInterno, "Path: %s \n Texto: %s \n largo_texto: %d\n",path_metadata,texto_final,(* largo_texto));
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

void atender_getPokemon(int *socket){
	t_get_pokemon* get_pokemon= recibirGetPokemon(socket);
	int enviado = enviarACK(*socket);
	if(enviado > 0){
		log_info(logGamecard,"Se devolvió el ACK");		
	}else{
		log_info(logGamecard,"ERROR al devolver ACK");
	}
	log_info(logGamecard, "Recibi GET_POKEMON %s [%d]\n",get_pokemon->nombre_pokemon,get_pokemon->id_mensaje);
}

void atender_catchPokemon(int *socket){
	t_catch_pokemon* catch_pokemon= recibirCatchPokemon(socket);
	int enviado = enviarACK(*socket);
	if(enviado > 0){
		log_info(logGamecard,"Se devolvió el ACK");		
	}else{
		log_info(logGamecard,"ERROR al devolver ACK");
	}
	log_info(logGamecard, "Recibi CATCH_POKEMON %s %d %d [%d]\n",catch_pokemon->nombre_pokemon,catch_pokemon->pos_x,catch_pokemon->pos_y,catch_pokemon->id_mensaje);
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
	printf("Posición= %s \n",posicion);
	bool bandera = 0;
	int largo_buffer = strlen(buffer);
	int posicion_remplazo = 0;
	char * dato_remplazo;
	char* texto_concatenado;
	int largo_dato_remplazo = 0;
	char ** split = string_split(buffer,"\n");
	printf("split : %s\n",*(split));
	
	for (int i=0; *(split+i) != NULL && bandera == 0; i++) {
		if(string_contains(*(split+i),posicion)){
			int largo_anterior;
			largo_anterior = strlen(*(split+i));
			printf("existe la posicion %s\n",*(split+i));
			bandera=1;
			char* nuevo_dato= editar_posicion(*(split+i), (int)new_pokemon->cantidad, posicion);
			texto_concatenado = malloc(largo_buffer + strlen(nuevo_dato)-largo_anterior);
			printf("largo buffer anterior: %d nuevo: %d \n",strlen(buffer),largo_buffer + (largo_dato_remplazo - largo_anterior));
			largo_dato_remplazo = strlen(nuevo_dato);

			texto_concatenado = malloc(largo_buffer);
			texto_concatenado = string_new();

			dato_remplazo = malloc(strlen(nuevo_dato));
			dato_remplazo = nuevo_dato;
			printf("dato_remplazo %s\n",dato_remplazo);
			posicion_remplazo = i;
		}
	}

	if(bandera==1){
		
		for (int i=0; *(split+i) != NULL; i++) {
			if(i != posicion_remplazo){
				printf("posicion no remplazo\n");
				string_append(&texto_concatenado,*(split+i));
				printf("texto_concatenado %d: %s \n",i,texto_concatenado);
			}
			else{
				string_append(&texto_concatenado,dato_remplazo);
			}
		}
		printf("texto nuevo: %s \n",texto_concatenado);
		return texto_concatenado;
	}
	else{
		texto_concatenado = malloc(largo_buffer+strlen(posicion)+sizeof(uint32_t));
		texto_concatenado = string_new();
		string_append(&texto_concatenado,buffer);
		printf("antes de agregar nueva posicion: %s\n",texto_concatenado);
		string_append(&texto_concatenado,"\n");
		char * nueva_posicion = string_from_format("%s%lu",posicion,new_pokemon->cantidad);
		printf("nueva posicion: %s",nueva_posicion);
		string_append_with_format(&texto_concatenado,nueva_posicion);
		printf("Nuevo texto CON BARRA N: %s",texto_concatenado);
		return texto_concatenado;
	}

}

char* editar_posicion(char* texto,int cantidad, char* posicion_texto){
	int posicion_largo = strlen(posicion_texto);
	if(string_contains(texto,"\n")){
		int length = strlen(texto) - posicion_largo - 2;  
		int cantidad_actual = (int)(string_substring(texto, posicion_largo+1, length));
		printf("caant_actual %d\n", cantidad_actual);
		return string_from_format("%s%d\n",posicion_texto,cantidad_actual+(int)cantidad);
	}
	else{
		int length = strlen(texto) - posicion_largo;
		printf("length_nro %d\n", length);
		char* cant_actual= string_substring(texto, posicion_largo, length);
		printf("cantidad_actual %sX\n", cant_actual);
		int cantidad_actual = atoi(cant_actual);
		printf("cantidad_actual %d\n", cantidad_actual);
		int nueva_cantidad = cantidad_actual+cantidad;
		printf("nueva_cantidad %d\n", nueva_cantidad);
		int nuevo_string = string_from_format("%s%d",posicion_texto,nueva_cantidad);
		printf("nuevo_string %s\n", nuevo_string);
		return nuevo_string;
	}

}

t_block* actualizar_datos (char* texto,char ** lista_bloques) {
	printf("TEXTO NUEVO: %s\n",texto);

    int block_size = FS_config->BLOCK_SIZE;
	int total_bloques = FS_config->BLOCKS;
    int largo_texto = strlen(texto);;
	int largo_texto_actual;
	int cant_bloques = cantidad_bloques(largo_texto, block_size);
	printf("cant bloques: %d",cant_bloques);
	t_block* info_block = malloc(sizeof(t_block));
	char* blocks_text;
	int contador_avance_bloque = 0;
	int block_number;
    

	log_info(logInterno, "Se escribe %s -> Tamaño: %d", texto, largo_texto);
	// Guardo el largo del texto para la metadata
	info_block->size = largo_texto;
	
	printf("blocks_text malloc: %d\n",cant_bloques*(sizeof(int)+1)+1);
	blocks_text = malloc(cant_bloques*(sizeof(int)+1)+1);


	string_append(&blocks_text,"[");
	printf("block text: %s\n",blocks_text);
	printf("cantidad de bloques %d\n", cant_bloques);

	for (int i=0; *(lista_bloques+i) != NULL; i++) {
		block_number = atoi(*(lista_bloques+i));

		texto = escribir_bloque(block_number, block_size, texto, &largo_texto);

		contador_avance_bloque++;

		if(contador_avance_bloque == cant_bloques)
			string_append(&blocks_text,string_from_format("%d",block_number));
		else
			string_append(&blocks_text,string_from_format("%d,",block_number));

	}

	char* bitmap = malloc(total_bloques+1);
	bitmap = get_bitmap(cant_bloques);
	printf("\nfile contents before:\n%s \n", bitmap);

	for(int i = 0; i < total_bloques && cant_bloques > contador_avance_bloque; i++) /* replace characters  */
	{
		if (bitmap[i] == '0') {
			printf("c_bloques: %d / contador: %d\n",cant_bloques,contador_avance_bloque);
			puts("si el bitmap es 0");
			block_number = i;
			printf("valor:%c -bloque modificado: %d\n",bitmap[i],block_number);
			texto = escribir_bloque(block_number, block_size, texto, &largo_texto);

			bitmap[i] = '1';
			contador_avance_bloque++;

			if(contador_avance_bloque == cant_bloques)
				string_append(&blocks_text,string_from_format("%d",block_number));
			else
				string_append(&blocks_text,string_from_format("%d,",block_number));
		}
		
	}

	printf("despues de escribir bloquesp");
	string_append(&blocks_text,"]");
	
    printf("%d", largo_texto);	
    info_block->blocks=malloc(strlen(blocks_text)+1);
	strcpy(info_block->blocks,blocks_text);
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
void* concatenar_bloques(int largo_texto, char ** lista_bloques){ //Acá puede pasársele la estructura t_metadata en vez del path

	char *datos_aux;
	char *datos=malloc (largo_texto); //En este malloc tiene que ir el SIZE que viene del metadata asociado a los bloques
	int tamanioArchivo=0;
	for (int i=0; *(lista_bloques +i) != NULL; i++) {
		
		char *dir_bloque=string_from_format ("%s/Blocks/%s.bin",config_gamecard->punto_montaje,*(lista_bloques+i));
		int archivo=open (dir_bloque, O_RDWR);
		printf("%c",*(lista_bloques+i));
		if (archivo != -1) {
			struct stat buf;
			fstat (archivo, &buf);
			datos_aux=mmap(NULL, buf.st_size, PROT_WRITE, MAP_SHARED, archivo, 0);
			memcpy(datos+tamanioArchivo,datos_aux,buf.st_size);
			tamanioArchivo= tamanioArchivo +buf.st_size;
			munmap (datos_aux,buf.st_size);
			printf ("Bloque: %d  Datos archivo:\n%s\n",i,datos);
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
	printf("largo_texto: %d block_size:%d \n",largo_texto, block_size);
	float div = (float)largo_texto/(float)block_size;
	//printf("floate: %f \n",div);
	double cant = ceil(div);
	return (int)cant;
}
