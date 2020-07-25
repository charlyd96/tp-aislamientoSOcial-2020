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
 
void trainer_to_catch(void)
{
    int index=0;
    int target=-1;
    u_int32_t distance_min= 100000  ; //Arreglar esta hardcodeada trucha
    mapPokemons *actual_pokemon;
    int i;
    int pokPosx;
    int pokPosy;
    planificar=true;
    sem_getvalue(&trainer_count,&i); //Saltea el proceso de planificación por distancia si se inicializa con todos los pokemones atrapados
    if (i==0) planificar=false; 

    void calculate_distance (void *element)
    {
        
        Trainer *trainer=element;
       // count++;
        if ( trainer->actual_status ==NEW ||trainer->actual_status == BLOCKED_NOTHING_TO_DO) //Sólo planifica entrenadores no bloqueados
        {   
            u_int32_t Tx= trainer->posx;
            u_int32_t Ty= trainer->posy;
            u_int32_t Px= pokPosx;
            u_int32_t Py= pokPosy;
            u_int32_t distance = abs (Tx - Px) + abs (Ty - Py); //Reemplazar por función "calcular_distancia"
            if (distance < distance_min)
            {
                distance_min=distance;
                index=trainer->index;
                target=1;
            }
        }
    }

    while (planificar)
    {             
        sem_wait (&trainer_count); // Este semáforo bloquea el proceso de planificación si no hay entrenadores para mandar a ready
        if (!planificar) break;
        log_info(internalLogTeam, "Lista global: %d\n", list_size (global_objective));
        log_info(internalLogTeam, "Lista global aux: %d\n", list_size (aux_global_objective));
            
        sem_wait(&poklist_sem); //Para evitar espera activa si no hay pokemones en el mapa. Revisar este comentario.
        sem_wait(&poklist_sem2); //Para evitar espera activa si no hay pokemones en el mapa. En realidad es por productor consumidor
        if (!planificar) break;
        actual_pokemon =  list_remove (mapped_pokemons, 0);
        sem_post(&poklist_sem2);
        char *nombre_pokemon = string_duplicate(actual_pokemon->name);
        pokPosx= actual_pokemon->posx;
        pokPosy= actual_pokemon->posy;
        free(actual_pokemon->name);
        free(actual_pokemon);

        //mover_objetivo_a_lista_auxiliar (nombre_pokemon);
        
        void imprimir_estados (void *trainer)
        {
        log_info(internalLogTeam, "Estado %d: %d\t",((Trainer*)trainer)->index,((Trainer*)trainer)->actual_status);   
        }
        //list_iterate (trainers,imprimir_estados);

        list_iterate (trainers,calculate_distance);
        
        //log_info(internalLogTeam, "indice planificado: %d\n", index);
        if (target == 1) //Si se pudo encontrar un entrenador libre y más cercano que vaya a cazar al pokemon, avanzo y lo mando
        {
            Trainer *trainer=list_get(trainers, index);
            trainer->actual_objective.posx = pokPosx;
            trainer->actual_objective.posy = pokPosy;
            trainer->actual_objective.name = nombre_pokemon;
            trainer->actual_status= READY;
            trainer->actual_operation= OP_EXECUTING_CATCH; 

            // 3. Operación de atrapar (indicando la ubicación y el pokemon a atrapar).
            //log_info(internalLogTeam, "El entrenador %d se planificó para atrapar un %s ubicado en (%d,%d)\n", trainer->index,trainer->actual_objective.name,pokPosx,pokPosy);
            //log_info(logTeam, "El entrenador %d se planificó para atrapar un %s ubicado en (%d,%d)\n",         trainer->index,trainer->actual_objective.name,pokPosx,pokPosy);
            send_trainer_to_ready (trainers, index, OP_EXECUTING_CATCH); 
            index=-1;
            distance_min= 100000  ; //Arreglar esta hardcodeada trucha
            target=-1;
        }
    }
    log_info(internalLogTeam, "Terminó la búsqueda de pokemones");
    //***********LIBERAR MEMORIA Y TERMINAR EL HILO **************//
}  

/* Productor hacia cola Ready */
void send_trainer_to_ready (t_list *lista, int index, Operation op) //Eliminar el switch, es innecesario.
{
    switch (op)
    {
        case OP_EXECUTING_CATCH:
        {
            Trainer *trainer = list_get (lista, index);
            sem_wait (&qr_sem2);
            newTrainerToReady=true;
            list_add (ReadyQueue , trainer );
            sem_post (&qr_sem2);
            sem_post (&qr_sem1);
            log_info (internalLogTeam, "[CAMBIO DE COLA] Se planificó a Ready al entrenador %d para atrapar un %s ubicado en (%d,%d)", index, trainer->actual_objective.name,trainer->actual_objective.posx, trainer->actual_objective.posy);
            log_info (logTeam, "[CAMBIO DE COLA] Se planificó a Ready al entrenador %d para atrapar un  %s ubicado en (%d,%d)", index,trainer->actual_objective.name,trainer->actual_objective.posx, trainer->actual_objective.posy);
            break;
        }
        case OP_EXECUTING_DEADLOCK:
        {
            Trainer *trainer = list_get (lista, index);
            sem_wait (&qr_sem2);
            newTrainerToReady=true;
            list_add (ReadyQueue , trainer );
            sem_post (&qr_sem2);
            sem_post (&qr_sem1);
            log_info (logTeam, "[CAMBIO DE COLA] Se planificó a Ready al entrenador %d para resolver un DEADLOCK", 
            ( (Trainer*) list_get(lista, index) )->index); //Verificar si en esta lista va trainers
            
            log_info (internalLogTeam, "[CAMBIO DE COLA] Se planificó a Ready al entrenador %d para resolver un DEADLOCK", 
            ( (Trainer*) list_get(lista, index) )->index);
            break;
        }
    }
}

void mover_objetivo_a_lista_auxiliar (char *name)
{
    pthread_mutex_lock (&auxglobal_sem);
    list_add (aux_global_objective , name);
    pthread_mutex_unlock (&auxglobal_sem);

    bool remover (void *element)
    {
        if (!strcmp( (char *)element, name))
        return true; 
        else return false;
    }
    pthread_mutex_lock (&global_sem);
    list_remove_by_condition (global_objective,remover); 
    pthread_mutex_unlock (&global_sem);     
}


void mover_objetivo_a_lista_global(char *nombre_pokemon)
{
    pthread_mutex_lock (&global_sem);
    list_add (global_objective , nombre_pokemon);
    pthread_mutex_unlock (&global_sem);
    remover_objetivo_global_auxiliar(nombre_pokemon);
}


void remover_objetivo_global_auxiliar(char *name_pokemon)
{
    bool remover (void * element)
    {
        if (!strcmp(name_pokemon,(char*)element))
        return (true);
        else return (false);
    }
    pthread_mutex_lock (&auxglobal_sem);
    list_remove_by_condition(aux_global_objective, remover);
    pthread_mutex_unlock (&auxglobal_sem);
}


/*================================================================================================================================
***********Funciones para tratar el filtrado de los mensajes APPEARED que llegan por el gameboy o por el broker*******************
 =================================================================================================================================
*/

void nuevos_pokemones_CAUGHT_SI(char *nombre_pokemon)
{
    bool buscar (void * element)
    {
        if (!strcmp(nombre_pokemon,(char*)element))
        return (true);
        else return (false);
    }
	pthread_mutex_lock(&aux_global_sem);
    list_remove_by_condition(aux_global_objective, buscar);
    bool hayPokemones=list_any_satisfy(aux_global_objective, buscar);
    pthread_mutex_unlock(&aux_global_sem);

    if (hayPokemones==false)
    remover_pokemones_en_mapa_auxiliar(nombre_pokemon);
}

void nuevos_pokemones_CAUGHT_NO (char *nombre_pokemon)
{
    bool buscar (void *pokemon)
    {
        if ( !strcmp(nombre_pokemon,((mapPokemons*) pokemon)->name) )
        return (true);
        else return (false);
    }
	pthread_mutex_lock(&aux_new_global_sem);
    mapPokemons *nuevo_pokemon = list_remove_by_condition(mapped_pokemons_aux, buscar); //Busco el pokemon en el mapa auxiliar
    pthread_mutex_unlock(&aux_new_global_sem);

    if (nuevo_pokemon!=NULL) //Si se encontró un pokemon en el mapa auxiliar, moverlo al mapa principal
    mover_pokemon_al_mapa(nuevo_pokemon);
    else //Si no se encontró un pokemon en el mapa auxiliar, mover el nombre pokemon pendiente de la lista auxiliar a la lista global de pendientes
    {
    mover_elemento_destino_origen(global_objective, aux_global_objective, &global_sem, &aux_global_sem,nombre_pokemon);
    }

}

void mover_de_aux_a_global(char *name)
{
    bool buscar (void * element)
    {
        if (!strcmp(name,(char*)element))
        return (true);
        else return (false);
    }
    pthread_mutex_lock(&aux_new_global_sem);
    list_remove_by_condition(aux_new_global_objective,buscar);
    pthread_mutex_unlock(&aux_new_global_sem);


    pthread_mutex_lock(&new_global_sem);
    list_add(new_global_objective, name);
	pthread_mutex_unlock(&new_global_sem);

}

void mover_pokemon_al_mapa (mapPokemons *nuevo_pokemon)
{
    sem_wait(&poklist_sem2);
    list_add(mapped_pokemons, nuevo_pokemon );
    sem_post(&poklist_sem);
    sem_post(&poklist_sem2);
}

void remover_pokemones_en_mapa_auxiliar(char *nombre_pokemon)
{
    bool buscar (void *pokemon)
    {
        mapPokemons *pok=pokemon;
        if (!strcmp(nombre_pokemon, pok->name))
        return (true);
        else return (false);
    }
    mapPokemons *pokemonAEliminar=list_remove_by_condition(mapped_pokemons_aux, buscar);
    while (pokemonAEliminar != NULL)
    {
        free (pokemonAEliminar->name);
        free (pokemonAEliminar);
        pokemonAEliminar = list_remove_by_condition(mapped_pokemons_aux, buscar);
    }
}


void send_trainer_to_exec (void)
{
    pthread_t thread;

    switch (algoritmo)
    {
        case FIFO:
        {
        pthread_create(&thread, NULL, (void*)fifo_exec,NULL);
        pthread_detach(thread);
        break;
        }

        case RR:
        {
        pthread_create(&thread, NULL, (void*)RR_exec,NULL);
        pthread_detach(thread);

        break;
        }

        case SJFSD:
        {
        pthread_create(&thread, NULL, (void*)SJFSD_exec,NULL);
        pthread_detach(thread);

        break;
        }

        case SJFCD:
        {
        pthread_create(&thread, NULL, (void*)SJFCD_exec,NULL);
        pthread_detach(thread);

        break;
        }

        default:;
    }
}

/*Desbloquea la planificación si todos los entrenadores quedaron bloqueados por deadlock o terminaron, para habilitar la fase de recuperación de Dadlock*/

void desbloquear_planificacion(void)
{
    int trainer_block_exit=0;
    void buscar_entrenadores_libres (void *entrenador)
    {
        Trainer *trainer=entrenador;
        if (trainer->actual_status==BLOCKED_DEADLOCK || trainer->actual_status==EXIT)
        trainer_block_exit++;  
    }
    list_iterate(trainers,buscar_entrenadores_libres);
    if (list_size(trainers)==trainer_block_exit)
    {
    planificar=false;
    sem_post(&trainer_count);
    }

}


void trainer_routine (Trainer *trainer)
{
    trainer->ejecucion=EXECUTING; //Esto es para que SJF-CD valgrind no arroje que se está haciendo una comparación con un valor no inicializado
    sem_wait(&(trainer)->trainer_sem); //Bloqueo post-inicilización
   
    while (trainer->actual_status != EXIT)
    {              
		switch (trainer->actual_operation)
		{
			case OP_EXECUTING_CATCH:
			{
				move_trainer_to_objective (trainer, OP_EXECUTING_CATCH); //Entre paréntesis debería ir "trainer". No sé por qué funciona así
                consumir_cpu(trainer);
				int result= send_catch (trainer);
                
				if (result == 1 )
				{
                    log_warning (internalLogTeam, "El entrenador %d cazó a %s en (%d,%d)"  
                             , trainer->index ,trainer->actual_objective.name,trainer->actual_objective.posx, trainer->actual_objective.posy);

                    eliminar_elemento_lista(trainer->actual_objective.name,aux_global_objective, &aux_global_sem);//Elimino el objetivo global auxiliar

                    nuevos_pokemones_CAUGHT_SI(trainer->actual_objective.name); //Elimino el objetivo del mapa auxiliar (si otro entrenador no lo necesita)
                    list_add(trainer->bag,trainer->actual_objective.name); //Agrego al inventario del entrenador
                    
                    if (detectar_deadlock (trainer))     
                    {

                        trainer->actual_status = BLOCKED_DEADLOCK;
                        
                        trainer_to_deadlock (trainer);
                        desbloquear_planificacion();
                        sem_wait (&trainer->trainer_sem); //Bloqueo al entrenador hasta ejecutar el algoritmo de recuperación de deadlock
                    }  
                        else if (comparar_listas(trainer->bag,trainer->personal_objective))//Verifico si cumplió sus objetivos
                            {
                                // 8. Resultado del Team (especificado anteriormente).
                                log_info (internalLogTeam, "El entrenador %d alcanzó su objetivo satisfactoriamente.", trainer->index);
                                 log_info (logTeam, "El entrenador %d alcanzó su objetivo satisfactoriamente.", trainer->index);
                                trainer->actual_status = EXIT;
                                desbloquear_planificacion();
                            } 
                            else if (list_size (trainer->bag) < list_size (trainer->personal_objective)) //sacar este último if
                                {
                                    // 8. Resultado del Team (especificado anteriormente).
                                    log_info (internalLogTeam, "[CAMBIO DE COLA] El entrenador %d se bloquerá a la espera de un nuevo Pokemon.", trainer->index);
                                    log_info (logTeam, "[CAMBIO DE COLA] El entrenador %d se bloquerá a la espera de un nuevo Pokemon.", trainer->index);
                                    trainer->actual_status = BLOCKED_NOTHING_TO_DO;
                                    sem_post (&trainer_count); //Incremento contador de entrenadores disponibles para atrapar
                                    sem_wait(&trainer->trainer_sem);
                                }

				} else //Caught=0
					{
                        // 8. Resultado del Team (especificado anteriormente).
                        log_error (internalLogTeam, "El entrenador %d no pudo cazar a %s en (%d,%d).",trainer->index, trainer->actual_objective.name,trainer->actual_objective.posx, trainer->actual_objective.posy);
                        log_error (logTeam, "El entrenador %d no pudo cazar a %s en (%d,%d).",trainer->index, trainer->actual_objective.name,trainer->actual_objective.posx, trainer->actual_objective.posy);
                        mover_elemento_destino_origen(global_objective,aux_global_objective, &global_sem, &aux_global_sem, trainer->actual_objective.name);//Vuelvo a setear el objetivo global
                        nuevos_pokemones_CAUGHT_NO (trainer->actual_objective.name); //Intento agregar un pokemon de esa especie al mapa
                        free(trainer->actual_objective.name);
                        trainer->actual_status= BLOCKED_NOTHING_TO_DO;
                        sem_post (&trainer_count);
                        sem_wait(&trainer->trainer_sem);
					}
				break;
			}

            case OP_EXECUTING_DEADLOCK:
            {
                move_trainer_to_objective (trainer, OP_EXECUTING_DEADLOCK);
                consumir_cpu_intercambio(trainer);
                intercambiar(trainer, list_get(deadlock_list, trainer->objetivo.index_objective));
                sem_post (&resolviendo_deadlock);
                sem_post (&using_cpu); //Como en este caso no hay mensaje CATCH, necesito disponibilizar la CPU  
                sem_wait (&trainer->trainer_sem); //Bloqueo al entrenador hasta ejecutar el algoritmo de recuperación de deadlock
                break; 
            }
			default: log_error(internalLogTeam, "Operación no válida"); //exit (INVALID_TRAINER_OP);
		}
    }
    
    //Liberar recursos y finalizar el hilo del entrenador
    //int index = trainer->index;
    //liberar_listas_entrenador(trainer);
    log_info (logTeam,"[CABIO DE COLA] El entrenador %d finalizó su hilo de ejecución con éxito", index);
}


void eliminar_elemento_lista (char *elemento1, t_list *lista, pthread_mutex_t *semaforo)
{
    bool comparar(void *elemento2)
        {
            if (!strcmp( (char *)elemento1, elemento2) )
            return true; else return false;
        }
    pthread_mutex_lock(semaforo);
    list_remove_by_condition(lista,comparar);
    pthread_mutex_unlock(semaforo);
}

/*Productor hacia cola de bloqueados por deadlock*/
void trainer_to_deadlock(Trainer *trainer)
{
    sem_wait (&deadlock_sem2); //Verificar si se requiere sincronización (esquema productor consumidor: ¿necesario?)
    list_add_in_index (deadlock_list, 0, trainer); 
    sem_post (&deadlock_sem2);
    sem_post (&deadlock_sem1);
    log_info (internalLogTeam, "[CAMBIO DE COLA] Se agregó al entrenador %d a la cola de bloqueados por Deadlock.", trainer->index);
    //sem_post (&trainer_count); //Revisar esto. Se lo disponibiliza para ser planificado, pero sólo para solucionar deadlock
    
    //sem_post(&using_cpu); //Disponibilizar la CPU para otro entrenador. No es necesario, la CPU se disponibilizó al mandar el CATCH en listen.c.
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
                break;
            }
        }
    }

    list_iterate(lista1,listas_iguales);
    bool result = list_is_empty(lista_duplicada);
    list_destroy (lista_duplicada);
    return (result);
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
    list_destroy (lista_objetivos_duplicados);
    list_destroy (lista_capturados_duplicados);

}


int deadlock_recovery (void)
{
    /*
    log_info(internalLogTeam, "Size lista de deadlock: %d\n", list_size (deadlock_list));
    if (!detectar_deadlock (list_get(deadlock_list, 0)))
    {
        Trainer *trainer = list_remove (deadlock_list,0);
        trainer->actual_status=EXIT;
        sem_post(&trainer->trainer_sem);
        sem_post(&resolviendo_deadlock);
        return 0;
    }*/
    log_info (logTeam,"[RECUPERACION DEADLOCK] Se comenzó a ejecutar el algoritmo de recuperación de DEADLOCK");
    //Correr algoritmo de detección (menos la primera vez)

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
    if ((entregar_trainer1 == NULL && recibir_trainer1== NULL) || (entregar_trainer1 != NULL && recibir_trainer1 == NULL) )
    {
    //Ejectur el algoritmo de recuperación nuevamente con otro entrenador
        log_info(internalLogTeam, "primer if");
        if (index+1 >=deadlock_list->elements_count)// Si index es mayor que la cantidad de bloquedaos en DL, abortar el programa.
            {                                       // No se pudo resolver el deadlock.
            log_info(internalLogTeam, "No se pudo resolver el deadlock. Abortando programa");
            exit (RECOVERY_DEADLOCK_ERROR);
            }

        log_info(internalLogTeam, "Resultado interno:%d\n",deadlock_recovery()); //Llamado recursivo. Liberar recursos del primer llamado al regresar del llamado recursivo.
        index=0;
        list_destroy(trainer1_sobrantes);
        list_destroy(trainer1_faltantes);
        list_destroy(trainer2_sobrantes);
        list_destroy(trainer2_faltantes);
        return (RECURSIVE_RECOVERY_SUCESS); //Retornar el éxito dando aviso de que fue recursivo
    }  
        
    //Si trainer2 le puede dar un pokemon a trainer1, pero trainer1 no tiene ninguno que le sirva a trainer2, le saco el primero
    // que no le sirva a trainer1 y se lo paso a trainer2
    if (entregar_trainer1 == NULL && recibir_trainer1 != NULL)
    {
    log_info(internalLogTeam, "tercer if");
    entregar_trainer1 = list_get(trainer1_sobrantes, 0);
    } //Si no se cumple, se realiza un intercambio efectivo para ambos entrenadores

    trainer1->objetivo.recibir=recibir_trainer1;
    trainer1->objetivo.entregar=entregar_trainer1;
    trainer1->objetivo.posx=trainer2->posx;
    trainer1->objetivo.posy=trainer2->posy;
    trainer1->objetivo.index_objective=index;
    trainer1->actual_operation = OP_EXECUTING_DEADLOCK;
    trainer2->actual_operation = OP_EXECUTING_DEADLOCK; //Mejorar esto ya que puede ser confuso. Agregar un estado "waiting for deadlock"
    send_trainer_to_ready(deadlock_list, 0, OP_EXECUTING_DEADLOCK); //Cambiar esa constante por trainer1->actual_operation
    
    
    log_info(internalLogTeam, "Trainer %d entregará %s", trainer1->index, entregar_trainer1);
    log_info(internalLogTeam, "Trainer %d recibirá %s",  trainer1->index, recibir_trainer1);

    sem_wait (&resolviendo_deadlock);
    
    list_add(trainer1->involucrados,(void*)trainer2->index);
    
    bool deadlockTrainer1= detectar_deadlock(trainer1);
    bool deadlockTrainer2= detectar_deadlock(trainer2);
    if (!deadlockTrainer2) list_remove(deadlock_list,index);
    if (!deadlockTrainer1) list_remove(deadlock_list,0);

    index=0;
    
    /*
    //}
    void imprimir3 (void *element)
    {
        log_info(internalLogTeam, (element);
    }
    printf ("Sobrantes entrenador %d:\n",trainer1->index);
    list_iterate(trainer1_sobrantes, imprimir3);
    printf ("Faltantes entrenador %d:\n",trainer1->index);
    list_iterate(trainer1_faltantes, imprimir3);
    printf ("Sobrantes entrenador %d:\n",trainer2->index);
    list_iterate(trainer2_sobrantes, imprimir3);
    printf ("Faltantes entrenador %d:\n",trainer2->index);
    list_iterate(trainer2_faltantes, imprimir3);*/
    list_destroy(trainer1_sobrantes);
    list_destroy(trainer1_faltantes);
    list_destroy(trainer2_sobrantes);
    list_destroy(trainer2_faltantes);
    return (0);

}


void intercambiar(Trainer *trainer1, Trainer *trainer2)
{
    log_info(logTeam, "[INTERCAMBIO] El entrenador %d hara un intercambio con el entrenador %d", trainer1->index, trainer2->index);
     void imprimir (void *element)
    {
        log_info(internalLogTeam, "%s",(char*)element);
    }
    log_info(internalLogTeam, "Antes del intercambio");
    log_info(internalLogTeam, "Bag entrenador %d tiene %d elementos: ", trainer1->index, list_size(trainer1->bag) );
    list_iterate (trainer1->bag,imprimir);
    log_info(internalLogTeam, "Bag entrenador %d tiene %d elementos: ", trainer2->index, list_size(trainer2->bag) );
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

    log_info(internalLogTeam, "Después del intercambio\n");
    log_info(internalLogTeam, "Bag entrenador %d: ", trainer1->index);
    list_iterate (trainer1->bag,imprimir);
    log_info(internalLogTeam, "Bag entrenador %d: ", trainer2->index);
    list_iterate (trainer2->bag,imprimir);
    
}


bool detectar_deadlock (Trainer *trainer)
{
    // 5. Inicio de algoritmo de detección de deadlock.
    log_warning(internalLogTeam, "[DETECCION DEADLOCK] Se inicia el Algoritmo de Detección de Deadlock.");
    log_warning(logTeam, "[DETECCION DEADLOCK] Se inicia el Algoritmo de Detección de Deadlock.");
        //Agregar: if trainer==NULL--> terminar (no hubo deadlocks)
    if (list_size (trainer->bag) >= list_size (trainer->personal_objective)  && !comparar_listas(trainer->bag,trainer->personal_objective))
    {
        log_info(logTeam, "[DETECCION DEADLOCK] El entrenador %d quedo en deadlock", trainer->index);
        return true; 
    } else
        {
            log_info(logTeam, "[DETECCION DEADLOCK] No existe deadlock en el entrenadores %d", trainer->index);
            return false;
        } 
    //Verifico deadlock. Sacar ese mayor igual
}


/*============================================================================================================
 ************************************Movimiento de entrendores en el mapa************************************* 
 *
 * Se recibe a un entrenador, y un código de operación que indica si el entrenador va a realizar la caza de un 
 *  ṕokemon o la resolución de un deadlock. Al código podría accederse desde la estructura Trainer, pero se
 *            prefirió dejarlo explicitado como parámetro para mejorar la expresividad de la función.
 * ============================================================================================================ 
 */

void move_trainer_to_objective (Trainer *trainer, Operation op)
{
    /*Sólo para mejorar la expresividad*/
    u_int32_t *Tx;
    u_int32_t *Ty;
    u_int32_t *Px;
    u_int32_t *Py;

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
            int value;
            sem_getvalue(&using_cpu,&value);
            printf ("Value: %d\n",value);
            break;
        }    

    }
    while ( (*Tx != *Px) || (*Ty!=*Py) )
    {
        if ( calculate_distance (*Tx+1, *Ty, *Px, *Py  ) < calculate_distance (*Tx, *Ty, *Px, *Py ) ){
        consumir_cpu(trainer);
        *Tx=*Tx+1;
        log_info (logTeam , "[MOVIMIENTO] El entrenador %d se movió  hacia la derecha. Posición: (%d,%d)", trainer->index, *Tx, *Ty);
        }

        if ( calculate_distance (*Tx, *Ty+1, *Px, *Py  ) < calculate_distance (*Tx, *Ty, *Px, *Py ) ){
        consumir_cpu(trainer);
        *Ty=*Ty+1;
        log_info (logTeam , "[MOVIMIENTO] l entrenador %d se movió  hacia arriba. Posición: (%d,%d)", trainer->index, *Tx, *Ty);
        }

        if ( calculate_distance (*Tx-1, *Ty, *Px, *Py  ) < calculate_distance (*Tx, *Ty, *Px, *Py ) ){
        consumir_cpu(trainer);
        *Tx=*Tx-1;
        log_info (logTeam , "[MOVIMIENTO] El entrenador %d se movió  hacia la izquierda. Posición: (%d,%d)", trainer->index, *Tx, *Ty);
        }

        if ( calculate_distance (*Tx, *Ty-1, *Px, *Py  ) < calculate_distance (*Tx, *Ty, *Px, *Py ) ){
        consumir_cpu(trainer);
        *Ty=*Ty-1;
        log_info (logTeam , "[MOVIMIENTO] El entrenador %d se movió  hacia abajo. Posición: (%d,%d)", trainer->index, *Tx, *Ty);
        }
    }
}

u_int32_t calculate_distance (u_int32_t Tx, u_int32_t Ty, u_int32_t Px, u_int32_t Py )
{
    return (abs (Tx - Px) + abs (Ty - Py));
}


void consumir_cpu(Trainer *trainer)
{
   switch (algoritmo)
   {
        case FIFO:
        {
            usleep (config->retardo_cpu * 1); //Uso usleep para que no sea tan lenta la ejecución
            trainer->ciclos_cpu_totales++;
            ciclos_cpu++;
            break;
        }

        case RR:
        {
            if (trainer->rafagaEjecutada >= config->quantum)
            { 
                trainer->actual_status=READY;
                trainer->ejecucion= PENDING;
                sem_post(&using_cpu);
                sem_wait(&trainer->trainer_sem);
                trainer->ejecucion= EXECUTING;
                usleep (config->retardo_cpu * 1); //Uso usleep para que no sea tan lenta la ejecución
            } 
            else
            {
                usleep (config->retardo_cpu * 1); //Uso usleep para que no sea tan lenta la ejecución
                trainer->ejecucion= EXECUTING;
            }
            trainer->rafagaEjecutada++;
            trainer->ciclos_cpu_totales++;
            ciclos_cpu++;
            break;
        }

        case SJFSD:
        {
            usleep (config->retardo_cpu * 1); //Uso usleep para que no sea tan lenta la ejecución
            trainer->rafagaEjecutada++;
            trainer->ciclos_cpu_totales++;
            ciclos_cpu++;
            break;
        }

        case SJFCD:
        {   
            trainer->rafagaEjecutada++;
            trainer->ciclos_cpu_totales++; 
            ciclos_cpu++;
            sem_wait(&qr_sem2);
            if (newTrainerToReady)
            {
                newTrainerToReady=false;
                trainer->rafagaEstimada= trainer->rafagaEstimada-trainer->rafagaEjecutada;
                ordenar_lista_ready();
                if ( ((Trainer *)list_get(ReadyQueue, 0))->rafagaEstimada > trainer->rafagaEstimada ) //Comparo la lista ordenada con el entrenador actual en ejecución
                {//Seguir ejecutando
                log_info(internalLogTeam, "****************************************************************seguir ejecutando************************************************************************");
                log_info(internalLogTeam, "Rafaga estimada actual: %f\nRafaga estimada nuevo entrenador:%f\n",trainer->rafagaEstimada, ((Trainer *)list_get(ReadyQueue, 0))->rafagaEstimada );
                sem_post(&qr_sem2); //Disponibilizar la cola ready
                }
                else 
                {//Desalojar
                log_warning(internalLogTeam, "[CAMBIO DE COLA] El entrenador %d será desalojado por el algoritmo SJF-CD", trainer->index);
                trainer->actual_status=READY;
                trainer->ejecucion=PENDING;
                sem_post(&using_cpu);
                sem_wait(&trainer->trainer_sem);
                trainer->ejecucion=EXECUTING;
                }
            }
            else 
            {
                sem_post(&qr_sem2);
            }        
        usleep (config->retardo_cpu * 1); //Uso usleep para que no sea tan lenta la ejecución
        break;
        }

        default:;

    }
}

void liberar_listas_entrenador(Trainer *trainer)
{
    list_destroy (trainer->personal_objective);
    list_destroy_and_destroy_elements (trainer->bag,free);
}


void consumir_cpu_intercambio(Trainer *trainer)
{
    for (int i=0; i<5; i++)
    {
        consumir_cpu(trainer);
    }
}