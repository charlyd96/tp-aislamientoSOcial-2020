/*
 * mainTeam.c
 *
 *  Created on: 1 jun. 2020
 *      Author: utnso
 */

#include "../include/team.h"

int main(void)
{
   Team* squad = Team_Init ();

   t_list *GET_list = Team_GET_generate(squad->global_objective); //Obtiene lista de objetivos globales
   imprimir_lista (GET_list); //Imprime objetivos globales
   listen_new_pokemons (squad->config);
   subscribe (squad->config); //Crea hilo 2 de escucha y manejo de mensajes nuevos por Gameboy

   Trainer_handler_create(squad); //Crea hilo 3 de manejo y planificación de entrenadores (Trainer_to_plan_ready) y
    							   //crea el hilo de cada entrenador

   send_trainer_to_exec(squad, squad->config->planning_algorithm);

    /* Solo para probar la correcta liberación de las listas*/
    if (LIBERAR ==1)
    liberar_listas(squad);

    return 0;

}
