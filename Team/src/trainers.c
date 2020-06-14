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
            u_int32_t distance = abs (Tx - Px) + abs (Ty - Py); //Reemplazar por función "calcular_distancia"
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
    list_add (team->ReadyQueue , list_get (team->trainers, index) );
    sem_post ( &(team->qr_sem2) );
    sem_post ( &(team->qr_sem1) );
    log_info (internalLogTeam, "Se planificó a Ready al entrenador %d", index);
}

void send_trainer_to_exec (Team* this_team, char* planning_algorithm)
{
    exec_error error;
    while (1) //Este while debería ser "mientras team no haya ganado"
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
    sem_wait(&(trainer)->trainer_sem); //Bloqueo post-inicilización

    while (trainer->actual_status != EXIT)
    {
		switch (trainer->actual_operation)
		{
			case OP_EXECUTING_CATCH:
			{
				move_trainer_to_objective (train, OP_EXECUTING_CATCH); //Entre paréntesis debería ir "trainer". No sé por qué funciona así
				int result= send_catch (trainer);
				if (result == 1 )
				{

                    remover_objetivo_global(trainer->actual_objective.name); //Elimino el objetivo glboal
                    list_add(trainer->bag,trainer->actual_objective.name); //Agrego al inventario del entrenador

                    if ( list_size (trainer->bag) >= list_size (trainer->personal_objective)  && //Verifico deadlock. Sacar ese mayor igual
                        !comparar_listas(trainer->bag,trainer->personal_objective))
                    {
                        log_error (logTeam, "Ha ocurrido un DEADLOCK en el entrenador %d", trainer->index);
                        trainer->actual_status = BLOCKED_DEADLOCK; 
                        trainer_to_deadlock (trainer);                       
                    }  
                    else if (comparar_listas(trainer->bag,trainer->personal_objective))//Verifico si cumplió sus objetivos
                         {
                            log_info (internalLogTeam, "El entrenador %d ha alcanzado su objetivo satisfactoriamente", trainer->index);
                            trainer->actual_status = EXIT;
                         } 
                          else if (list_size (trainer->bag) < list_size (trainer->personal_objective))
                              {
                                log_info (internalLogTeam, "El entrenador %d cazó a %s en (%d,%d). Se bloquerá a la espera de un nuevo pokemon"  
                                , trainer->index ,trainer->actual_objective.name,trainer->actual_objective.posx, trainer->actual_objective.posy);
                                trainer->actual_status = BLOCKED_NOTHING_TO_DO;
                                sem_post (&trainer_count); //Incremento contador de entrenadores disponibles para atrapar
                                sem_wait(&trainer->trainer_sem);
                               }

				} else //Caught=0
					{
					//Eliminar pokemon del mapa (la lista mapped_pokemons)
					//Enviar al entrenador a estado BLOCKED_NOTHING_TO_DO
					}
				break;
			}


				case OP_EXECUTING_DEADLOCK:
                {
                    move_trainer_to_objective (trainer, OP_EXECUTING_DEADLOCK);
                    puts ("solucionando deadlock");
                    t_list* lista_pok_faltantes= list_create();
                    t_list *lista_pok_sobrantes = list_create();
                    split_objetivos_capturados(trainer, lista_pok_sobrantes,lista_pok_faltantes);
                    break;
                }
				
				

				default: puts ("Operación no válida"); //exit (INVALID_TRAINER_OP);
		}
    }

    //Liberar recursos y finalizar el hilo del entrenador
    log_info (internalLogTeam,"El entrenador %d finalizó su hilo de ejecución con éxito", trainer->index);
}

/*Productor hacia cola de bloqueados por deadlock*/
void trainer_to_deadlock(Trainer *trainer)
{
    sem_wait (&deadlock_sem2);
    list_add (deadlock_list, trainer); 
    sem_post (&deadlock_sem2);
    sem_post (&deadlock_sem1);
    log_info (internalLogTeam, "Se agregó al entrenador %d a la cola de bloqueados por Deadlock", trainer->index);

    sem_wait (&trainer->trainer_sem); //Bloqueo al entrenador hasta ejecutar el algoritmo de recuperación de deadlock
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
/***************ESTA FUNCIÓN NO GENERA MEMORY LEAKS************/
/*************************************************************/
bool comparar_listas (t_list *lista1, t_list *lista2)
{

    if (list_size(lista1) != list_size(lista2)) return false;

    //t_list *lista_duplicada=duplicar_lista(lista2);
    t_list *lista_duplicada =list_duplicate (lista2);
    void listas_iguales (void *elemento2)
    {
        bool comparar_strcmp (void *element)
        {
            if (!strcmp( (char *)element, elemento2) )
            return true; else return false;
        }

        for (int i=0; i<lista2->elements_count; i++)
        {
            char *elemento1=list_get(lista2,i);
            if (strcmp ( (char*)elemento2, elemento1 ) ==0)
            {
                list_remove_by_condition (lista_duplicada,comparar_strcmp);
            }
        }
    }

    list_iterate(lista1,listas_iguales);
    return (list_is_empty(lista_duplicada));
}


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

/*============================================================================================================
 ************************************Algoritmo de recuperación de deadlock************************************* 
 *
 * Se recibe a un entrenador, y se devuelven por referencia dos listas. Una contiene la lista de pokemones que 
 *      le falta atrapar al entrenador, y la otra contiene los pokemones que atrapó pero que no necesita.
 * ============================================================================================================ 
 */


void split_objetivos_capturados (Trainer *trainer, t_list *lista_pok_sobrantes, t_list *lista_pok_faltantes)
    {
    t_list *lista_objetivos  = trainer->personal_objective;
    t_list *lista_capturados = trainer->bag;
    t_list *lista_objetivos_duplicados= list_duplicate (trainer->personal_objective);
    t_list *lista_capturados_duplicados= list_duplicate (trainer->bag);

    /* "Genera" una lista nueva con los pokemones que le faltan atrapar*/
    void buscar_objetivos_faltantes (void *nombre1)
    {
        bool comparar_strcmp (void *element)
            {
                if (!strcmp( (char *)element, nombre1) )
                return true; else return false;
            }

        for (int i=0; i<lista_objetivos->elements_count; i++)
        {
            char *nombre2= list_get(lista_objetivos,i);
            if ( strcmp ( nombre2, (char *)nombre1) == 0)
            {
            list_remove_by_condition(lista_objetivos_duplicados,comparar_strcmp); //Sólo quedan los objetivos que le faltan
            break;
            }
        }
    }

    /* "Genera" una lista nueva con los pokemones que atrapó y le sobran*/
    void buscar_capturados_sobrantes (void *nombre2)
    {
        bool comparar_strcmp (void *element)
                {
                    if (!strcmp( (char *)element, nombre2) )
                    return true; else return false;
                }

        for (int i=0; i<lista_capturados->elements_count; i++)
        {
            char *nombre1= list_get(lista_capturados,i);
            if ( strcmp ( nombre1, (char *)nombre2) == 0)
            {
            list_remove_by_condition(lista_capturados_duplicados,comparar_strcmp); //Solo quedan los caputadoras que sobran
            break;
            }
        }
    }


    void imprimir2 (void *elemento)
    {
    puts ((char*)elemento);
    }

    list_iterate(lista_capturados, buscar_objetivos_faltantes);
    list_iterate(lista_objetivos, buscar_capturados_sobrantes );


    puts ("Objetivos:");
    list_iterate(lista_objetivos,imprimir2);
    puts ("Capturados:");
    list_iterate(lista_capturados,imprimir2);

    puts ("Los que le faltan:");
    list_iterate(lista_objetivos_duplicados ,imprimir2);
    puts ("");

    puts ("Los que le sobran:");
    list_iterate(lista_capturados_duplicados,imprimir2);

    list_add_all (lista_pok_sobrantes, lista_objetivos_duplicados);
    list_add_all (lista_pok_faltantes, lista_capturados_duplicados );

}



void deadlock_recovery (void)
{
    //Correr algoritmo de detección (menos la primera vez)
   // for (int i=0; i<deadlock_list->elements_count; i++)
    //{
   
    Trainer *trainer1 = list_get (deadlock_list, 0);
    Trainer *trainer2 = list_get (deadlock_list, 1);

    t_list *trainer1_sobrantes=list_create ();
    t_list *trainer1_faltantes=list_create (); 

    t_list *trainer2_sobrantes=list_create ();
    t_list *traienr2_faltantes=list_create ();

    split_objetivos_capturados(trainer1, trainer1_sobrantes, trainer1_faltantes);
    split_objetivos_capturados(trainer2, trainer2_sobrantes, trainer2_faltantes);

    //}
    void imprimir3 (void *element)
    {
        puts (elemento);
    }
    puts ("Sobrantes entrenador 1: ");
    list_iterate(trainer1_sobrantes, imprimir3);
    puts ("Faltantes entrenador 1: ");
    list_iterate(trainer1_faltantes, imprimir3);
    puts ("Sobrantes entrenador 2: ");
    list_iterate(trainer2_sobrantes, imprimir3);
    puts ("Faltantes entrenador 1: ");
    list_iterate(trainer2_faltantes, imprimir3);


}























void move_trainer_to_objective (Trainer *trainer, Operation op)
{

    switch (op)
    {
        case OP_EXECUTING_CATCH:
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
    case (OP_EXECUTING_DEADLOCK):
    {
        //DESARROLLAR
    }

}
}

u_int32_t calculate_distance (u_int32_t Tx, u_int32_t Ty, u_int32_t Px, u_int32_t Py )
{
    return (abs (Tx - Px) + abs (Ty - Py));
}
