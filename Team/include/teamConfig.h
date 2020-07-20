/*
 * TeamConfig.h
 *
 *  Created on: 1 jun. 2020
 *      Author: utnso
 */

#ifndef INCLUDE_TEAMCONFIG_H_
#define INCLUDE_TEAMCONFIG_H_

#include <commons/config.h>
#include <stdlib.h>
#include <stdio.h>
#include <semaphore.h>
#include <commons/log.h>
#include <commons/collections/list.h>

extern sem_t trainer_count;

extern t_list *global_objective;

extern t_list *ID_caught;
extern pthread_mutex_t ID_caught_sem;


extern t_list *ID_localized;
extern pthread_mutex_t ID_localized_sem;

extern bool win;

extern sem_t terminar_appeared;

sem_t terminar_caught;

extern int socketGameboy;

extern int socketAppeared;

extern int socketLocalized;

extern int socketCaught;

extern int socketGameboyCliente;

t_list *global_for_free;

t_log *internalLogTeam;
t_log *logTeam;

typedef struct {
    t_config* team_config;
    u_int32_t reconnection_time;
    int retardo_cpu;
    char *planning_algorithm;
    int quantum;
    double alpha;
    u_int32_t initial_estimation;
    char *broker_IP;
    char *broker_port;
    char *team_IP;
    char *team_port;

} Config;
Config *config;

typedef enum
{
    FIFO,
    RR,
    SJFSD,
    SJFCD
} planificacion;

planificacion algoritmo;

extern t_list *trainers;

/*Para crear puntero a archivo de configuración*/
t_config *get_config(void);


/*Para cargar las configuraciones globales y específicas de los entrenadores*/
void Team_load_global_config();
void Team_load_trainers_config(); 

/*Para testear algunas cosas*/
void _imprimir_lista (void *elemento);
void _imprimir_objetivos (void *elemento);
void _imprimir_inventario (void *elemento);
void _imprimir_lista (void *elemento);

/*Para liberar memoria*/
void  free_split (char **string);
void _free_sub_list (void* elemento);
void destruir_listas (void);
void cerrar_conexiones(void);

void liberar_lista_global(void);

void liberar_entrenadores(void);

void remover_objetivos_globales_conseguidos(t_list *global_bag);

void liberar_configuraciones(void);

void imprimir_entrenador(void);

void destruir_log (void);

#endif /* INCLUDE_TEAMCONFIG_H_ */
