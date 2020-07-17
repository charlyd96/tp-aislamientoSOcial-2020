/*
 * teamConfig.c
 *
 *  Created on: 1 jun. 2020
 *      Author: utnso
 */


#include "../include/teamConfig.h"

#include <string.h>
#include <commons/string.h>
#include "../include/team.h"
#include "../include/trainers.h"





// ============================================================================================================
//                               ***** Función parar leer las configuraciones*****
// ============================================================================================================

t_config* get_config()
{
    t_config* ret =  config_create("../team.config");
    if (ret == NULL)
    log_info (internalLogTeam, "No se pudieron leer las configuraciones");
    else log_info (internalLogTeam, "Configuraciones leidas correctamente");
    return ret;
}


// ============================================================================================================
//                               ***** Función parar inicializar a los entrenadores *****
// ============================================================================================================

void Team_load_trainers_config(void)
{
    /*  Sólo por estética */
    t_config *config_aux= config->team_config;

    /*  Devuelve un array de strings, donde cada posición del array es del tipo posX|posY */
    char ** pos_trainers_to_array = config_get_array_value (config_aux, "POSICIONES_ENTRENADORES");

    /*  Devuelven un array de strings, donde cada posición del array es del tipo pok1|pok2|pok3...pokN*/
    char ** pok_in_bag_to_array   = config_get_array_value (config_aux, "POKEMON_ENTRENADORES");
    char ** trainers_obj_to_array = config_get_array_value (config_aux, "OBJETIVOS_ENTRENADORES");

    /*  Creo lista de entrenadores que va a manejar el Team  */
    trainers = list_create();

    /*  Creo la lista de objetivos globales que va a manejar el Team */
    global_objective = list_create();
    pthread_mutex_init(&global_sem, NULL);

    aux_global_objective= list_create();
    pthread_mutex_init (&auxglobal_sem, NULL);
  
    new_global_objective=list_create();
    pthread_mutex_init(&new_global_sem, NULL);

    aux_new_global_objective=list_create();
    pthread_mutex_init(&aux_new_global_sem, NULL);

    t_list *bag_global= list_create();

    ID_localized = list_create();

    pthread_mutex_init(&ID_localized_sem, NULL);

    ID_caught= list_create();
    pthread_mutex_init(&ID_caught_sem, NULL);

    /*  Creo un puntero a estructura del tipo Trainer para ir creando cada entrenador leído por archivo de configuración.
        Estas estructuras se van apuntando en nodos de una lista definida en la estructura del Team como t_list *trainers     */
    Trainer *entrenadores;
    int index=0;

    /* -------------- Del archivo de configuración hacia lista de entrenadores --------------------- */

    for (int i=0; *(pos_trainers_to_array + i) !=NULL ; i++)
    {
        entrenadores=malloc (sizeof(Trainer));

        char **posicion = string_split(*(pos_trainers_to_array + i), "|");
        entrenadores->posx=atoi(*posicion) ;
        entrenadores->posy=atoi(*(posicion +1) );
        entrenadores->actual_status= NEW;
        free (*(posicion));
        free (*(posicion+1));

        entrenadores->bag = list_create();
        
        if (i ==0)
        {//puts ("if");
        //printf ("%s\n",*(pok_in_bag_to_array + i ));
           // puts (*(pok_in_bag_to_array + i ));
              list_add(entrenadores->bag, *(pok_in_bag_to_array ));
        }
      
        char **objetivos = string_split(*(trainers_obj_to_array + i), "|" );
        entrenadores->personal_objective = list_create();
        for (int k=0 ; *(objetivos + k) != NULL ; k++)
        list_add (entrenadores->personal_objective, *(objetivos+k));

        sem_init (&(entrenadores->trainer_sem), 0, 0);
        sem_post (&trainer_count);

        entrenadores->config = config;
        entrenadores->index= index;
        entrenadores->rafagaEjecutada=0;
        entrenadores->rafagaEstimada=config->initial_estimation;
        entrenadores->rafagaRemanente=0;
        
        index++;
        /*  Añado el entrenador creado, ya cada uno con su lista bag y lista de objetivos, a la lista de entrenadores */
        list_add(trainers, entrenadores);

        /*  Añado los objetivos de cada entrenador a la lista de objetivos globales */
        list_add_all (global_objective, list_duplicate(entrenadores->personal_objective)); //Ver si se puede poner un list_duplicate. Ya se cambió. Antes estaba duplicar_lista
        list_add_all (bag_global,entrenadores->bag);
        
        /*  Libero memoria innecesaria generada por la función string_split de las commons  */
        free (posicion);
        //free (mochila);
        free (objetivos);
    }

    remover_objetivos_globales_conseguidos(bag_global);
    list_destroy(bag_global);
    new_global_objective = list_duplicate(global_objective);

    /*  Libero memoria innecesaria generada por la función string_split de las commons  */
   /* free_split (pos_trainers_to_array);
    free_split (pok_in_bag_to_array);
    free_split (trainers_obj_to_array);
*/
    /* Sólo para testear la correcta elctura desde el archivo de configuraci{on hacia las listas */
    if (PRINT_TEST == 1) //Ver DEFINE en archivo team.h
    list_iterate(trainers, _imprimir_lista);

    config_destroy (config_aux);
}

// ============================================================================================================
//                               ***** Función parar cargar las configuraciones globales *****
// ============================================================================================================

void Team_load_global_config()
{
    ciclos_cpu=0;
    config->reconnection_time    = config_get_int_value(config->team_config, "TIEMPO_RECONEXION");
    config->retardo_cpu          = config_get_int_value(config->team_config, "RETARDO_CICLO_CPU");
    config->planning_algorithm   = string_duplicate(config_get_string_value(config->team_config, "ALGORITMO_PLANIFICACION"));
    config->quantum              = config_get_int_value(config->team_config, "QUANTUM");
    config->alpha                = config_get_double_value(config->team_config, "ALPHA");
    config->initial_estimation   = config_get_int_value(config->team_config, "ESTIMACION_INICIAL");
    config->broker_IP            = string_duplicate (config_get_string_value(config->team_config, "IP_BROKER"));
    config->broker_port          = string_duplicate (config_get_string_value(config->team_config, "PUERTO_BROKER"));
    config->team_IP              = string_duplicate (config_get_string_value(config->team_config, "IP_TEAM"));
    config->team_port            = string_duplicate (config_get_string_value(config->team_config, "PUERTO_TEAM"));

    if (!strcmp (config->planning_algorithm, "FIFO"))
    algoritmo=FIFO;
    else if (!strcmp (config->planning_algorithm, "RR"))
    algoritmo=RR;
    else if (!strcmp (config->planning_algorithm, "SJF-SD"))
    algoritmo=SJFSD;
    else if (!strcmp (config->planning_algorithm, "SJF-CD"))
    algoritmo=SJFCD;
    else exit (BAD_SCHEDULING_ALGORITHM);

    /* Just to test the correct reading from the configurations file to the globals configurations*/
    if (PRINT_TEST == 1)
    {
        puts ("\n\n\nShowing the global configurations:");
        printf ("TIEMPO_RECONEXION= %d\n",config->reconnection_time );
        printf ("RETARDO_CICLO_CPU= %d\n",config->retardo_cpu);
        printf ("ALGORITMO_PLANIFICACION= %s\n",config->planning_algorithm);
        printf ("QUANTUM= %d\n",config->quantum);
        printf ("ESTIMACION_INICIAL= %d\n",config->initial_estimation);
        printf ("IP_BROKER= %s\n", config->broker_IP);
        printf ("PUERTO_BROKER= %s\n", config->broker_port);
        printf ("IP_TEAM= %s\n", config->team_IP);
        printf ("PUERTO_TEAM= %s\n", config->team_port);
    }


}





// ============================================================================================================
//                               ***** Funciones parar liberar listas *****
// ============================================================================================================

void liberar_listas ()
{
    list_destroy_and_destroy_elements (trainers, _free_sub_list);
    list_destroy (global_objective);
}


void free_split (char **string)
{
    for (int i=0; *(string + i) != NULL ; i++)
    free (*(string+i));
    free (string);
}

void _free_sub_list (void* elemento)
{
    list_destroy_and_destroy_elements ( ( (Trainer *) elemento)->personal_objective, free);
    list_destroy_and_destroy_elements ( ( (Trainer *) elemento)->bag, free);
    free (elemento);
}


// ============================================================================================================
//                               ***** Funciones para imprimir listas (sólo para testear) *****
// ============================================================================================================


void _imprimir_lista (void *elemento)
{
    t_list *mochila= ( (Trainer *) elemento)->bag;
    list_iterate (mochila,_imprimir_inventario);
    t_list *objetivos = ( (Trainer *) elemento)->personal_objective;
    list_iterate (objetivos,_imprimir_objetivos);
}


void _imprimir_inventario (void *elemento)
{
    printf ("Atrapados: %s\n",(char *)elemento);
}


void _imprimir_objetivos (void *elemento)
{
    printf ("Objetivos: %s\n",(char *) elemento);
}


void remover_objetivos_globales_conseguidos(t_list *global_bag)
{
    char *nombre;

    bool comparar (void *element)
    {
        if (!strcmp( (char *)element, nombre))
        return true; else return false;
    }


    bool comparar_y_borrar (void *element)
    {
        for (int i=0; i<global_bag->elements_count; i++)
        {
            nombre=list_get(global_bag,i);
            if (!strcmp( (char *)element, nombre ) )
            {
            list_remove_by_condition(global_bag,comparar);
            return true;
            }
        }
        return false;
    }

    for (int i=0; i<global_objective->elements_count; i++)
    {
    list_remove_by_condition(global_objective,comparar_y_borrar);
    }
}