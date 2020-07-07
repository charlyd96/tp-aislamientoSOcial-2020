/*
 * gamecard.h
 *
 *  Created on: 5 jun. 2020
 *      Author: utnso
 */

#ifndef GAMECARD_H_
#define GAMECARD_H_

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <sys/stat.h>
#include "protocolo.h"
#include "conexion.h"
#include "serializacion.h"
#include <commons/bitarray.h>
#include <commons/config.h>
#include <commons/string.h>
#include <commons/txt.h>
#include <commons/collections/list.h>
#include <pthread.h>
#include <fcntl.h>
#include <sys/mman.h>
#include <math.h>


char* pathGamecardConfig = "gameCard.config";

typedef struct{
	char* directory;
	int size;
	char** blocks;
	char* open;
}t_metadata;

typedef struct{
	int tiempo_reintento_conexion;
	int tiempo_reintento_operacion;
	char* punto_montaje;
	char* ip_gamecard;
	char* puerto_gamecard;
} t_configuracion;

typedef struct {
	int BLOCK_SIZE;
	int BLOCKS;
	char *MAGIC_NUMBER;

} t_FS_config;

typedef enum{
	CREATE_DIRECTORY_ERROR,
	METADATA_ERROR,
	FILE_CONFIG_ERROR,
	FILE_FSMETADATA_ERROR,
	OPEN_BLOCK_ERROR,
} error;

typedef struct{
	int size;
	char* blocks;
}t_block;

t_config *FSmetadata; //No necesita ser global porque se usa sólo en la inicialización
t_FS_config *FS_config;

t_configuracion* config_gamecard;
t_config* config_ruta; //No necesita ser global porque se usa sólo en la inicialización
t_log* logGamecard;
t_log* logInterno;

t_metadata* data_pokemon; //Revisar esto - No debería ser global
t_config* metadata; //Revisar esto - No debería ser global

int numero_bloque=0;

void crear_config_gamecard();
int crear_metadata_pokemon(char* path);

void iniciar_directorio_basico();
void crear_directorio_pokemon(char* nombre_pokemon);
bool existe_archivo(char* path);

void atender_cliente(int* socket_cliente);
void atender_newPokemon(int socket);
void atender_getPokemon(int socket);
void atender_catchPokemon(int socket);
void leer_FS_metadata (t_configuracion *config_gamecard);
void crear_metadata (char *directorio,t_block* info_blocks);
void* concatenar_bloques (char *path_pokemon , t_new_pokemon *mensaje);
t_block* crear_blocks(t_new_pokemon* new_pokemon);
void bloques_disponibles(int cantidad, t_list* bloques);
char* get_bitmap();
char* escribir_bloque(int block_number, int block_size, char* texto, int* largo_texto);
int cantidad_bloques(int largo_texto, int block_size);
#endif /* GAMECARD_H_ */
