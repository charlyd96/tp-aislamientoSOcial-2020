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

typedef struct {
    t_config* team_config;
    u_int32_t reconnection_time;
    u_int32_t cpu_cycle;
    char *planning_algorithm;
    u_int32_t quantum;
    u_int32_t initial_estimation;
    char* broker_IP;
    char *broker_port;
} Config;

/*Para crear puntero a archivo de configuración*/
t_config* get_config(void);


/*Para cargar las configuraciones globales y específicas de los entrenadores*/
void Team_load_global_config(Config *config);
void Team_load_trainers_config(); //Recibe un tipo Team*

/*Para testear algunas cosas*/
void _imprimir_lista (void *elemento);
void _imprimir_objetivos (void *elemento);
void _imprimir_inventario (void *elemento);
void _imprimir_lista (void *elemento);

/*Para liberar memoria*/
void  free_split (char **string);
void _free_sub_list (void* elemento);


#endif /* INCLUDE_TEAMCONFIG_H_ */
