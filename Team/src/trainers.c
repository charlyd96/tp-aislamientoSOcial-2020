/*
 * trainers.c
 *
 *  Created on: 1 jun. 2020
 *      Author: utnso
 */

#include <trainers.h>
#include <commons/string.h>
#include <string.h>


// ============================================================================================================
//    ***** Función que recibe un pokemon existente en el mapa y planifica al entrenador más cercano *****
//        ***** Es un subrutina invocada por un hilo creado en la función Trainer_handler_create ****
// ============================================================================================================

void* Trainer_to_plan_ready (void *this_team)//(mapPokemons pokemon_in_map, t_list* trainer)
{
    int index=0, count=-1;
    u_int32_t distance_min= 100000  ; //Arreglar esta hardcodeada trucha
    mapPokemons *actual_pokemon;
    Team *team= this_team;

    void calculate_distance (void *element)
    {
        Trainer *trainer=element;
        count++;
        if ( trainer->actual_status ==NEW ||trainer->actual_status == BLOCKED_NOTHING_TO_DO)
        {
            u_int32_t Tx= trainer->posx;
            u_int32_t Ty= trainer->posy;
            u_int32_t Px= actual_pokemon->posx;
            u_int32_t Py= actual_pokemon->posy;
            u_int32_t distance = abs (Tx - Px) + abs (Ty - Py);
            if (distance < distance_min)
            {
                distance_min=distance;
                index=count;

            }


        }

    }

    while (list_is_empty ( team->global_objective ) == 0)
    {
        sem_wait( &(team->poklist_sem)); //Para evitar espera activa si no hay pokemones en el mapa
        sem_wait( &(team->poklist_sem2)); //Para evitar espera activa si no hay pokemones en el mapa
        actual_pokemon =  list_remove (team->mapped_pokemons, 0)  ;
        sem_post( &(team->poklist_sem2));

        sem_wait (&trainer_count); //Este semáforo bloquea el proceso de planificación si no hay entrenadores para mandar a ready
		list_iterate (team->trainers,calculate_distance);
		( (Trainer*) list_get (team->trainers, index))->actual_objective.posx = actual_pokemon->posx;
		( (Trainer*) list_get (team->trainers, index))->actual_objective.posy = actual_pokemon->posy;
		( (Trainer*) list_get (team->trainers, index))->actual_objective.name  = string_duplicate (actual_pokemon->name);
		( (Trainer*) list_get (team->trainers, index))->actual_status= READY;
		( (Trainer*) list_get (team->trainers, index))->actual_operation= OP_EXECUTING_CATCH;
		send_trainer_to_ready (team, index);

		count=-1;
		index=-1;
		distance_min= 100000  ; //Arreglar esta hardcodeada trucha

    } //else: se cumplio el objetivo global -> verificar deadlocks

}

/* Productor hacia cola Ready */
void send_trainer_to_ready (Team *this_team, u_int32_t index)

{
    Team *team=this_team;
    printf ("El entrenador planificado tiene indice %d\n",index);
    printf ("Su estado es %d\n" , ((Trainer*) list_get(team->trainers, index))->actual_status ) ;
    sem_wait ( &(team->qr_sem2) );
    list_add  (team->ReadyQueue , list_get (team->trainers, index) );
    sem_post ( &(team->qr_sem2) );
    sem_post ( &(team->qr_sem1) );
}

void send_trainer_to_exec (Team* this_team, char* planning_algorithm)
{
    exec_error error;
    //Trainer *trainer=this_team->trainers;
    while (1)
    {
    //sleep (5);
    /*
    puts ("entrenador planificado:");
    printf ("Atrapará un %s en la posicion x: %d, y: %d\n", ((Trainer*) list_get (this_team->ReadyQueue, 0))->actual_objective.name,
    ((Trainer*) list_get (this_team->ReadyQueue, 0))->actual_objective.posx,
    ((Trainer*) list_get (this_team->ReadyQueue, 0))->actual_objective.posy);
    printf ("element counts: %d \n", this_team->ReadyQueue->elements_count);
    printf ("element counts despuesde dormir: %d \n", this_team->ReadyQueue->elements_count);
    */
    if (!strcmp(planning_algorithm, "FIFO"))
    {
       error= fifo_exec (this_team);

    }
    /*
    *TO DO: sjfcd_exec()
    *TO DO: sjf_exec()
    *TO DO: rr_exec()
    */

    }
}
//--Hay que buscar la forma de mandar al entrenador a la lista de bloqueados, siendo que trainer_routine no recibe la lista.
//--Podemos hacer que las colas sean variables globales o bien pasarle la estructura completa this_team

void* trainer_routine (void *train)
{
    Trainer *trainer=train;
    trainer->actual_status = NEW;
    sem_wait(&(trainer)->t_sem);
    printf ("Operacion del entrenador: %d\n",trainer->actual_operation);
    printf ("Antes del switch:%d\n",  trainer->actual_status);
    switch (trainer->actual_operation)
    {
        case OP_EXECUTING_CATCH:
        puts ("Yendo a cazar pokemon");
        move_trainer_to_pokemon (train); //Entre paréntesis debería ir "trainer". No sé por qué funciona así
        printf ("Despues de ejecutar:%d\n",  trainer->actual_status);
        puts ("Voy a mandar un CATCH_POKEMON");
        list_add (BlockedQueue,train);


        sem_post(&using_cpu);
        sem_wait(&(trainer->t_sem));//Esto no debería estar aca. Si el proceso es interrumpido justo despues de hacer sem_post,
        break;				 	   // podria arrancar a ejecutar un nuevo proceso en la cpu antes de que el actual salga

        case EXECUTING_DEADLOCK:
        puts ("solucionar deadlock");
        break;

        default: puts ("Operación no válida");
    }

}


void move_trainer_to_pokemon (Trainer *trainer)
{
    /*Sólo para mejorar la expresividad*/
    u_int32_t *Tx=&(trainer->posx);
    u_int32_t *Ty=&(trainer->posy);
    u_int32_t *Px=&(trainer->actual_objective.posx);
    u_int32_t *Py=&(trainer->actual_objective.posy);
    printf ("Posición entrenador antes de moverse : (%d,%d)\n", (int)trainer->posx, (int)trainer->posy ) ;


    while ( (*Tx != *Px) || (*Ty!=*Py) )
    {

        if ( calculate_distance (*Tx+1, *Ty, *Px, *Py  ) < calculate_distance (*Tx, *Ty, *Px, *Py ) ){
        *Tx=*Tx+1;
        puts ("me movi hacia la derecha");
        }

        if ( calculate_distance (*Tx, *Ty+1, *Px, *Py  ) < calculate_distance (*Tx, *Ty, *Px, *Py ) ){
        *Ty=*Ty+1;
        puts ("me movi hacia arriba");
        }

        if ( calculate_distance (*Tx-1, *Ty, *Px, *Py  ) < calculate_distance (*Tx, *Ty, *Px, *Py ) ){
        *Tx=*Tx-1;
        puts ("me movi hacia la izquierda");
        }

        if ( calculate_distance (*Tx, *Ty-1, *Px, *Py  ) < calculate_distance (*Tx, *Ty, *Px, *Py ) ){
        *Ty=*Ty-1;
        puts ("me movi hacia abajo");
        }

            printf ("Posición pokemon: (%d,%d)\n", (int)trainer->actual_objective.posx, (int)trainer->actual_objective.posy);
            printf ("Posición entrenador: (%d,%d)\n", (int)trainer->posx, (int)trainer->posy);

        usleep (300000);
    }
    puts   ("Llegué a la ubicación del Pokemon. Vamos a verificarlo:");
    printf ("Posición entrenador: (%d,%d)\n", (int)trainer->posx,(int)trainer->posy);
    printf ("Posición pokemon: (%d,%d)\n", (int)trainer->actual_objective.posx,(int)trainer->actual_objective.posy);

}

u_int32_t calculate_distance (u_int32_t Tx, u_int32_t Ty, u_int32_t Px, u_int32_t Py )
{
return (abs (Tx - Px) + abs (Ty - Py));
}
