/*
 * mainTeam.c
 *
 *  Created on: 1 jun. 2020
 *      Author: utnso
 */

#include "../include/team.h"

int main(void)
{
    internalLogTeam= log_create ("internalLogTeam.log", "Team", 1,LOG_LEVEL_INFO);
    logTeam= log_create ("logTeam.log", "Team", 1,LOG_LEVEL_INFO);
    Team* squad = Team_Init (); //Obtiene configuraciones y entrenadores, los localiza y define el objetivo global

    t_list *GET_list = Team_GET_generate(); //Obtiene lista de objetivos globales para enviar mensajes GET
    
    //subscribe (squad->config); //Crea tres hilos para suscribirse a las tres colas de mensajes

    Trainer_handler_create(squad); //Crea hilo de manejo y planificación de entrenadores (trainer_to_catch) y
    							   //crea el hilo de cada entrenador
    
    //enviar_mensajes_get(squad->config, GET_list); //Envía los mensajes GET al Broker y libera la lista

    listen_new_pokemons (squad->config); //Crea hilo para socket de escucha del GameBoy
    //imprimir_lista (GET_list); //Imprime objetivos globales
    



    send_trainer_to_exec(squad, squad->config->planning_algorithm);


    //liberar semaforos

    /* Solo para probar la correcta liberación de las listas*/
    if (LIBERAR ==1)
    liberar_listas(squad);

    return 0;

}
