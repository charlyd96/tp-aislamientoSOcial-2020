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
    int target=-1;
    u_int32_t distance_min= 100000  ; //Arreglar esta hardcodeada trucha
    mapPokemons *actual_pokemon;
    Team *team= this_team;

    void calculate_distance (void *element)
    {
        Trainer *trainer=element;
        count++;
        if ( trainer->actual_status ==NEW ||trainer->actual_status == BLOCKED_NOTHING_TO_DO) //Sólo planifica entrenadores no bloqueados
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
                target=1;
            }
        }
    }

    while (list_is_empty ( team->global_objective ) == 0)
    {
        sem_wait(&poklist_sem); //Para evitar espera activa si no hay pokemones en el mapa
        sem_wait(&poklist_sem2); //Para evitar espera activa si no hay pokemones en el mapa
        actual_pokemon =  list_remove (mapped_pokemons, 0)  ;
        sem_post(&poklist_sem2);
        sem_wait (&trainer_count); // Este semáforo bloquea el proceso de planificación si no hay entrenadores para mandar a ready
        						   // Se le deberá hacer el post una vez que el entrenador reciba el caught.

        list_iterate (team->trainers,calculate_distance);
        if (target ==1) //Si se pudo encontrar un entrenador libre y más cercano que vaya a cazar al pokemon, avanzo y lo mando
        {
			( (Trainer*) list_get (team->trainers, index))->actual_objective.posx = actual_pokemon->posx;
			( (Trainer*) list_get (team->trainers, index))->actual_objective.posy = actual_pokemon->posy;
			( (Trainer*) list_get (team->trainers, index))->actual_objective.name = string_duplicate (actual_pokemon->name);
			( (Trainer*) list_get (team->trainers, index))->actual_status= READY;
			( (Trainer*) list_get (team->trainers, index))->actual_operation= OP_EXECUTING_CATCH;

			send_trainer_to_ready (team, index);

			count=-1;
			index=-1;
			distance_min= 100000  ; //Arreglar esta hardcodeada trucha
			target=-1;
        }
    } //else: se cumplio el objetivo global -> verificar deadlocks

    //***********LIBERAR MEMORIA Y TERMINAR EL HILO **************//
}

/* Productor hacia cola Ready */
void send_trainer_to_ready (Team *this_team, u_int32_t index)

{
    Team *team=this_team;
    printf ("El entrenador planificado tiene indice %d\n",index);
    sem_wait ( &(team->qr_sem2) );
    list_add  (team->ReadyQueue , list_get (team->trainers, index) );
    sem_post ( &(team->qr_sem2) );
    sem_post ( &(team->qr_sem1) );
}

void send_trainer_to_exec (Team* this_team, char* planning_algorithm)
{
    exec_error error;
    while (1)
    {

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
    sem_wait(&(trainer)->trainer_sem);
   // printf ("Operacion del entrenador: %d\n",trainer->actual_operation);
    //printf ("Antes del switch:%d\n",  trainer->actual_status);

    while (1)
    {
		switch (trainer->actual_operation)
		{
			case OP_EXECUTING_CATCH:
			{
				puts ("Yendo a cazar pokemon");
				move_trainer_to_pokemon (train); //Entre paréntesis debería ir "trainer". No sé por qué funciona así
				printf ("Despues de ejecutar:%d\n",  trainer->actual_status);
				puts ("Voy a mandar un CATCH_POKEMON");
				list_add (BlockedQueue,train); //Por el momento esta cola no se utiliza para nada
				catch_internal mensaje_catch;
				mensaje_catch.trainer_sem= trainer->trainer_sem;
				mensaje_catch.actual_objective= trainer->actual_objective;

				int accert = send_catch_and_recv_id (mensaje_catch);

				if (accert == 1 )
				{
					//Eliminar objetivo personal de la lista de entrenadores (salvo que sea un pokemon de otro entrenador, deadlock)
					//Eliminar objetivo global de la lista de objetivos globales
					//Agregar el pokemon al inventario del entrenador
					//Eliminar pokemon del mapa (la lista mapped_pokemons)
					//Enviar al entrenador a estado BLOCKED_NOTHING_TO_DO
					//Verificar si tiene todos los pokemones que requiere
					//Verificar si quedó en deadlock
				} else
					{
					//Eliminar pokemon del mapa (la lista mapped_pokemons)
					//Enviar al entrenador a estado BLOCKED_NOTHING_TO_DO

					}
				break;
			}


				case EXECUTING_DEADLOCK:
				puts ("solucionar deadlock");
				break;

				default: puts ("Operación no válida");
		}
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
