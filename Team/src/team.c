/*
 * team.c
 *
 *  Created on: 27 abr. 2020
 *      Author: aislamientoSOcial
 */

#include "../include/team.h"

#include <string.h>
#include <conexion.h>

#include <protocolo.h>
#include <serializacion.h>

#include <trainers.h>


/* Initializing all Team components */
void Team_Init(void)
{

    config= malloc (sizeof (Config));
    config->team_config = get_config();
    sem_init (&trainer_count, 0, 0);					//*********Mejorar la ubicación de esta instrucción***************//
    sem_init (&using_cpu, 0,0);
    Team_load_global_config();
    
    Team_load_trainers_config();

    ReadyQueue= list_create  ();                 //*********Mejorar la ubicación de esta instrucción***************//
    sem_init (&qr_sem1, 0, 0);               //*********Mejorar la ubicación de esta instrucción***************//
    sem_init (&qr_sem2, 0, 1);
   
    mapped_pokemons = list_create();             //*********Mejorar la ubicación de esta instrucción***************//
    sem_init (&poklist_sem, 0, 0);           //*********Mejorar la ubicación de esta instrucción***************//
    sem_init (&poklist_sem2, 0, 1);          //*********Mejorar la ubicación de esta instrucción***************//

    mapped_pokemons_aux = list_create();
    sem_init (&poklistAux_sem1, 0, 0);
    sem_init (&poklistAux_sem2, 0, 1);


    /*Lista de entrenadores en deadlock*/
    deadlock_list = list_create();
    sem_init (&deadlock_sem1, 0, 0);
    sem_init (&deadlock_sem2, 0, 1); //Verificar si es necesario el esquema productor/consumidor
    
    sem_init (&resolviendo_deadlock, 0, 0);




}

// ============================================================================================================
//      ***** Función que crea una lista con los pokemones que van a ser enviados como mensaje GET *****
//              *****Toma la lista de objetivos globales y elimina los pokemones repetidos*****
// ============================================================================================================

t_list*  Team_GET_generate ()
{
    t_list* list_get = list_create();

    void get_pokemons (void *element)
    {
        bool compare_pokemons (void *string)
        {
        if (strcmp((char*)string,(char*)element)  == 0)
        return (true);
        else return (false);
        }

    if (list_any_satisfy (list_get,compare_pokemons) == false)
    list_add (list_get, element);
    }

    list_iterate (global_objective, get_pokemons);
    return (list_get);
}

void enviar_mensajes_get (t_list* GET_list)
{
	
    void send_get (void *element)
    {
        t_get_pokemon mensaje_get;
        mensaje_get.nombre_pokemon=(char *)element;
        mensaje_get.id_mensaje=0;
        int socket = crearSocketCliente (config->broker_IP,config->broker_port);
        if (socket != -1)
        {
        int enviado= enviarGetPokemon (socket, mensaje_get);
        log_info(internalLogTeam, "Se envió GET %s al broker. Enviado=%d. Socket= %d",mensaje_get.nombre_pokemon,enviado,socket);
        recv (socket,&(mensaje_get.id_mensaje),sizeof(uint32_t),MSG_WAITALL); //Recibir ID
		printf ("\t\t\t\t\t\t\tEl Id devuelto fue: %d. Pertenece al pokemon %s\t\t\t\t\t\t\n", mensaje_get.id_mensaje, mensaje_get.nombre_pokemon);
        informarIDlocalized(mensaje_get.id_mensaje);
        close (socket); 
        }
        else log_info(internalLogTeam, "No se pudo enviar GET %s al broker.\n",mensaje_get.nombre_pokemon);
    }
	list_iterate(GET_list,send_get );
}



// ============================================================================================================
//                               ***** Función para imprimir listas (sólo para testear) *****
// ============================================================================================================

void imprimir_lista (t_list *lista)
{

    void imprimir (void *element)
    {
        puts ((char*)element);
    }

    list_iterate (lista, imprimir);
}

/* Consumidor de cola Ready */
void fifo_exec (void)
{
    while (1) //Este while debería ser "mientras team no haya ganado"
    {
        
        // if (team ganó) entonces return;
        sem_wait ( &qr_sem1 );
        sem_wait ( &qr_sem2 );

        Trainer* trainer= list_remove (ReadyQueue, 0);
        trainer->actual_status= EXEC;
        sem_post ( &qr_sem2 );
        sem_post ( &(trainer->trainer_sem) );
        sem_wait (&using_cpu);
    }
}
/* Consumidor de cola Ready (y productor cuando se desalojó por quantum)*/
void RR_exec (void)
{
       while (1) //Este while debería ser "mientras team no haya ganado"
       {

            sem_wait ( &qr_sem1 );
            sem_wait ( &qr_sem2 );
            Trainer* trainer= list_remove (ReadyQueue, 0);
            trainer->actual_status= EXEC; //Verificar si esto puede salir de la zona crítica
            sem_post ( &qr_sem2 );

            sem_post ( &(trainer->trainer_sem) );
            sem_wait (&using_cpu);

            if (trainer->ejecucion == PENDING)
            {
            send_trainer_to_ready(trainers, trainer->index,trainer->actual_operation);
            }
            else trainer->ejecucion = FINISHED;

            ciclos_cpu=0;
       }
}

void SJFSD_exec (void)
{
    int sizeReady=0;
    int sizeReadyAnt=0; 
       while (1) //Este while debería ser "mientras team no haya ganado"
       {
            sem_wait ( &qr_sem1 );
            sem_wait ( &qr_sem2 );
            sizeReady=list_size(ReadyQueue);
            if (sizeReady != sizeReadyAnt+1) //Se le suma uno porque el list_remove saca el entrenador que está por ejecutar
            ordenar_lista_ready();
            
            Trainer* trainer= list_remove (ReadyQueue, 0);       
            trainer->actual_status= EXEC; //Verificar si esto puede salir de la zona crítica
            sem_post ( &qr_sem2 );
            sizeReadyAnt=sizeReady;
            sem_post ( &(trainer->trainer_sem) );
            sem_wait (&using_cpu);
            trainer->rafagaEstimada = actualizar_estimacion(trainer);
            trainer->rafagaEjecutada=0;
            printf ("************************************************************************Nueva ráfaga estimada (%d): %f******************************\n",trainer->index, trainer->rafagaEstimada);

        }
}


void ordenar_lista_ready (void)
{
    bool comparador (void *tr1, void *tr2)
    {
    Trainer *trainer1=tr1;
    Trainer *trainer2=tr2;
    return (trainer1->rafagaEstimada<trainer2->rafagaEstimada); 
    }
    list_sort(ReadyQueue,comparador); //No necesito proteger esta zona crítica, porque ya se protegió antes del llamado a la función
}

double  actualizar_estimacion (Trainer *trainer)
{
    return ( (config->alpha)*(trainer->rafagaEjecutada) + (1-config->alpha)*(trainer->rafagaEstimada) ); 
}


// ============================================================================================================
//    ***** Función que  crea un hilo por cada uno de los entrenadores existente en la lista del Team*****
//                  ***** Recibe una estructura Team y devuelve un código de error *****
// ============================================================================================================

int Trainer_handler_create ()
{
    u_int32_t error;
    void  create_thread (void *train)
    { 
        Trainer* trainer= train;      
        error = pthread_create( &(trainer->thread_id), NULL, trainer_routine, trainer);
        pthread_detach (trainer->thread_id);
    }

    list_iterate (trainers, create_thread);
    if (error != 0)
    {
        exit (TRAINER_CREATION_FAILED);
    }

    void *resultado;
    pthread_t thread;
    pthread_create ( &thread, NULL, (void*)trainer_to_catch, resultado); 
    pthread_join (thread, &resultado); //hacerlo join y llamar a un nuevo hilo 

    while (list_size (deadlock_list)>0)
    {
    int value;

    sem_getvalue(&resolviendo_deadlock, &value);
    printf ("Resolviendo deadlock antes del recovery vale %d\n", value);
    printf ("Resultado de la recuperación: %d\n",deadlock_recovery());
    printf ("En lista Deadlock: %d\n", list_size (deadlock_list));
    
    sem_getvalue(&resolviendo_deadlock, &value);
    printf ("Resolviendo deadlock vale %d\n", value);
    sem_wait (&resolviendo_deadlock);
    //sleep (4);
    }

    void imprimir_estados (void *trainer)
    {
        printf ("Estado: %d\n",((Trainer*)trainer)->actual_status);
    }

    list_iterate (trainers,imprimir_estados);

    void imprimir_entrenadores (void *entrenador)
    {
        void imprimir_objetivos (void *objetivo)
        {
        printf ("%s\n",(char*)objetivo);
        }

        void imprimir_bag (void *capturado)
        {
        printf ("%s\n",(char*)capturado);
        }

        printf ("Lista objetivos entrenador %d\n",((Trainer*)entrenador)->index);
        list_iterate (((Trainer*)entrenador)->bag,imprimir_bag);

        printf ("Lista capturados entrenador %d\n",((Trainer*)entrenador)->index);
        list_iterate(((Trainer*)entrenador)->personal_objective,imprimir_objetivos);
    }

    
    list_iterate (trainers, imprimir_entrenadores);
    return (error);
    

}



// ============================================================================================================
//    ***** Función que está a la espera de la llegada de nuevos pokemons y los guarda en una lista *****
//          ***** Recibe una estructura de tipo Team. Añade nodos con data ingresda por consola ****
// ============================================================================================================



void listen_new_pokemons ()
{
    pthread_t thread; //OJO. Esta variable se está perdiendo
    pthread_create (&thread, NULL, (void*)listen_routine_gameboy , NULL);
    pthread_detach (thread);
} 


void subscribe ()
{
	pthread_t thread1; //OJO. Esta variable se está perdiendo
	pthread_create (&thread1, NULL, listen_routine_colas , (int*)APPEARED_POKEMON);
	pthread_detach (thread1);

	pthread_t thread2; //OJO. Esta variable se está perdiendo
	pthread_create (&thread2, NULL, listen_routine_colas , (int*)LOCALIZED_POKEMON);
	pthread_detach (thread2);
  
	pthread_t thread3; //OJO. Esta variable se está perdiendo
	pthread_create (&thread3, NULL, listen_routine_colas , (int*)CAUGHT_POKEMON);
	pthread_detach (thread3);
}
