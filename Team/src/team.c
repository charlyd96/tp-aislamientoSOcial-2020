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
    inicializar_listas(); 
    inicializar_semaforos();  
    newTrainerToReady=false; //Mejorar la ubicación de esa instrucción
    ciclos_cpu=0;
    context_switch=0;
    config= malloc (sizeof (Config));
    config->team_config = get_config();
    Team_load_global_config();
    Team_load_trainers_config();
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
            int enviado= enviarGetPokemon (socket, mensaje_get, P_TEAM, ID_proceso);
            log_info(internalLogTeam, "Se envió GET %s al broker. Enviado=%d. Socket= %d",mensaje_get.nombre_pokemon,enviado,socket);
            recv (socket,&(mensaje_get.id_mensaje),sizeof(uint32_t),MSG_WAITALL); //Recibir ID
            log_info(internalLogTeam,"\t\t\t\t\t\t\tEl Id devuelto fue: %d. Pertenece al pokemon %s\t\t\t\t\t\t\n", mensaje_get.id_mensaje, mensaje_get.nombre_pokemon);
            informarIDlocalized(mensaje_get.id_mensaje);
            close (socket); 
        }
        else log_info(internalLogTeam, "No se pudo enviar GET %s al broker.\n",mensaje_get.nombre_pokemon);
    }
	list_iterate(GET_list,send_get );
    list_destroy(GET_list);
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
    
    while (1)
    {
        sem_wait ( &qr_sem1 );
        sem_wait ( &qr_sem2 );
        if (win) break;  
        Trainer* trainer= list_remove (ReadyQueue, 0);
        trainer->actual_status= EXEC;
        sem_post ( &qr_sem2 );
        sem_post ( &(trainer->trainer_sem) );
        sem_wait (&using_cpu);
        context_switch++;
    }
    sem_post(&terminar_ejecucion);
}
/* Consumidor de cola Ready (y productor cuando se desalojó por quantum)*/
void RR_exec (void)
{
       while (1) 
       {
            sem_wait ( &qr_sem1 );
            sem_wait ( &qr_sem2 );
            if (win) break;  
            Trainer* trainer= list_remove (ReadyQueue, 0);
            trainer->actual_status= EXEC; //Verificar si esto puede salir de la zona crítica
            sem_post ( &qr_sem2 );

            sem_post ( &(trainer->trainer_sem) );
            sem_wait (&using_cpu);

            if (trainer->ejecucion == PENDING)
            {
            send_trainer_to_ready(trainers, trainer->index,trainer->actual_operation);
            }
            trainer->rafagaEjecutada=0;
            puts ("cambio de contexto");
            context_switch++;
       }
       sem_post(&terminar_ejecucion);
}

void SJFSD_exec (void)
{
    while (1) //Este while debería ser "mientras team no haya ganado"
    {
        sem_wait ( &qr_sem1 );
        sem_wait ( &qr_sem2 );
        if (win) break;                
        if (newTrainerToReady)
        ordenar_lista_ready();

        newTrainerToReady=false;
        Trainer* trainer= list_remove (ReadyQueue, 0);       
        trainer->actual_status= EXEC; //Verificar si esto puede salir de la zona crítica
        sem_post ( &qr_sem2 );

        sem_post ( &(trainer->trainer_sem) );
        sem_wait (&using_cpu);
        trainer->rafagaEstimada = actualizar_estimacion(trainer);
        trainer->rafagaEjecutada=0;
        context_switch++;
    }
    sem_post(&terminar_ejecucion);

}

void SJFCD_exec (void)
{
    while (1) //Este while debería ser "mientras team no haya ganado"
    {
        sem_wait ( &qr_sem1 );
        sem_wait ( &qr_sem2 );
        if (win) break;  
        if (newTrainerToReady)
        ordenar_lista_ready();
        
        newTrainerToReady=false;
        Trainer* trainer= list_remove (ReadyQueue, 0);       
        trainer->actual_status= EXEC; //Verificar si esto puede salir de la zona crítica
        sem_post ( &qr_sem2 );
        
        trainer->rafagaAux=trainer->rafagaEstimada;
        sem_post ( &(trainer->trainer_sem) );
        sem_wait (&using_cpu);
        if (trainer->ejecucion==FINISHED)
        {
        trainer->rafagaEstimada = trainer->rafagaAux;
        trainer->rafagaEstimada = actualizar_estimacion(trainer);
        trainer->rafagaEjecutada=0;
        }
        else if (trainer->ejecucion==PENDING)
        {
        newTrainerToReady=true;
        list_add (ReadyQueue, trainer);
        sem_post (&qr_sem2);
        sem_post (&qr_sem1);
        } 
        context_switch++;
    }
    sem_post(&terminar_ejecucion);
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
    double nuevaRafaga=(config->alpha)*(trainer->rafagaEjecutada) + (1-config->alpha)*(trainer->rafagaEstimada);
    log_info(internalLogTeam,"************************************************************************Nueva ráfaga estimada (%d): %f******************************\n",trainer->index, nuevaRafaga);
    return (nuevaRafaga) ; 
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
        error = pthread_create( &(trainer->thread_id), NULL, (void*)trainer_routine, trainer);
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
   
    log_info(internalLogTeam,"Resultado de la recuperación: %d\n",deadlock_recovery());
    log_info(internalLogTeam,"En lista Deadlock: %d\n", list_size (deadlock_list));
    

    }

    void imprimir_estados (void *trainer)
    {
        log_info(internalLogTeam,"Estado: %d\n",((Trainer*)trainer)->actual_status);
    }

    //list_iterate (trainers,imprimir_estados);

    void imprimir_entrenadores (void *entrenador)
    {
        void imprimir_objetivos (void *objetivo)
        {
        log_info(internalLogTeam,"%s\n",(char*)objetivo);
        }

        void imprimir_bag (void *capturado)
        {
        log_info(internalLogTeam,"%s\n",(char*)capturado);
        }

        log_info(internalLogTeam,"Lista objetivos entrenador %d\n",((Trainer*)entrenador)->index);
        list_iterate (((Trainer*)entrenador)->bag,imprimir_bag);

        log_info(internalLogTeam,"Lista capturados entrenador %d\n",((Trainer*)entrenador)->index);
        list_iterate(((Trainer*)entrenador)->personal_objective,imprimir_objetivos);
    }

    
    //list_iterate (trainers, imprimir_entrenadores);
    return (error);
    

} 

// ============================================================================================================
//    *************** Función que abre un socket de escucha para recibir mensajes del Gameboy *****************
// ============================================================================================================

void listen_new_pokemons (void)
{
    pthread_t thread; //OJO. Esta variable se está perdiendo
    pthread_create (&thread, NULL, (void*)listen_routine_gameboy , NULL);
    pthread_detach (thread);
} 

// ============================================================================================================
//    *************** Función que crea tres hilos para manejar las suscripciones a cada cola ******************
// ============================================================================================================

void subscribe (void)
{
	pthread_t thread1; //OJO. Esta variable se está perdiendo
	pthread_create (&thread1, NULL, (void *)listen_routine_colas , (int*)APPEARED_POKEMON);
	pthread_detach (thread1);

	pthread_t thread2; //OJO. Esta variable se está perdiendo
	pthread_create (&thread2, NULL, (void *)listen_routine_colas , (int*)LOCALIZED_POKEMON);
	pthread_detach (thread2);
  
	pthread_t thread3; //OJO. Esta variable se está perdiendo
	pthread_create (&thread3, NULL, (void *)listen_routine_colas , (int*)CAUGHT_POKEMON);
	pthread_detach (thread3);
}

void inicializar_listas (void)
{
    trainers = list_create();
    ReadyQueue= list_create  (); 
    mapped_pokemons = list_create();            
    mapped_pokemons_aux = list_create();
    deadlock_list = list_create();
    global_objective = list_create();
    aux_global_objective= list_create();
    //new_global_objective=list_create(); //Se crea en teamConfig.c al hacer list_duplicate(global_objective)
    aux_new_global_objective=list_create();
    global_for_free = list_create();
    ID_caught= list_create();
    ID_localized = list_create();
    especies = list_create();
}


void destruir_listas (void)
{
    list_destroy (ReadyQueue);
    list_destroy (mapped_pokemons);           
    list_destroy (mapped_pokemons_aux);
    list_destroy (deadlock_list);
    list_destroy (global_objective); 
    list_destroy (aux_global_objective);
    list_destroy (new_global_objective);
    list_destroy (aux_new_global_objective);
    liberar_lista_global();
    list_destroy (ID_caught);
    list_destroy (ID_localized);
}

void inicializar_semaforos (void)
{
    sem_init (&qr_sem1, 0, 0);              
    sem_init (&qr_sem2, 0, 1);
    sem_init (&poklist_sem, 0, 0);           
    sem_init (&poklist_sem2, 0, 1);
    sem_init (&poklistAux_sem1, 0, 0);
    sem_init (&poklistAux_sem2, 0, 1);
    sem_init (&deadlock_sem1, 0, 0);
    sem_init (&deadlock_sem2, 0, 1); //Verificar si es necesario el esquema productor/consumidor
    sem_init (&resolviendo_deadlock, 0, 0);
    sem_init (&trainer_count, 0, 0);					
    sem_init (&using_cpu, 0,0);
    sem_init (&terminar_ejecucion, 0, 0);
    sem_init (&terminar_localized, 0, 1);
    pthread_mutex_init (&especies_sem, NULL);
    pthread_mutex_init (&aux_global_sem, NULL);


}

void cerar_semaforos (void)
{
    sem_close (&qr_sem1);              
    sem_close (&qr_sem2);
    sem_close (&poklist_sem);           
    sem_close (&poklist_sem2);
    sem_close (&poklistAux_sem1);
    sem_close (&poklistAux_sem2);
    sem_close (&deadlock_sem1);
    sem_close (&deadlock_sem2); //Verificar si es necesario el esquema productor/consumidor
    sem_close (&resolviendo_deadlock);
    sem_close (&trainer_count);					
    sem_close (&using_cpu);
    sem_close (&terminar_ejecucion);
}

void imprimir_metricas (void)
{
    log_debug(logTeam,"**************************************************************");
    log_debug(logTeam,"¡Felicidades! ¡Team ha alcanzado su objetivo satisfactoriamente!");
    log_debug(logTeam,"\t\t¡Veamos los resultados!");
    log_debug(logTeam,"Resultados Globales:");
    log_debug(logTeam,"Ciclos de CPU totales: %d", ciclos_cpu);
    log_debug(logTeam,"Cambios de contexto: %d\n", context_switch-1); //Porque cuando termina de iterar cuenta uno más

    void metricas_entrenador (void *trainer)
    {   
        Trainer *entrenador=trainer;
        log_debug(logTeam,"Entrenador %d: ",entrenador->index);
        log_debug(logTeam,"Ciclos CPU utilizados: %d\n",  entrenador->ciclos_cpu_totales);
    }
    
    log_debug(logTeam,"Resultados de cada Entrenador: ");
    list_iterate(trainers,metricas_entrenador);

    log_debug(logTeam, "Ocurrieron los siguientes deadlocks entre los entrenadores:");
    
    void imprimir_deadlocks (void *entrenador)
    {
        char *involucrados=formar_string_involucrados( (Trainer*) entrenador);
        if (involucrados != NULL)
        log_debug (logTeam, "El entrenador %d estuvo involucrado en un deadlock con: %s", ((Trainer*)entrenador)->index, involucrados );
        free (involucrados);
    }
    list_iterate(trainers,imprimir_deadlocks);
}

char * formar_string_involucrados(Trainer *trainer)
{
    if (list_is_empty(trainer->involucrados))
    {
    list_destroy(trainer->involucrados);
    return (NULL);
    }
    
    char *numString = string_new();
    void crear_string (void *numeros)
    {
    char *num= string_itoa((int)numeros);
    string_append(&numString, num);
    string_append(&numString, "\t");
    free(num);
    }
    list_iterate(trainer->involucrados, crear_string);
    list_destroy(trainer->involucrados);
    return(numString);
}
