/*
 * gameCard.c
 *
 *  Created on: 28 abr. 2020
 *      Author: aislamientoSOcial
 */

#include "gamecard.h"

int main(void){

	
	logGamecard = log_create("gamecard.log", "Gamecard", 1, LOG_LEVEL_INFO);
	logInterno = log_create("gamecardInterno.log", "GamecardInterno", 1, LOG_LEVEL_INFO);
	crear_config_gamecard();
	leer_FS_metadata(config_gamecard);
	int socketServidorGamecard = crearSocketServidor(config_gamecard->ip_gamecard, config_gamecard->puerto_gamecard);

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
	config_gamecard->punto_montaje = string_duplicate (config_get_string_value(config_ruta, "PUNTO_MONTAJE_TALLGRASS"));
	config_gamecard->ip_gamecard = string_duplicate (config_get_string_value(config_ruta, "IP_GAMECARD"));
	config_gamecard->puerto_gamecard = string_duplicate (config_get_string_value(config_ruta, "PUERTO_GAMECARD"));
	
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
		config_destroy (FSmetadata);
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
	switch(cod_op){
		case NEW_POKEMON:
			atender_newPokemon(socket_cliente);
			break;
		case GET_POKEMON:
			atender_getPokemon(socket_cliente);
			break;
		case CATCH_POKEMON:
			atender_catchPokemon(socket_cliente);
			break;
		default:
			printf("La operación no es correcta");
			break;
	}
}

void atender_newPokemon(int socket){
	t_new_pokemon* new_pokemon = recibirNewPokemon(socket);
	log_info(logGamecard, "Recibi NEW_POKEMON %s %d %d %d [%d]\n",new_pokemon->nombre_pokemon,new_pokemon->pos_x,new_pokemon->pos_y,new_pokemon->cantidad,new_pokemon->id_mensaje);

	char* pathMetadata = string_from_format("%s/Files/%s/Metadata.bin", config_gamecard->punto_montaje, new_pokemon->nombre_pokemon);
	printf ("pathmetadata:%s\n", pathMetadata);
	if(existe_archivo(pathMetadata)){ //Si existe la metadata del archivo, opero sobre los bloques y la metadata ya existente
		
		//Marcar el archivo abierto en la metadata (ver después el tema de la sincronización)
		//Traer los bloques a memoria y concatenarlos - concatenar_bloques ();
		//Operar los datos y generar un nuevo buffer con los datos actualizados
		//Ir escribiendo otra vez bloque a bloque: write(buffer,size_max_bloque)
		//Actualizar la metadata: si se necesitó grabar un bloque nuevo, agregarlo a la lista de bloques de la metadata asociada
		//Marcar el archivo como cerrado (ver después el tema de la sincronización)
		//Enviar mensaje a la cola APPEARED_POKEMON (ver después)

		//Se puede pasarle una estrucura t_metadata en vez de la ruta al metadata. Esto está asociado al comentario de la línea 212
		char *buffer=concatenar_bloques (pathMetadata, new_pokemon); //Apunta buffer a todos los datos del archivo concatenados
		}
	else{ //Si no existe, creo el directorio del pokemon
		crear_directorio_pokemon(new_pokemon->nombre_pokemon);
		crear_metadata(new_pokemon->nombre_pokemon);
		
	}
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
			log_error(logInterno, "ERROR: No se pudo crear directorio /%s", pokemon); 
			exit (CREATE_DIRECTORY_ERROR);
			 }
}

void atender_getPokemon(int socket){
	t_get_pokemon* get_pokemon= recibirGetPokemon(socket);
	log_info(logGamecard, "Recibi GET_POKEMON %s [%d]\n",get_pokemon->nombre_pokemon,get_pokemon->id_mensaje);
}

void atender_catchPokemon(int socket){
	t_catch_pokemon* catch_pokemon= recibirCatchPokemon(socket);
	log_info(logGamecard, "Recibi CATCH_POKEMON %s %d %d [%d]\n",catch_pokemon->nombre_pokemon,catch_pokemon->pos_x,catch_pokemon->pos_y,catch_pokemon->id_mensaje);
}



/**=================================================================================================================
 * Esta función debería recibir un t_metadata para poder crear/actualizar la metadata luego de modificar los bloques
 * =================================================================================================================
*/
void crear_metadata (char *pokemon) {

	char *path_metadata= string_from_format ("%s/Files/%s/Metadata.bin",config_gamecard->punto_montaje, pokemon);
	FILE *archivo = fopen (path_metadata, "w+");
	fclose (archivo);

	t_config *config_metadata=config_create(path_metadata);
	config_set_value(config_metadata, "DIRECTORY", "N");
	config_set_value(config_metadata, "SIZE", "0");
	config_set_value(config_metadata, "BLOCKS", "[]"); 
	config_set_value(config_metadata, "OPEN", "N");
	config_save(config_metadata);
	config_destroy (config_metadata);

}
/*
actualizar_metadata (t_new_pokemon *mensaje,char *path_metadata){

	t_metadata *data = actualizar_bloque(mensaje)

	t_config *config_metadata=config_create (path_metadata);

	config_set_value (config_metadata, "NUEVO_VALOR");

	config_set_value (config_metadata, "NUEVO_VALOR");

	config_set_value (config_metadata, "NUEVO_VALOR");

	config_set_value (config_metadata, "NUEVO_VALOR");
	
}*/

/*
t_metadata* actualizar_bloque (t_new_pokemon *mensaje, char *path_metadata) {

	t_config config_metadata= config_create(path_metadata);

	char **lista_bloques=config_get_array_value(metadata, "BLOCKS");

}*/

/**==========================================================================================================================
 * Esta función debe llamarse si el pokemon ya existía. Levanta los bloques y devuelve un puntero a esos bloques concatenados
 * ==========================================================================================================================
*/
void* concatenar_bloques (char *path_pokemon , t_new_pokemon *mensaje){ //Acá puede pasársele la estructura t_metadata en vez del path

	t_config *data_config = config_create (path_pokemon);

	char **lista_bloques=config_get_array_value(data_config, "BLOCKS");

	char *datos_aux;
	char *datos=malloc (39); //En este malloc tiene que ir el SIZE que viene del metadata asociado a los bloques
	int tamanioArchivo=0;
	for (int i=0; *(lista_bloques +i) != NULL; i++) {

		char *dir_bloque=string_from_format ("%s/Blocks/%s.bin",config_gamecard->punto_montaje,*(lista_bloques+i));
		int archivo=open (dir_bloque, O_RDWR);

		if (archivo != -1) {
			struct stat buf;
			fstat (archivo, &buf);
			datos_aux=mmap(NULL, buf.st_size, PROT_WRITE, MAP_SHARED, archivo, 0);
			memcpy(datos+tamanioArchivo,datos_aux,buf.st_size);
			tamanioArchivo= tamanioArchivo +buf.st_size;
			munmap (datos_aux,buf.st_size);
		}
		else{
			puts ("No se abrió el bloque");
			exit (OPEN_BLOCK_ERROR);	
		}
		 
		
	}
	config_destroy(data_config);
	printf ("Datos archivo:\n%s\n",datos);
	return (datos);
}
