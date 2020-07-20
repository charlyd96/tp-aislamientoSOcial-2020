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
#include <semaphore.h>

#include "strings.h"


char* pathGamecardConfig = "/home/utnso/workspace/tp-2020-1c-aislamientoSOcial/GameCard/gameCard.config";
uint32_t ID_PROCESO;
typedef struct{
	char* directory;
	int size;
	char** blocks;
	char* open;
}t_metadata;

typedef struct{
	int tiempo_reintento_conexion;
	int tiempo_reintento_operacion;
	int tiempo_retardo_operacion;
	char* punto_montaje;
	char* ip_gamecard;
	char* puerto_gamecard;
	char* ip_broker;
	char* puerto_broker;
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

void atender_cliente(int socket_cliente);
void atender_newPokemon(t_new_pokemon* new_pokemon);
void atender_getPokemon(t_get_pokemon* get_pokemon);
void atender_catchPokemon(t_catch_pokemon* catch_pokemon);
void leer_FS_metadata (t_configuracion *config_gamecard);
void crear_metadata (char *directorio,t_block* info_blocks);
char* concatenar_bloques(int largo_texto, char ** lista_bloques);
t_block* crear_blocks(t_new_pokemon* new_pokemon);
void bloques_disponibles(int cantidad, t_list* bloques);
char* get_bitmap();
char* escribir_bloque(int block_number, int block_size, char* texto, int* largo_texto);
int cantidad_bloques(int largo_texto, int block_size);
char* agregar_pokemon(char *buffer, t_new_pokemon* new_pokemon);
char* editar_posicion(char* texto,int cantidad, char* posicion_texto);
char* concatenar_lista_char(int largo_texto, char ** lista);
t_block* actualizar_datos (char* texto,char ** lista_bloques);

void levantarPuertoEscucha(void);
int reintentar_conexion(op_code colaSuscripcion);
void listen_routine_colas (void *colaSuscripcion);
void subscribe();
int enviarAppearedAlBroker(t_new_pokemon* new_pokemon);
int enviarLocalizedAlBroker(t_localized_pokemon* msg_localized);
int enviarCaughtAlBroker(t_caught_pokemon * msg_caught);
char *getPosicionesPokemon(char* buffer, uint32_t* cant_pos);
char *getDatosBloques(t_config *data);
//Semaforos
sem_t mx_file_metadata;
sem_t mx_creacion_archivo;
sem_t mx_w_bloques;
#endif /* GAMECARD_H_ */
