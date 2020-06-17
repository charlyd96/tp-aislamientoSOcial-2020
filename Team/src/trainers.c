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

void* trainer_to_catch(void *this_team)
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

    while (1)
    {      
         // Este semáforo bloquea el proceso de planificación si no hay entrenadores para mandar a ready
                        // Se le deberá hacer el post una vez que el entrenador reciba el caught y pueda volver a ser  planificado
        pthread_mutex_lock (&global_sem);
        if (list_size (global_objective) > 1 )
        {
            pthread_mutex_unlock (&global_sem);

            sem_wait (&trainer_count);
            sem_wait(&poklist_sem); //Para evitar espera activa si no hay pokemones en el mapa. Revisar este comentario.
            sem_wait(&poklist_sem2); //Para evitar espera activa si no hay pokemones en el mapa. En realidad es por productor consumidor
            actual_pokemon =  list_remove (mapped_pokemons, 0);
            sem_post(&poklist_sem2);

            mover_objetivo_a_lista_auxiliar (actual_pokemon);
        
            list_iterate (team->trainers,calculate_distance);
            printf ("target= %d\n",target);
            if (target ==1) //Si se pudo encontrar un entrenador libre y más cercano que vaya a cazar al pokemon, avanzo y lo mando
            {
                ( (Trainer*) list_get (team->trainers, index))->actual_objective.posx = actual_pokemon->posx;
                ( (Trainer*) list_get (team->trainers, index))->actual_objective.posy = actual_pokemon->posy;
                ( (Trainer*) list_get (team->trainers, index))->actual_objective.name = string_duplicate (actual_pokemon->name); //Malloc
                ( (Trainer*) list_get (team->trainers, index))->actual_status= READY;                                           //oculto
                ( (Trainer*) list_get (team->trainers, index))->actual_operation= OP_EXECUTING_CATCH;        
                
                send_trainer_to_ready (team->trainers, index, OP_EXECUTING_CATCH);

                count=-1;
                index=-1;
                distance_min= 100000  ; //Arreglar esta hardcodeada trucha
                target=-1;
            }
        } 
        else 
            {   
                pthread_mutex_unlock (&global_sem);
                pthread_mutex_lock (&auxglobal_sem);                
                if (list_size (aux_global_objective) > 0 ) 
                {

                    pthread_mutex_unlock (&auxglobal_sem);
                    sem_wait (&trainer_count);
                }
                else 
                {
                    pthread_mutex_unlock (&auxglobal_sem);
                    break;
                } 
            }
    }

    puts ("Terminó la búsqueda de pokemones");
    //***********LIBERAR MEMORIA Y TERMINAR EL HILO **************//
}

/* Productor hacia cola Ready */
void send_trainer_to_ready (t_list *lista, int index, Operation op)

{
    printf ("OP vale %d\n",op);
    switch (op)
    {
        case OP_EXECUTING_CATCH:
        {
            sem_wait (&qr_sem2);
            list_add (ReadyQueue , list_get (lista, index) );
            sem_post (&qr_sem2);
            sem_post (&qr_sem1);
            log_info (internalLogTeam, "Se planificó a Ready al entrenador %d para atrapar un pokemon", index);
            break;
        }
        case OP_EXECUTING_DEADLOCK:
        {
            sem_wait (&qr_sem2);
            list_add (ReadyQueue , list_get (lista, index) );
            sem_post (&qr_sem2);
            sem_post (&qr_sem1);
            log_info (internalLogTeam, "Se planificó a Ready al entrenador %d para resolver un DEADLOCK", 
            ( (Trainer*) list_get(deadlock_list, index) )->index);
            break;
        }
    }
}

void mover_objetivo_a_lista_auxiliar (mapPokemons *actual_pokemon)
{
    pthread_mutex_lock (&auxglobal_sem);
    list_add (aux_global_objective , actual_pokemon);
    pthread_mutex_unlock (&auxglobal_sem);

    bool comparar (void *element)
    {
        if (!strcmp( (char *)element, actual_pokemon->name))
        return true; else return false;
    }

    pthread_mutex_lock (&global_sem);
    list_remove_by_condition (global_objective,comparar); 
    pthread_mutex_unlock (&global_sem);     
}

void send_trainer_to_exec (Config *config)
{
    pthread_t thread;
    pthread_create(&thread, NULL, trainer_exec,config);
    pthread_detach(thread);
}

void *trainer_exec (void *configuracion)
{
    exec_error error;
    Config *config=configuracion;
    while (1) //Este while debería ser "mientras team no haya ganado"
    {

    if (!strcmp(config->planning_algorithm, "FIFO"))
    {
       error= fifo_exec ();
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
                    log_info (internalLogTeam, "El entrenador %d cazó a %s en (%d,%d)"  
                             , trainer->index ,trainer->actual_objective.name,trainer->actual_objective.posx, trainer->actual_objective.posy);

                    remover_objetivo_global_auxiliar(trainer->actual_objective.name); //Elimino el objetivo global auxiliar
                    list_add(trainer->bag,trainer->actual_objective.name); //Agrego al inventario del entrenador
                    
                    if (detectar_deadlock (trainer))     
                    {
                        log_error (logTeam, "Ha ocurrido un DEADLOCK en el entrenador %d", trainer->index);
                        trainer->actual_status = BLOCKED_DEADLOCK; 
                        trainer_to_deadlock (trainer);
                        sem_wait (&trainer->trainer_sem); //Bloqueo al entrenador hasta ejecutar el algoritmo de recuperación de deadlock
                    }  
                        else if (comparar_listas(trainer->bag,trainer->personal_objective))//Verifico si cumplió sus objetivos
                            {
                                log_info (internalLogTeam, "El entrenador %d ha alcanzado su objetivo satisfactoriamente", trainer->index);
                                trainer->actual_status = EXIT;
                            } 
                            else if (list_size (trainer->bag) < list_size (trainer->personal_objective))
                                {
                                    log_info (internalLogTeam, "El entrenador %d se bloquerá a la espera de un nuevo pokemon", trainer->index);
                                    trainer->actual_status = BLOCKED_NOTHING_TO_DO;
                                    sem_post (&trainer_count); //Incremento contador de entrenadores disponibles para atrapar
                                    sem_wait(&trainer->trainer_sem);
                                }

				} else //Caught=0
					{
					//Eliminar pokemon del mapa (la lista mapped_pokemons)
					//Enviar al entrenador a estado BLOCKED_NOTHING_TO_DO
                    //list_add (global_objective, aux_global_objective) //Esto está escrito así nomás
                    sem_post (&trainer_count);
                    sem_wait(&trainer->trainer_sem);
					}
				break;
			}


				case OP_EXECUTING_DEADLOCK:
                {
                    move_trainer_to_objective (trainer, OP_EXECUTING_DEADLOCK);
                    intercambiar(trainer, list_get(deadlock_list, trainer->objetivo.index_objective));
                    list_remove (deadlock_list,0);
                    if (detectar_deadlock(trainer))
                    {
                    trainer->actual_status = BLOCKED_DEADLOCK;
                    trainer_to_deadlock (trainer);
                    sem_post (&resolviendo_deadlock);
                    sem_wait (&trainer->trainer_sem); //Bloqueo al entrenador hasta ejecutar el algoritmo de recuperación de deadlock
                    }
                    else 
                    {
                    trainer->actual_status = EXIT; 
                    sem_post (&resolviendo_deadlock);  
                    }
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
    sem_wait (&deadlock_sem2); //Verificar si se requiere sincronización
    list_add (deadlock_list, trainer); 
    sem_post (&deadlock_sem2);
    sem_post (&deadlock_sem1);
    log_info (internalLogTeam, "Se agregó al entrenador %d a la cola de bloqueados por Deadlock", trainer->index);
    sem_post (&trainer_count); //Revisar esto. Se lo disponibiliza para ser planificado, pero sólo para solucionar deadlock
    sem_post(&using_cpu); //Disponibilizar la CPU para otro entrenador
}   

void remover_objetivo_global_auxiliar(char *name_pokemon)

{
    bool remover (void * element)
    {
        if (strcmp(name_pokemon,(char*)element))
        return (true);
        else return (false);
    }
    pthread_mutex_lock (&auxglobal_sem);
    list_remove_and_destroy_by_condition(aux_global_objective, remover, free);
    pthread_mutex_unlock (&auxglobal_sem);
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

/*
    puts ("Objetivos:");
    list_iterate(lista_objetivos,imprimir2);
    puts ("");
    puts ("Capturados:");
    list_iterate(lista_capturados,imprimir2);
    puts ("");
    puts ("Los que le faltan:");
    list_iterate(lista_objetivos_duplicados ,imprimir2);
    puts ("");
    puts ("Los que le sobran:");
    list_iterate(lista_capturados_duplicados,imprimir2);
    puts ("");*/
    list_add_all (lista_pok_faltantes, lista_objetivos_duplicados);
    list_add_all (lista_pok_sobrantes, lista_capturados_duplicados );

}



int deadlock_recovery (void)
{
    
    printf ("Size lista de deadlock: %d\n", list_size (deadlock_list));
    if (!detectar_deadlock (list_get(deadlock_list, 0)))
    {
        puts ("voy a romper");
        Trainer *trainer = list_remove (deadlock_list,0);
        puts ("pase");
        trainer->actual_status=EXIT;
        sem_post(&trainer->trainer_sem);
        sem_post(&resolviendo_deadlock);
        puts ("rrompi");
        return 0;
    }
    log_info (logTeam,"Se comenzó a ejecutar el algoritmo de recuperación de DEADLOCK");
    //Correr algoritmo de detección (menos la primera vez)
   // for (int i=0; i<deadlock_list->elements_count; i++)
    //{
    static int index=0; //Variable estática que mantiene su valor cada vez que se llama a la función
    index++;

    Trainer *trainer1 = list_get (deadlock_list, 0);
    Trainer *trainer2 = list_get (deadlock_list, index);

    t_list *trainer1_sobrantes=list_create ();
    t_list *trainer1_faltantes=list_create (); 

    t_list *trainer2_sobrantes=list_create ();
    t_list *trainer2_faltantes=list_create ();

    split_objetivos_capturados(trainer1, trainer1_sobrantes, trainer1_faltantes);
    split_objetivos_capturados(trainer2, trainer2_sobrantes, trainer2_faltantes);

    char *entregar_trainer1; //recibir_trainer2
    char *recibir_trainer1; //entregar_trainer2
    int flag_encontrado=0;

    for (int j=0; j<trainer2_faltantes->elements_count; j++)
    {
        bool comparar_strcmp (void *element_trainer1_sobrantes)
            {
                if (!strcmp( (char *)element_trainer1_sobrantes , list_get(trainer2_faltantes, j) ) )
                {
                    flag_encontrado=1;
                    return true;
                }
                else return false;
            }       
        //Busco el pokemon que le sobra a trainer1, para entregarlo a trainer2
        entregar_trainer1=list_find (trainer1_sobrantes, comparar_strcmp);
        if (flag_encontrado == 1) break;
    }

    flag_encontrado=0;

    for (int k=0; k<trainer1_faltantes->elements_count; k++)
    {
        bool comparar_strcmp (void *element_trainer2_sobrantes)
            {
                if (!strcmp( (char *)element_trainer2_sobrantes , list_get(trainer1_faltantes, k) ) )
                {
                    flag_encontrado=1;
                    return true;
                }
                else return false;
            }       
        
        recibir_trainer1=list_find (trainer2_sobrantes, comparar_strcmp);
        if (flag_encontrado == 1) break;
    }  
    
    // Si ambos son NULL, no puedo hacer intercambio. Correr algoritmo con otro entrenador 
    if (entregar_trainer1 == NULL && recibir_trainer1== NULL)
    {
    //Ejectur el algoritmo de recuperación nuevamente con otro entrenador
        puts ("primer if");
        if (index+1 >=deadlock_list->elements_count)// Si index es mayor que la cantidad de bloquedaos en DL, abortar el programa.
            {                                       // No se pudo resolver el deadlock.
            puts ("No se pudo resolver el deadlock. Abortando programa");
            exit (RECOVERY_DEADLOCK_ERROR);
            }
        printf ("Resultado interno:%d\n",deadlock_recovery()); //Llamado recursivo. Liberar recursos del primer llamado al regresar del llamado recursivo.
        index=0;
        return (RECURSIVE_RECOVERY_SUCESS); //Retornar el éxito dando aviso de que fue recursivo
    } else 

        //Si trainer1 le puede dar un pokemon a trainer2, pero trainer2 no tiene ninguno que le sirva a trainer1, le saco el primero
        // que no le sirva a trainer2 y se lo paso a trainer1
        if (entregar_trainer1 != NULL && recibir_trainer1 == NULL)
        {
        puts ("segundo if");
        recibir_trainer1 = list_get(trainer2_sobrantes, 0);
        } else
        
            //Si trainer2 le puede dar un pokemon a trainer1, pero trainer1 no tiene ninguno que le sirva a trainer2, le saco el primero
            // que no le sirva a trainer1 y se lo paso a trainer2
            if (entregar_trainer1 == NULL && recibir_trainer1 != NULL)
            {
            puts ("tercer if");
            entregar_trainer1 = list_get(trainer1_sobrantes, 0);
            } //Si no se cumple ninguna, se realiza efectivo para ambos entrenadores

    trainer1->objetivo.recibir=recibir_trainer1;
    trainer1->objetivo.entregar=entregar_trainer1;
    trainer1->objetivo.posx=trainer2->posx;
    trainer1->objetivo.posy=trainer2->posy;
    trainer1->objetivo.index_objective=index;
    puts ("SEND TRAINER TO READY");
    trainer1->actual_operation = OP_EXECUTING_DEADLOCK;
    trainer2->actual_operation = OP_EXECUTING_DEADLOCK; //Mejorar esto ya que puede ser confuso. Agregar un estado "waiting for deadlock"
    send_trainer_to_ready(deadlock_list, 0, OP_EXECUTING_DEADLOCK);
    
    


    //realizar_intercambio()

    
    
    
   /* printf ("Trainer %d entregará %s\n", trainer1->index, entregar_trainer1);
    printf ("Trainer %d recibirá %s\n",  trainer1->index, recibir_trainer1);
    puts ("");

    
    //}
    void imprimir3 (void *element)
    {
        puts (element);
    }
    printf ("Sobrantes entrenador %d:\n",trainer1->index);
    list_iterate(trainer1_sobrantes, imprimir3);
    printf ("Faltantes entrenador %d:\n",trainer1->index);
    list_iterate(trainer1_faltantes, imprimir3);
    printf ("Sobrantes entrenador %d:\n",trainer2->index);
    list_iterate(trainer2_sobrantes, imprimir3);
    printf ("Faltantes entrenador %d:\n",trainer2->index);
    list_iterate(trainer2_faltantes, imprimir3);*/
    index=0; //Reiniciar el index para ejecutar nuevamente el algoritmo con otros entrenadores luego del intercambio
    return (0);

}


int intercambiar(Trainer *trainer1, Trainer *trainer2)
{

     void imprimir (void *element)
    {
        puts ((char*)element);
    }
    puts ("Antes del intercambio");
    printf ("Bag entrenador %d:\n", trainer1->index);
    list_iterate (trainer1->bag,imprimir);
    puts ("");
    printf ("Bag entrenador %d:\n", trainer2->index);
    list_iterate (trainer2->bag,imprimir);
    char *recibir=trainer1->objetivo.recibir;
    char *entregar=trainer1->objetivo.entregar;

    bool comparar_strcmp_recibir (void *element)
    {
        if (!strcmp( (char *)element, recibir) )
        return true; else return false;
    }

    bool comparar_strcmp_entregar (void *element)
    {
        if (!strcmp( (char *)element, entregar) )
        return true; else return false;
    }
    char *aux_recibir=list_remove_by_condition(trainer2->bag,comparar_strcmp_recibir);
    char *aux_entregar=list_remove_by_condition(trainer1->bag,comparar_strcmp_entregar);
   
    list_add(trainer1->bag,aux_recibir);
    list_add(trainer2->bag,aux_entregar);

    puts ("Después del intercambio");
     printf ("Bag entrenador %d:\n", trainer1->index);
    list_iterate (trainer1->bag,imprimir);
    puts ("");
     printf ("Bag entrenador %d:\n", trainer2->index);
    list_iterate (trainer2->bag,imprimir);
}


bool detectar_deadlock (Trainer *trainer)
{
 if (list_size (trainer->bag) >= list_size (trainer->personal_objective)  && !comparar_listas(trainer->bag,trainer->personal_objective))
 return true; 
 else return false;
//Verifico deadlock. Sacar ese mayor igual
}
















void move_trainer_to_objective (Trainer *trainer, Operation op)
{
    /*Sólo para mejorar la expresividad*/
    u_int32_t *Tx;
    u_int32_t *Ty;
    u_int32_t *Px;
    u_int32_t *Py;
    printf ("Operacion: %d\n", op);
    switch (op)
    {
        case OP_EXECUTING_CATCH:
        {     
            Tx=&(trainer->posx);
            Ty=&(trainer->posy);
            Px=&(trainer->actual_objective.posx);
            Py=&(trainer->actual_objective.posy);
            log_info (internalLogTeam, "El entrenador %d empezó a ejecutar la caza de un pokemon", trainer->index);
            break;
        }
        case OP_EXECUTING_DEADLOCK: 
        {
            Tx=&(trainer->posx);
            Ty=&(trainer->posy);
            Px=&(trainer->objetivo.posx);
            Py=&(trainer->objetivo.posy);
            log_info (internalLogTeam, "El entrenador %d empezó a ejecutar una recuperación de DEADLOCK con el entrenador %d", 
            trainer->index, ( (Trainer *) list_get(deadlock_list, (trainer->objetivo.index_objective)))->index);
            break;
        }    

    }
    
    while ( (*Tx != *Px) || (*Ty!=*Py) )
    {
        if ( calculate_distance (*Tx+1, *Ty, *Px, *Py  ) < calculate_distance (*Tx, *Ty, *Px, *Py ) ){
        *Tx=*Tx+1;
        //log_info (logTeam , "Se movió un entrenador hacia la derecha. Posición: (%d,%d)", *Tx, *Ty);
        }

        if ( calculate_distance (*Tx, *Ty+1, *Px, *Py  ) < calculate_distance (*Tx, *Ty, *Px, *Py ) ){
        *Ty=*Ty+1;
        //log_info (logTeam , "Se movió un entrenador hacia arriba. Posición: (%d,%d)", *Tx, *Ty);
        }

        if ( calculate_distance (*Tx-1, *Ty, *Px, *Py  ) < calculate_distance (*Tx, *Ty, *Px, *Py ) ){
        *Tx=*Tx-1;
        //log_info (logTeam , "Se movió un entrenador hacia la izquierda. Posición: (%d,%d)", *Tx, *Ty);
        }

        if ( calculate_distance (*Tx, *Ty-1, *Px, *Py  ) < calculate_distance (*Tx, *Ty, *Px, *Py ) ){
        *Ty=*Ty-1;
       // log_info (logTeam , "Se movió un entrenador hacia abajo. Posición: (%d,%d)", *Tx, *Ty);
        }
        usleep (300000);
    }
}

u_int32_t calculate_distance (u_int32_t Tx, u_int32_t Ty, u_int32_t Px, u_int32_t Py )
{
    return (abs (Tx - Px) + abs (Ty - Py));
}


