/*
 * mainTeam.c
 *
 *  Created on: 1 jun. 2020
 *      Author: utnso
 */

#include "../include/team.h"

int main(int argc, char **argv)

{
      /*void* routine(void *dato)
{
    sleep (13);
    sem_post ( &using_cpu);
    puts ("hice el post");
}
   pthread_t thread;
   pthread_create(&thread,NULL,routine,NULL);
   pthread_detach(thread);*/
   if (argc!=2)
   {
       puts ("No se proporcionó el número de ID de proceso");
       exit(-1);
   }
    ID_proceso = atoi(argv[1]);
    internalLogTeam= log_create ("internalLogTeam.log", "Team", 1,LOG_LEVEL_INFO);
    logTeam= log_create ("logTeam.log", "Team", 1,LOG_LEVEL_INFO);
    Team_Init (); //Obtiene configuraciones y entrenadores, los localiza y define el objetivo global

    t_list *GET_list = Team_GET_generate(); //Obtiene lista de objetivos globales para enviar mensajes GET
    
    subscribe (); //Crea tres hilos para suscribirse a las tres colas de mensajes
    send_trainer_to_exec();

    enviar_mensajes_get(GET_list); //Envía los mensajes GET al Broker y libera la lista

    listen_new_pokemons (); //Crea hilo para socket de escucha del GameBoy
   //imprimir_lista (GET_list); //Imprime objetivos globales

    Trainer_handler_create(); //Crea hilo de manejo y planificación de entrenadores (trainer_to_catch) y
    						  //crea el hilo de cada entrenador

    //liberar semaforos
   // usleep (200); //Para que se imprima el último log
    /* Solo para probar la correcta liberación de las listas*/
    if (LIBERAR ==1)
    win=true;
    //sleep (2);

    //Antes de cerrar conexiones, esperar a que vuelvan todos los hilos
    cerrar_conexiones();
    destruir_log();
    destruir_listas();
    liberar_entrenadores();
    liberar_configuraciones();
    sleep (3);
    return 0;
}

