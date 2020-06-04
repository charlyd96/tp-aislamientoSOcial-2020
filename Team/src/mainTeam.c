/*
 * mainTeam.c
 *
 *  Created on: 1 jun. 2020
 *      Author: utnso
 */

#include "../include/team.h"
int varPrueba =5;
int main(void)
{
    Team* squad = Team_Init ();

    t_list *GET_list = Team_GET_generate(squad->global_objective);
    imprimir_lista (GET_list);

    listen_new_pokemons(squad); //Crea hilo 2 de escucha y manejo de mensajes nuevos por Gameboy
    Trainer_handler_create(squad); //Crea hilo 3 de manejo y planificación de entrenadores

    send_trainer_to_exec(squad, squad->config->planning_algorithm);

    /* Solo para probar la correcta liberación de las listas*/
    if (LIBERAR ==1)
    liberar_listas(squad);

    return 0;

}
