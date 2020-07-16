/*
 * trainers.c
 *
 *  Created on: 1 jun. 2020
 *      Author: utnso
 */

#include <trainers.h>
#include <commons/string.h>
#include <string.h>

#define DEFAULT_CATCH       1 

// ============================================================================================================
//    ***** Función que recibe un pokemon existente en el mapa y planifica al entrenador más cercano *****
//        ***** Es un subrutina invocada por un hilo creado en la función Trainer_handler_create ****
// ============================================================================================================

void* Trainer_to_plan_ready (void *this_team)
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

    while (list_is_empty (global_objective) == 0)
    {
        sem_wait(&poklist_sem); //Para evitar espera activa si no hay pokemones en el mapa
        sem_wait(&poklist_sem2); //Para evitar espera activa si no hay pokemones en el mapa
        actual_pokemon =  list_remove (mapped_pokemons, 0)  ;
        sem_post(&poklist_sem2);
        sem_wait (&trainer_count); // Este semáforo bloquea el proceso de planificación si no hay entrenadores para mandar a ready
        						   // Se le deberá hacer el post una vez que el entrenador reciba el caught y pueda volver a ser
                                   // planificado
        list_iterate (team->trainers,calculate_distance);
        if (target ==1) //Si se pudo encontrar un entrenador libre y más cercano que vaya a cazar al pokemon, avanzo y lo mando
        {
			( (Trainer*) list_get (team->trainers, index))->actual_objective.posx = actual_pokemon->posx;
			( (Trainer*) list_get (team->trainers, index))->actual_objective.posy = actual_pokemon->posy;
			( (Trainer*) list_get (team->trainers, index))->actual_objective.name = string_duplicate (actual_pokemon->name); //Malloc
			( (Trainer*) list_get (team->trainers, index))->actual_status= READY;                                           //oculto
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
    sem_wait ( &(team->qr_sem2) );
    list_add  (team->ReadyQueue , list_get (team->trainers, index) );
    sem_post ( &(team->qr_sem2) );
    sem_post ( &(team->qr_sem1) );
    log_info (internalLogTeam, "Se planificó a Ready al entrenador %d", index);
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


void* trainer_routine (void *train)
{
    Trainer *trainer=train;
    trainer->actual_status = NEW;
    sem_wait(&(trainer)->trainer_sem);

    while (trainer->actual_status != EXIT)
    {
		switch (trainer->actual_operation)
		{
			case OP_EXECUTING_CATCH:
			{
				move_trainer_to_pokemon (train); //Entre paréntesis debería ir "trainer". No sé por qué funciona así
				//list_add (BlockedQueue,train); //Por el momento esta cola no se utiliza para nada
				int result= send_catch (trainer);
				if (result == 1 )
				{

                    remover_objetivo_global(trainer->actual_objective.name);
                    printf ("Lista global: %d\n",list_size (global_objective));
                    list_add(trainer->bag,trainer->actual_objective.name);
                    if ( list_size (trainer->bag) >= list_size (trainer->personal_objective)  && 
                    !comparar_listas(trainer->bag,trainer->personal_objective) )
                    {
                        log_info (logTeam, "Ha ocurrido un DEADLOCK en el entrenador %d", trainer->index);
                        trainer->actual_status = BLOCKED_DEADLOCK;
                        sem_post(&trainer_count);
                        sem_wait(&trainer->trainer_sem);
                       
                    }

                    if (list_size (trainer->bag) == list_size (trainer->personal_objective) && 
                    comparar_listas(trainer->bag,trainer->personal_objective))
                    {
                        log_info (internalLogTeam, "El entrenador %d ha alcanzado su objetivo satisfactoriamente", trainer->index);
                        trainer->actual_status = EXIT;
                    }

                    if (list_size (trainer->bag) < list_size (trainer->personal_objective) )
                    {
                        trainer->actual_status = BLOCKED_NOTHING_TO_DO;
                        sem_post (&trainer_count);
                        sem_wait(&trainer->trainer_sem);
        
                    }

                
                    //printf ("Comparar listas: %d\n",comparar_listas (trainer->bag,trainer->personal_objective));

               
                    //Eliminar objetivo personal de la lista de entrenadores (salvo que sea un pokemon de otro entrenador, deadlock)
					//Eliminar objetivo global de la lista de objetivos globales ---LISTO
					//Agregar el pokemon al inventario del entrenador ----LISTO
					//free (trainer->actual_objective.name);
                   
                    //Incrementar semáforo contador de entrenadores libres
                    //free (trainer->actual_objective);   //Eliminar pokemon del mapa (dela lista mapped_pokemons ya se sacó al planificarse) 
					//Enviar al entrenador a estado BLOCKED_NOTHING_TO_DO
					//Verificar si tiene todos los pokemones que requiere (recorrer lista de objetivos personales)
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


void remover_objetivo_global(char *name_pokemon)

{
    bool remover (void * element)
    {
        if (strcmp(name_pokemon,(char*)element))
        return (true);
        else return (false);
    }
list_remove_and_destroy_by_condition(global_objective, remover, free);
}

/***************************************************************/
/********************FALTA LIBERAR MEMORIA*********************/
/*************************************************************/
bool comparar_listas (t_list *lista1, t_list *lista2)
{
    if (list_size(lista1) != list_size(lista2))
    return false;

    int flag=0;
    t_list *listaDuplicada = duplicar_lista(lista2);
    void buscar_iguales (void *nombre1)
    {
        void comparar (void *nombre2)
        {                 
          if (flag ==0 && list_is_empty(listaDuplicada)==0)
            {
                bool comparar_strcmp (void *element)
                    {   
                        
                    if ( !strcmp((char*)element, (char*)nombre1) )
                    {
                       flag=1;
                    return true;
                    }
                    else return false;
                    }
        
                list_remove_and_destroy_by_condition (listaDuplicada,comparar_strcmp,free);
                
            }
            
        }

        list_iterate(lista2,comparar);     
        flag=0;

    }

    list_iterate(lista1,buscar_iguales);
    if (list_is_empty(listaDuplicada) )
    {
        puts ("esta vacia");
    return true;
    }
    else return false;
}



/***************************************************************/
/********************FALTA LIBERAR MEMORIA*********************/
/*************************************************************/

t_list* duplicar_lista (t_list *lista)
{
    t_list *listaDuplicada= list_create();
    void duplicar (void *elemento)
    {
     char *string=malloc (sizeof ((char*) elemento));
     strcpy(string, (char*) elemento);   
     list_add(listaDuplicada,string);
    }

list_iterate(lista,duplicar);
return (listaDuplicada);
}




void move_trainer_to_pokemon (Trainer *trainer)
{
    /*Sólo para mejorar la expresividad*/
    u_int32_t *Tx=&(trainer->posx);
    u_int32_t *Ty=&(trainer->posy);
    u_int32_t *Px=&(trainer->actual_objective.posx);
    u_int32_t *Py=&(trainer->actual_objective.posy);
    log_info (internalLogTeam, "El entrenador %d empezó a ejecutar la caza de un pokemon", trainer->index);

    while ( (*Tx != *Px) || (*Ty!=*Py) )
    {

        if ( calculate_distance (*Tx+1, *Ty, *Px, *Py  ) < calculate_distance (*Tx, *Ty, *Px, *Py ) ){
        *Tx=*Tx+1;
        log_info (logTeam , "Se movió un entrenador hacia la derecha. Posición: (%d,%d)", *Tx, *Ty);
        }

        if ( calculate_distance (*Tx, *Ty+1, *Px, *Py  ) < calculate_distance (*Tx, *Ty, *Px, *Py ) ){
        *Ty=*Ty+1;
        log_info (logTeam , "Se movió un entrenador hacia arriba. Posición: (%d,%d)", *Tx, *Ty);
        }

        if ( calculate_distance (*Tx-1, *Ty, *Px, *Py  ) < calculate_distance (*Tx, *Ty, *Px, *Py ) ){
        *Tx=*Tx-1;
        log_info (logTeam , "Se movió un entrenador hacia la izquierda. Posición: (%d,%d)", *Tx, *Ty);
        }

        if ( calculate_distance (*Tx, *Ty-1, *Px, *Py  ) < calculate_distance (*Tx, *Ty, *Px, *Py ) ){
        *Ty=*Ty-1;
        log_info (logTeam , "Se movió un entrenador hacia abajo. Posición: (%d,%d)", *Tx, *Ty);
        }

        usleep (300000);
    }


}

u_int32_t calculate_distance (u_int32_t Tx, u_int32_t Ty, u_int32_t Px, u_int32_t Py )
{
return (abs (Tx - Px) + abs (Ty - Py));
}
