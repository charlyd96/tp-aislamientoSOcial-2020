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

t_log *internalLogTeam;
t_log *logTeam;

typedef struct {
    t_config* team_config;
    u_int32_t reconnection_time;
    u_int32_t cpu_cycle;
    char *planning_algorithm;
    u_int32_t quantum;
    u_int32_t initial_estimation;
    char *broker_IP;
    char *broker_port;
    char *team_IP;
    char *team_port;

} Config;

extern t_list *trainers;

Config *config;

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
void liberar_listas (void);

#endif /* INCLUDE_TEAMCONFIG_H_ */
