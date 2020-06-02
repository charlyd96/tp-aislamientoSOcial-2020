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

void liberar_listas (Team *this_team);


// ============================================================================================================
//                               ***** Función parar leer las configuraciones*****
// ============================================================================================================

t_config* get_config()
{
    t_config* ret =  config_create("team.config");
    if (ret == NULL)
    puts("[ERROR] CONFIG: FILE NOT OPENED");
    else puts("[DEBUG] Config loaded");
    return ret;
}


// ============================================================================================================
//                               ***** Función parar inicializar a los entrenadores *****
// ============================================================================================================

void Team_load_trainers_config(Team *this_team)
{
    /*  Sólo por estética */
    t_config *config= this_team->config->team_config;

    /*  Devuelve un array de strings, donde cada posición del array es del tipo posX|posY */
    char ** pos_trainers_to_array = config_get_array_value (config, "POSICIONES_ENTRENADORES");

    /*  Devuelven un array de strings, donde cada posición del array es del tipo pok1|pok2|pok3...pokN*/
    char ** pok_in_bag_to_array   = config_get_array_value (config, "POKEMON_ENTRENADORES");
    char ** trainers_obj_to_array = config_get_array_value (config, "OBJETIVOS_ENTRENADORES");

    /*  Creo lista de entrenadores que va a manejar el Team  */
    this_team->trainers = list_create();

    /*  Creo la lista de objetivos globales que va a manejar el Team */
    this_team->global_objective = list_create();

    /*  Creo un puntero a estructura del tipo Trainer para ir creando cada entrenador leído por archivo de configuración.
        Estas estructuras se van apuntando en nodos de una lista definida en la estructura del Team como t_list *trainers     */
    Trainer *entrenadores;


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

        char **mochila = string_split(*(pok_in_bag_to_array + i), "|");
        entrenadores->bag = list_create();
        for (int j=0; *(mochila + j) != NULL ; j++)
        list_add(entrenadores->bag, *(mochila+j) );


        char **objetivos = string_split(*(trainers_obj_to_array + i), "|" );
        entrenadores->personal_objective = list_create();
        for (int k=0 ; *(objetivos + k) != NULL ; k++)
        list_add (entrenadores->personal_objective, *(objetivos+k));

        sem_init (&(entrenadores->t_sem), 0, 0);

        /*  Añado el entrenador creado, ya cada uno con su lista bag y lista de objetivos, a la lista de entrenadores */
        list_add(this_team->trainers, entrenadores);

        /*  Añado los objetivos de cada entrenador a la lista de objetivos globales */
        list_add_all (this_team->global_objective, entrenadores->personal_objective);

        /*  Libero memoria innecesaria generada por la función string_split de las commons  */
        free (posicion);
        free (mochila);
        free (objetivos);
    }

    /*  Libero memoria innecesaria generada por la función string_split de las commons  */
    free_split (pos_trainers_to_array);
    free_split (pok_in_bag_to_array);
    free_split (trainers_obj_to_array);

    /* Sólo para testear la correcta elctura desde el archivo de configuraci{on hacia las listas */
    if (PRINT_TEST == 1) //Ver DEFINE en archivo team.h
    list_iterate(this_team->trainers, _imprimir_lista);

}

// ============================================================================================================
//                               ***** Función parar cargar las configuraciones globales *****
// ============================================================================================================

void Team_load_global_config(Config *config)
{
    config->reconnection_time      = config_get_int_value(config->team_config, "TIEMPO_RECONEXION");
    config->cpu_cycle              = config_get_int_value(config->team_config, "RETARDO_CICLO_CPU");
    config->planning_algorithm     = string_duplicate(config_get_string_value(config->team_config, "ALGORITMO_PLANIFICACION"));
    config->quantum                = config_get_int_value(config->team_config, "QUANTUM");
    config->initial_estimation     = config_get_int_value(config->team_config, "ESTIMACION_INICIAL");
    config->broker_IP              = string_duplicate (config_get_string_value(config->team_config, "IP_BROKER"));
    config->broker_port            = string_duplicate (config_get_string_value(config->team_config, "PUERTO_BROKER"));

    /* Just to test the correct reading from the configurations file to the globals configurations*/
    if (PRINT_TEST == 1)
    {
        puts ("\n\n\nShowing the global configurations:");
        printf ("TIEMPO_RECONEXION= %d\n",config->reconnection_time );
        printf ("RETARDO_CICLO_CPU= %d\n",config->cpu_cycle);
        printf ("ALGORITMO_PLANIFICACION= %s\n",config->planning_algorithm);
        printf ("QUANTUM= %d\n",config->quantum);
        printf ("ESTIMACION_INICIAL= %d\n",config->initial_estimation);
        printf ("IP_BROKER= %s\n", config->broker_IP);
        printf ("PUERTO_BROKER= %s\n", config->broker_port);
    }

    config_destroy (config->team_config);
}





// ============================================================================================================
//                               ***** Funciones parar liberar listas *****
// ============================================================================================================

void liberar_listas (Team *this_team)
{
    list_destroy_and_destroy_elements (this_team->trainers, _free_sub_list);
    list_destroy (this_team->global_objective);
    free (this_team);
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
