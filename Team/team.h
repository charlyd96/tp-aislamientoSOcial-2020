#include "protocolo.h"
#include "conexion.h"
#include "serializacion.h"
#include <commons/config.h>
#include <commons/string.h>
#include <commons/collections/list.h>

typedef struct {
	char* POSICIONES_ENTRENADORES;
	char* POKEMON_ENTRENADORES;
	char* OBJETIVOS_ENTRENADORES;
	int TIEMPO_RECONEXION;
	int RETARDO_CICLO_CPU;
	char* ALGORITMO_PLANIFICACION;
	int QUANTUM;
	int ESTIMACION_INICIAL;
	char* IP_BROKER;
	char* PUERTO_BROKER;
	char* IP_TEAM;
	char* PUERTO_TEAM;
	char* LOG_FILE;
} t_config_team;
typedef struct{
	int id_entrenador;
	char* posicion_inicial;
	t_list* pokemones;
	t_list* objetivos;
}t_entrenador;
