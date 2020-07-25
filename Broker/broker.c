/*
 * broker.c
 *
 *  Created on: 24 abr. 2020
 *      Author: aislamientoSOcial
 */

#include "broker.h"
void destruir_particion(void* elem){
	t_particion* part = elem;
	list_destroy_and_destroy_elements(part->susc_enviados,free);
	free(elem);
}
void clean_particion(void* elem){
	t_particion* part = elem;
	list_clean(part->susc_enviados);

}
/* FUNCIONES - INICIALIZACIÓN */

int crearConfigBroker(){
	log_info(logBrokerInterno, "Se inicializó el Log.");

    if (!existeArchivoConfig(pathConfigBroker)) {
		log_error(logBrokerInterno, "ERROR: Verificar path del archivo.");
	    return -1;
	}

	config_broker = malloc(sizeof(t_configuracion));
	config_ruta = config_create(pathConfigBroker);

	if (config_ruta != NULL){
	    config_broker->tam_memoria = config_get_int_value(config_ruta, "TAMANO_MEMORIA");
		config_broker->tam_minimo_particion = config_get_int_value(config_ruta, "TAMANO_MINIMO_PARTICION");
	    config_broker->ip_broker = config_get_string_value(config_ruta, "IP_BROKER");
	    config_broker->puerto_broker = config_get_string_value(config_ruta, "PUERTO_BROKER");
	    config_broker->frecuencia_compatacion = config_get_int_value(config_ruta, "FRECUENCIA_COMPACTACION");
	    config_broker->log_file = config_get_string_value(config_ruta, "LOG_FILE");

	    algoritmo_mem = config_get_string_value(config_ruta, "ALGORITMO_MEMORIA");
		if(strcmp(algoritmo_mem,"PARTICIONES") == 0) config_broker->algoritmo_memoria = PARTICIONES;
		if(strcmp(algoritmo_mem,"BS") == 0) config_broker->algoritmo_memoria = BS;

		algoritmo_reemplazo = config_get_string_value(config_ruta, "ALGORITMO_REEMPLAZO");
		if(strcmp(algoritmo_reemplazo,"FIFO") == 0) config_broker->algoritmo_reemplazo = FIFO;
		if(strcmp(algoritmo_reemplazo,"LRU") == 0) config_broker->algoritmo_reemplazo = LRU;

		algoritmo_libre = config_get_string_value(config_ruta, "ALGORITMO_PARTICION_LIBRE");
	    if(strcmp(algoritmo_libre,"FF") == 0) config_broker->algoritmo_particion_libre = FF;
	    if(strcmp(algoritmo_libre,"BF") == 0) config_broker->algoritmo_particion_libre = BF;
	}

	if(config_broker == NULL){
	    log_error(logBrokerInterno, "ERROR: No se pudo levantar el archivo de configuración.");
	    return -1;
	}
	return 0;
}

bool existeArchivoConfig(char* path){
	FILE * file = fopen(path, "r");
	if(file!=NULL){
		fclose(file);
		return true;
	} else{
		return false;
	}
}

void inicializarColas(){
	crearConfigBroker();

	cola_new = malloc(sizeof(t_cola));
	cola_appeared = malloc(sizeof(t_cola));
	cola_get = malloc(sizeof(t_cola));
	cola_localized = malloc(sizeof(t_cola));
	cola_catch = malloc(sizeof(t_cola));
	cola_caught = malloc(sizeof(t_cola));

	cola_new->nodos = list_create();;
	cola_new->suscriptores = list_create();

	cola_appeared->nodos = list_create();
	cola_appeared->suscriptores = list_create();

	cola_get->nodos = list_create();
	cola_get->suscriptores = list_create();

	cola_localized->nodos = list_create();
	cola_localized->suscriptores = list_create();

	cola_catch->nodos = list_create();
	cola_catch->suscriptores = list_create();

	cola_caught->nodos = list_create();
	cola_caught->suscriptores = list_create();
}

void inicializarSemaforos(){
	pthread_mutex_init(&sem_cola_new, NULL);
	pthread_mutex_init(&sem_cola_appeared, NULL);
	pthread_mutex_init(&sem_cola_catch, NULL);
	pthread_mutex_init(&sem_cola_caught, NULL);
	pthread_mutex_init(&sem_cola_get, NULL);
	pthread_mutex_init(&sem_cola_localized, NULL);

	pthread_mutex_init(&sem_nodo_new, NULL);
	pthread_mutex_init(&sem_nodo_appeared, NULL);
	pthread_mutex_init(&sem_nodo_catch, NULL);
	pthread_mutex_init(&sem_nodo_caught, NULL);
	pthread_mutex_init(&sem_nodo_localized, NULL);

	sem_init(&mensajes_new, 0, 0);
	sem_init(&mensajes_appeared, 0, 0);
	sem_init(&mensajes_catch, 0, 0);
	sem_init(&mensajes_caught, 0, 0);
	sem_init(&mensajes_get, 0, 0);
	sem_init(&mensajes_localized, 0, 0);

	sem_init(&identificador, 0, (unsigned int) 1);
}

void inicializarMemoria(){
	cache = malloc(config_broker->tam_memoria);
	particiones = list_create();

	//Partición libre inicial
	t_particion* particionInicial = malloc(sizeof(t_particion));

	if(config_broker->algoritmo_memoria == BS){
		//Exponente en base 2 que representa a la memoria (para buddy)
		buddy_U = (uint32_t)ceil(log10(config_broker->tam_memoria)/log10(2));
		log_info(logBrokerInterno,"EL U es %d",buddy_U);
		particionInicial->buddy_i = buddy_U;
	}
	particionInicial->libre = true;
	particionInicial->base = 0;
	particionInicial->tamanio = config_broker->tam_memoria;
	particionInicial->susc_enviados = list_create();
	list_add(particiones,particionInicial);

	//free(particionInicial);
}

void liberarColas(){

}

/* FUNCIONES - MEMORIA */

int buscarParticionLibre(uint32_t largo_stream){
	int i = 0;
	int largo_list = list_size(particiones);
	t_algoritmo_particion_libre algoritmo = config_broker->algoritmo_particion_libre;
	bool encontrado = false;
	if (algoritmo == FF) {
		log_info(logBrokerInterno,"Buscar partición libre FF");
		while (encontrado == false && i < largo_list) {
			t_particion* part = list_get(particiones, i);
			if ((part->libre) == true && largo_stream <= (part->tamanio)) {
				encontrado = true;
			} else {
				i++;
			}
		}
		//Si salió por encontrado
		if (encontrado) {
			return i;
		}
		//Si salió por i, no encontró ninguna que pueda contener el largo_stream
		return -1;

	} else if (algoritmo == BF) {
		log_info(logBrokerInterno, "Algoritmo Best Fit.");
		//Inicializo dif en tamaño max memoria, así el primer candidato será válido
		uint32_t diferenciaActual = config_broker->tam_memoria;
		int indiceCandidato = -1;
		for (i = 0; i < largo_list; i++) {
			t_particion* part = list_get(particiones, i);
			if ((part->libre) == true && (part->tamanio) >= largo_stream) {
				//Candidato, si es mejor que el anterior lo reservo
				if (diferenciaActual > (part->tamanio) - largo_stream) {
					diferenciaActual = (part->tamanio) - largo_stream;
					indiceCandidato = i;
				}
			}
		}
		return indiceCandidato;
	}
	//Si no matcheo ningun algoritmo
	return -1;
}
/**
 * Busca una partición libre y de tamaño indicado
 */

int buscarHuecoBuddy(uint32_t i){
	uint32_t largo = (uint32_t)list_size(particiones);
	for(uint32_t x = 0; x < largo; x++){
		t_particion* part = list_get(particiones,x);
		//log_info(logBrokerInterno,"analizo part base %d, i %d, indice %d,libre %d",part->base,part->buddy_i,x,part->libre);
		if(part->libre == true && part->buddy_i == i){
			return x;
		}
	}
	return -1;
}
/**
 * Parte a la mitad el buddy del índice dado
 */
void partirBuddy(int indice){
	int i = 0;
	t_particion* buddy_izq = list_get(particiones,indice);
	//limpio lista de enviados
	list_clean(buddy_izq->susc_enviados);
	t_particion* buddy_der = malloc(sizeof(t_particion));
	buddy_der->base = buddy_izq->base;
	buddy_der->buddy_i = buddy_izq->buddy_i;
	buddy_der->susc_enviados = list_create();
	buddy_der->libre = buddy_izq->libre;
	buddy_der->tamanio = buddy_izq->tamanio;

	//El orden (exponente en base 2) actual del buddy
	i = buddy_izq->buddy_i;
	//log_info(logBrokerInterno,"Se parte el buddy indice %d base %d i %d",indice,buddy_izq->base,buddy_izq->buddy_i);
	if(i > 0){
		//El buddy de la derecha comienza 2^(i-1) bytes después de la base del izquierdo
		buddy_der->buddy_i = i - 1;
		buddy_der->base = buddy_der->base + (int)pow(2,i-1);

		buddy_izq->buddy_i = i - 1;

		list_replace(particiones,indice,buddy_der);
		list_add_in_index(particiones,indice,buddy_izq);
	}
	//free(buddy_der);
}
/**
 * algoritmo recursivo para buscar una partición buddy adecuada.
 * se van partiendo a la mitad los buddys de ser necesario
 */
int obtenerHuecoBuddy(uint32_t i){
	int indice;

	//Condición de salida de la recursividad
	if(i == (buddy_U + 1)){
		return -1;
	}

	indice = buscarHuecoBuddy(i);
	if(indice == -1){
		indice = obtenerHuecoBuddy(i + 1);
		//Si encontró parto el buddy, sino no hago nada
		if(indice != -1){
			partirBuddy(indice);
		}
	}
	return indice;
}
/**
 * el campo "id" de la partición depende del tipo de mensaje puede referirse al id_mensaje o id_mensaje_correlativo
 * Esta funcion es para reutilizar en cada cachearTalMensaje()
 */
void buscarParticionYAlocar(int largo_stream,void* stream,op_code tipo_msg,uint32_t id){
	sem_wait(&mx_particiones);
	int indice = -1;

	if(config_broker->algoritmo_memoria == PARTICIONES){
		//buscarParticionLibre(largo_stream) devuelve el índice de la particion libre o -1 si no encuentra
		indice = buscarParticionLibre(largo_stream);
		int cant_intentos_fallidos = 0;
		log_info(logBrokerInterno,"indice %d",indice);
		while (indice < 0) {
			cant_intentos_fallidos++;
			if(cant_intentos_fallidos < config_broker->frecuencia_compatacion){
				eliminarParticion();
			}else{
				// 8. Ejecución de compactación (para particiones dinámicas)
				log_info(logBroker, "Ejecución de Compactación.");
				log_info(logBrokerInterno,"Ejecución de Compactación.");
				compactarParticiones();
				cant_intentos_fallidos = 0;

				indice = buscarParticionLibre(largo_stream);
				if(indice < 0){
					cant_intentos_fallidos++;
					eliminarParticion();
				}
			}
			//Buscar de nuevo
			indice = buscarParticionLibre(largo_stream);
		}
	}
	if(config_broker->algoritmo_memoria == BS){
		//calculo la potencia de 2 mínima para contener el stream
		int i = (int)ceil(log10(largo_stream)/log10(2));
		//log_info(logBrokerInterno,"el i es %d",i);
		//Se busca recursivamente una partición de orden i
		indice = obtenerHuecoBuddy(i);
		while(indice == -1){
			//Si el indice es -1, no encontró partición, aplico algoritmo de reemplazo
			eliminarParticionBuddy();
			indice = obtenerHuecoBuddy(i);
		}
	}

	//Copiar estr. admin. de la particion
	t_particion* part_libre = list_get(particiones, indice);

	//alocar: (cache es un puntero void* al inicio de la caché)
	//cache + base de la part_libre es el puntero al inicio de la partición libre
	memcpy(cache + (part_libre->base), stream, largo_stream);

	//actualizar estructura administrativa (lista de particiones)
	//Esto depende del tipo de algoritmo
	if(config_broker->algoritmo_memoria == PARTICIONES){
		t_particion* part_nueva = malloc(sizeof(t_particion));
		part_nueva->libre = false;
		part_nueva->tipo_mensaje = tipo_msg;
		part_nueva->base = part_libre->base;
		part_nueva->tamanio = largo_stream;
		part_nueva->id = id;
		struct timeval current_time;
		gettimeofday(&current_time, NULL);
		part_nueva->time_creacion = current_time; //Hora actual del sistema
		part_nueva->time_ultima_ref = current_time; //Hora actual del sistema
		part_nueva->susc_enviados = list_create();
		// list_clean(part_nueva->susc_enviados);

		part_libre->base = part_libre->base + largo_stream;
		part_libre->tamanio = part_libre->tamanio - largo_stream;

		//Y acá cambiar [{...part_libre...}] por [{nueva},{libre_mas_chica}]
		//Pasos: Insertar la nueva -> [{nueva},{...part_libre...}]
		list_add_in_index(particiones,indice,part_nueva);
		//y luego eliminar/reemplazar la part_libre original
		if(part_libre->tamanio == 0){
			list_remove_and_destroy_element(particiones,indice+1,destruir_particion);
		}else{
			list_replace(particiones,indice+1,part_libre);
		}

		char* cola = colaParaLogs(part_nueva->tipo_mensaje);

		// 6. Almacenado de un mensaje dentro de la memoria (indicando posición de inicio de su partición).
		log_info(logBroker, "Se almacena el Mensaje %s en la Partición con posición de inicio %d.", cola, part_nueva->base, cache + part_nueva->base);
		//log_info(logBrokerInterno, "Se almacena el Mensaje %s en la Partición con posición de inicio %d.", cola, part_nueva->base, part_nueva->base);

		log_info(logBrokerInterno, "ID_MENSAJE %d, asigno partición base %d y tamanio %d",id, part_nueva->base,part_nueva->tamanio);
	}
	if(config_broker->algoritmo_memoria == BS){

		part_libre->libre = false;
		part_libre->tipo_mensaje = tipo_msg;
		part_libre->id = id;
		struct timeval current_time;
		gettimeofday(&current_time, NULL);
		part_libre->time_creacion = current_time; //Hora actual del sistema
		part_libre->time_ultima_ref = current_time; //Hora actual del sistema
		part_libre->tamanio = (uint32_t)pow(2,part_libre->buddy_i);
		// part_libre->susc_enviados = list_create();
		list_clean(part_libre->susc_enviados);

		list_replace(particiones,indice,part_libre);
		log_debug(logBrokerInterno, "ID_MENSAJE %d, asigno partición base %d, i %d, tamanio %d",id, part_libre->base,part_libre->buddy_i,part_libre->tamanio);
			
		char* cola = colaParaLogs(part_libre->tipo_mensaje);

		// 6. Almacenado de un mensaje dentro de la memoria (indicando posición de inicio de su partición).
		log_info(logBroker, "Se almacena el Mensaje %s en la Partición con posición de inicio %d.", cola, part_libre->base, cache + part_libre->base);
		//log_info(logBrokerInterno, "Se almacena el Mensaje %s en la Partición con posición de inicio %d.", cola, part_libre->base, part_libre->base);
	}
	//-> DESMUTEAR LISTA DE PARTICIONES
	sem_post(&mx_particiones);
}

void liberarParticion(int indice_victima){
	t_particion* part_liberar = list_get(particiones,indice_victima);
	part_liberar->libre = true;
	list_clean(part_liberar->susc_enviados);

	list_replace(particiones, indice_victima, part_liberar);

	// 7. Eliminado de una partición de memoria (indicado la posición de inicio de la misma).
	log_info(logBroker, "Se elimina la Partición con posición de inicio %d.", part_liberar->base, cache+part_liberar->base);
	log_info(logBrokerInterno, "Se elimina la Partición con posición de inicio %d.", part_liberar->base, cache+part_liberar->base);
	//Consolidar

	//Si no es la última partición
	if(indice_victima + 1 < list_size(particiones)){
		t_particion* part_der = list_get(particiones,indice_victima+1);
		if(part_der->libre == true){
			//Fusionar
			part_liberar->tamanio = part_liberar->tamanio + part_der->tamanio;
			// list_clean(part_der->susc_enviados);
			list_remove_and_destroy_element(particiones,indice_victima + 1,destruir_particion);
			log_info(logBrokerInterno,"Se consolida con particion indice %d",indice_victima+1);
		}
	}
	//Si no es la primera partición
	if(indice_victima > 0){
		t_particion* part_izq = list_get(particiones,indice_victima-1);
		if(part_izq->libre == true){
			//Fusionar
			part_liberar->base = part_izq->base;
			part_liberar->tamanio = part_liberar->tamanio + part_izq->tamanio;
			// list_clean(part_izq->susc_enviados);
			list_remove_and_destroy_element(particiones,indice_victima -1,destruir_particion);
			log_info(logBrokerInterno,"Se consolida con particion indice %d",indice_victima-1);
		}
	}
}

void eliminarParticionBuddy(){
	log_info(logBrokerInterno,"Eliminar particion buddy");
	int indice_victima = -1;
	t_algoritmo_reemplazo algoritmo = config_broker->algoritmo_reemplazo;
	switch(algoritmo){
		case FIFO:{
			//Eliminar la partición con con time_creación más viejo
			indice_victima = victimaSegunFIFO();
			break;
		}
		
		case LRU:{
			indice_victima = victimaSegunLRU();
			break;
		}
		
		default:
		{
			log_info(logBrokerInterno,"No matcheo reemplazo");
			return;
			break;
		}
	}
	//Eliminar partición. Si tiene un compañero (buddy) del mismo tamaño y vacío, consolidar
	t_particion* part_liberar = list_get(particiones,indice_victima);
	part_liberar->libre = true;
	list_clean(part_liberar->susc_enviados);
	uint32_t i = part_liberar->buddy_i;
	list_replace(particiones, indice_victima, part_liberar);

	// 7. Eliminado de una partición de memoria (indicado la posición de inicio de la misma).
	log_info(logBroker, "Se elimina la Partición con posición de inicio %d.", part_liberar->base, cache+part_liberar->base);
	log_info(logBrokerInterno, "Se elimina la Partición con posición de inicio %d.", part_liberar->base, cache+part_liberar->base);

	//Consolidar buddys
	bool huboConsolidacion;
	uint32_t i_aux = i;
	do{
		huboConsolidacion = false;
		//Si no es la última partición
		if(indice_victima + 1 < list_size(particiones)){
			t_particion* part_der = list_get(particiones,indice_victima+1);
			if(part_der->libre == true && part_der->buddy_i == i_aux){
				//Fusionar
				// 8. Asociación de bloques (para buddy system), indicar que particiones se asociaron (indicar posición inicio de ambas particiones).
				log_info(logBroker, "Asociación de Particiones: Partición %d con Partición %d.", part_liberar->base, part_der->base);
				log_info(logBrokerInterno,"Se consolida a derecha con particion buddy indice %d, i %d",indice_victima+1,i_aux);
				part_liberar->buddy_i = part_liberar->buddy_i + 1;
				// list_clean(part_der->susc_enviados);
				list_remove_and_destroy_element(particiones,indice_victima + 1,destruir_particion);
				huboConsolidacion = true;
				i_aux++;
			}
		}
		//Si no es la primera partición
		if(indice_victima > 0){
			t_particion* part_izq = list_get(particiones,indice_victima-1);
			if(part_izq->libre == true && part_izq->buddy_i == i_aux){
				//Fusionar
				// 8. Asociación de bloques (para buddy system), indicar que particiones se asociaron (indicar posición inicio de ambas particiones).
				log_info(logBroker, "Asociación de Particiones: Partición %d con Partición %d.", part_liberar->base, part_izq->base);
				log_info(logBrokerInterno,"Se consolida a izquierda con particion indice %d",indice_victima-1);
				part_liberar->base = part_izq->base;
				part_liberar->buddy_i = part_liberar->buddy_i + 1;
				// list_clean(part_izq->susc_enviados);
				list_remove_and_destroy_element(particiones,indice_victima -1,destruir_particion);
				huboConsolidacion = true;
				indice_victima = indice_victima -1;
				i_aux++;
			}
		}
	}while(huboConsolidacion);
}
void eliminarParticion(){
	t_algoritmo_reemplazo algoritmo = config_broker->algoritmo_reemplazo;
	int indice_victima = 0;

	switch(algoritmo){
		case FIFO:{
			//Eliminar la partición con con time_creación más viejo
			indice_victima = victimaSegunFIFO();
			break;
		}
		
		case LRU:{
			indice_victima = victimaSegunLRU();
			break;
		}
		
		default:{
			log_info(logBrokerInterno,"No matcheo reemplazo");
			break;
		}
	}
	//Liberar la partición víctima
	liberarParticion(indice_victima);
}

int victimaSegunFIFO(){
	log_info(logBrokerInterno, "Algoritmo FIFO.");

	int cant_particiones = list_size(particiones);
	struct timeval time_aux;
	gettimeofday(&time_aux, NULL);
	int indice_victima = -1;

	for(int i=0; i<cant_particiones; i++){
		t_particion* part = list_get(particiones,i);
		if(part->libre == false){
			//Si los segundos son menores ó los segundos son iguales y los microsegundos menores
			if(part->time_creacion.tv_sec < time_aux.tv_sec || (part->time_creacion.tv_sec == time_aux.tv_sec && part->time_creacion.tv_usec < time_aux.tv_usec)){
				time_aux = part->time_creacion;
				indice_victima = i;
			}
		}
	}
	return indice_victima;
}

int victimaSegunLRU(){
	log_info(logBrokerInterno, "Algoritmo LRU.");

	int indice_victima = -1;
	int cant_particiones = list_size(particiones);
	struct timeval time_aux;
	gettimeofday(&time_aux, NULL);

	for(int i = 0; i < cant_particiones; i++) {
		t_particion* part = list_get(particiones, i);
		if(part->libre == false){
			if(part->time_ultima_ref.tv_sec < time_aux.tv_sec || (part->time_ultima_ref.tv_sec == time_aux.tv_sec && part->time_ultima_ref.tv_usec < time_aux.tv_usec)){
				time_aux = part->time_ultima_ref;
				indice_victima = i;
			}
		}
	}

	return indice_victima;
}

/*	Yo lo que me imagino es recorrer la lista "particiones" e ir pusheando las particiones
	ocupadas en una lista auxiliar, mientas vas haciendo el memcpy para reubicar los datos,
	acumulando el tamaño de cada una en una variable y acomodando las bases. Al final pusheas una
	ultima partición libre de tamanio = MAX_MEMORIA - ACUMULADO_OCUPADO
	y la lista auxiliar es tu nueva lista "particiones"
*/
void compactarParticiones(){
	int cant_particiones = list_size(particiones);
	t_list* lista_aux = list_duplicate(particiones);
	list_clean_and_destroy_elements(particiones,clean_particion);

	uint32_t offset = 0;
	for(int i=0; i < cant_particiones; i++){
		t_particion* part = list_get(lista_aux, i);
		if(part->libre == false){
			memmove(cache+offset,cache+part->base,part->tamanio);
			part->base = offset;
			offset += part->tamanio;
			list_add(particiones,part);
		}else{
			destruir_particion(part);
		}
	}
	t_particion* espacio_libre = malloc(sizeof(t_particion));
	espacio_libre->libre   = true;
	espacio_libre->base    = offset;
	espacio_libre->tamanio = config_broker->tam_memoria - offset;
	espacio_libre->susc_enviados = list_create();
	list_add(particiones,espacio_libre);
	//Libero solo el puntero a la lista auxiliar, no sus elementos porque se los pasé a particiones
	list_destroy(lista_aux);
	// free(espacio_libre);
}

void cachearNewPokemon(t_new_pokemon* msg){
	uint32_t largo_nombre = strlen(msg->nombre_pokemon); //Sin el \0
	uint32_t largo_stream = 4*sizeof(uint32_t) + largo_nombre;

	if(largo_stream > config_broker->tam_memoria){
		log_error(logBrokerInterno, "ERROR: El tamaño del Mensaje supera el tamaño de la Memoria %d B.", config_broker->tam_memoria);
	}else{
		if(largo_stream < config_broker->tam_minimo_particion){
			largo_stream = config_broker->tam_minimo_particion;
		}

		void* stream = malloc(largo_stream);
		uint32_t offset = 0;
		memcpy(stream + offset, &largo_nombre, sizeof(uint32_t));
		offset += sizeof(uint32_t);
		memcpy(stream + offset, msg->nombre_pokemon, largo_nombre);	//Copio "pikachu" sin el \0
		offset += largo_nombre;
		memcpy(stream + offset, &(msg->pos_x), sizeof(uint32_t));
		offset += sizeof(uint32_t);
		memcpy(stream + offset, &(msg->pos_y), sizeof(uint32_t));
		offset += sizeof(uint32_t);
		memcpy(stream + offset, &(msg->cantidad), sizeof(uint32_t));
		offset += sizeof(uint32_t);

		buscarParticionYAlocar(largo_stream,stream,NEW_POKEMON,msg->id_mensaje);
		free(stream);
	}
}

void cachearAppearedPokemon(t_appeared_pokemon* msg){
	uint32_t largo_nombre = strlen(msg->nombre_pokemon); //Sin el \0
	uint32_t largo_stream = 3 * sizeof(uint32_t) + largo_nombre;

	if(largo_stream > config_broker->tam_memoria){
		log_error(logBrokerInterno, "ERROR: El tamaño del Mensaje supera el tamaño de la Memoria %d B.", config_broker->tam_memoria);
	}else{
		if(largo_stream < config_broker->tam_minimo_particion){
			largo_stream = config_broker->tam_minimo_particion;
		}

		void* stream = malloc(largo_stream);
		uint32_t offset = 0;
		memcpy(stream + offset, &largo_nombre, sizeof(uint32_t));
		offset += sizeof(uint32_t);
		memcpy(stream + offset, msg->nombre_pokemon, largo_nombre);	//Copio "pikachu" sin el \0
		offset += largo_nombre;
		memcpy(stream + offset, &(msg->pos_x), sizeof(uint32_t));
		offset += sizeof(uint32_t);
		memcpy(stream + offset, &(msg->pos_y), sizeof(uint32_t));
		offset += sizeof(uint32_t);

		buscarParticionYAlocar(largo_stream,stream,APPEARED_POKEMON,msg->id_mensaje_correlativo);
		free(stream);
	}
}

void cachearCatchPokemon(t_catch_pokemon* msg){
	uint32_t largo_nombre = strlen(msg->nombre_pokemon); //Sin el \0
	uint32_t largo_stream = 3 * sizeof(uint32_t) + largo_nombre;

	if(largo_stream > config_broker->tam_memoria){
		log_error(logBrokerInterno, "ERROR: El tamaño del Mensaje supera el tamaño de la Memoria %d B.", config_broker->tam_memoria);
	}else{
		if(largo_stream < config_broker->tam_minimo_particion){
			largo_stream = config_broker->tam_minimo_particion;
		}

		void* stream = malloc(largo_stream);
		uint32_t offset = 0;
		memcpy(stream + offset, &largo_nombre, sizeof(uint32_t));
		offset += sizeof(uint32_t);
		memcpy(stream + offset, msg->nombre_pokemon, largo_nombre);	//Copio "pikachu" sin el \0
		offset += largo_nombre;
		memcpy(stream + offset, &(msg->pos_x), sizeof(uint32_t));
		offset += sizeof(uint32_t);
		memcpy(stream + offset, &(msg->pos_y), sizeof(uint32_t));
		offset += sizeof(uint32_t);

		buscarParticionYAlocar(largo_stream,stream,CATCH_POKEMON,msg->id_mensaje);

		free(stream);
	}
}

void cachearCaughtPokemon(t_caught_pokemon* msg){
	uint32_t largo_stream = sizeof(uint32_t);

	if(largo_stream > config_broker->tam_memoria){
		log_error(logBrokerInterno, "ERROR: El tamaño del Mensaje supera el tamaño de la Memoria %d B.", config_broker->tam_memoria);
	}else{
		if(largo_stream < config_broker->tam_minimo_particion){
			largo_stream = config_broker->tam_minimo_particion;
		}

		void* stream = malloc(largo_stream);

		memcpy(stream, &(msg->atrapo_pokemon), sizeof(uint32_t));
		
		buscarParticionYAlocar(largo_stream,stream,CAUGHT_POKEMON,msg->id_mensaje_correlativo);

		free(stream);
	}
}

void cachearGetPokemon(t_get_pokemon* msg){
	uint32_t largo_nombre = strlen(msg->nombre_pokemon); //Sin el \0
	uint32_t largo_stream = sizeof(uint32_t) + largo_nombre;

	if(largo_stream > config_broker->tam_memoria){
		log_error(logBrokerInterno, "ERROR: El tamaño del Mensaje supera el tamaño de la Memoria %d B.", config_broker->tam_memoria);
	}else{
		if(largo_stream < config_broker->tam_minimo_particion){
			largo_stream = config_broker->tam_minimo_particion;
		}

		//Serializo el msg
		void* stream = malloc(largo_stream);
		uint32_t offset = 0;
		memcpy(stream + offset, &largo_nombre, sizeof(uint32_t));
		offset += sizeof(uint32_t);
		memcpy(stream + offset, msg->nombre_pokemon, largo_nombre);	//Copio "pikachu" sin el \0

		buscarParticionYAlocar(largo_stream,stream,GET_POKEMON,msg->id_mensaje);

		free(stream);
	}
}

void cachearLocalizedPokemon(t_localized_pokemon* msg){
	uint32_t largo_nombre = strlen(msg->nombre_pokemon); //Sin el \0
	uint32_t largo_stream = 2 * sizeof(uint32_t) + largo_nombre + 2* sizeof(uint32_t) * msg->cant_pos;

	if(largo_stream > config_broker->tam_memoria){
		log_error(logBrokerInterno, "ERROR: El tamaño del Mensaje supera el tamaño de la Memoria %d B.", config_broker->tam_memoria);
	}else{
		if(largo_stream < config_broker->tam_minimo_particion){
			largo_stream = config_broker->tam_minimo_particion;
		}

		void* stream = malloc(largo_stream);
		uint32_t offset = 0;
		memcpy(stream + offset, &largo_nombre, sizeof(uint32_t));
		offset += sizeof(uint32_t);
		memcpy(stream + offset, msg->nombre_pokemon, largo_nombre);	//Copio "pikachu" sin el \0
		offset += largo_nombre;
		memcpy(stream + offset, &(msg->cant_pos), sizeof(uint32_t));
		offset += sizeof(uint32_t);
		if(msg->cant_pos > 0){
			uint32_t pos_x, pos_y;
			char** pos_list = string_get_string_as_array(msg->posiciones);
			for(uint32_t i=0; i<(msg->cant_pos); i++){
				char** pos_pair = string_split(pos_list[i],"|");
				pos_x = atoi(pos_pair[0]);
				pos_y = atoi(pos_pair[1]);

				memcpy(stream + offset, &(pos_x), sizeof(uint32_t));
				offset += sizeof(uint32_t);
				memcpy(stream + offset, &(pos_y), sizeof(uint32_t));
				offset += sizeof(uint32_t);

				free_split(pos_pair);
			}
			free_split(pos_list);
		}

		buscarParticionYAlocar(largo_stream,stream,LOCALIZED_POKEMON,msg->id_mensaje_correlativo);

		free(stream);
	}
}

t_new_pokemon* descachearNewPokemon(void* stream, uint32_t id){
	t_new_pokemon * mensaje_a_enviar = malloc(sizeof(t_new_pokemon));
	uint32_t largo_nombre = 0;
	uint32_t offset = 0;

	memcpy(&largo_nombre, stream + offset, sizeof(uint32_t));
	offset += sizeof(uint32_t);

	mensaje_a_enviar->nombre_pokemon = malloc(largo_nombre + 1);
	*(mensaje_a_enviar->nombre_pokemon + largo_nombre) = '\0';

	memcpy(mensaje_a_enviar->nombre_pokemon, stream + offset, largo_nombre);
	offset += largo_nombre;
	memcpy(&(mensaje_a_enviar->pos_x), stream + offset, sizeof(uint32_t));
	offset += sizeof(uint32_t);
	memcpy(&(mensaje_a_enviar->pos_y), stream + offset, sizeof(uint32_t));
	offset += sizeof(uint32_t);
	memcpy(&(mensaje_a_enviar->cantidad), stream + offset, sizeof(uint32_t));
	offset += sizeof(uint32_t);

	mensaje_a_enviar->id_mensaje = id;

	return mensaje_a_enviar;
}

t_appeared_pokemon descachearAppearedPokemon(void* stream, uint32_t id){
	t_appeared_pokemon mensaje_a_enviar;
	uint32_t largo_nombre = 0;
	uint32_t offset = 0;

	memcpy(&largo_nombre, stream + offset, sizeof(uint32_t));
	offset += sizeof(uint32_t);

	mensaje_a_enviar.nombre_pokemon = malloc(largo_nombre + 1);
	*(mensaje_a_enviar.nombre_pokemon + largo_nombre) = '\0';

	memcpy(mensaje_a_enviar.nombre_pokemon, stream + offset, largo_nombre);
	offset += largo_nombre;
	memcpy(&(mensaje_a_enviar.pos_x), stream + offset, sizeof(uint32_t));
	offset += sizeof(uint32_t);
	memcpy(&(mensaje_a_enviar.pos_y), stream + offset, sizeof(uint32_t));
	offset += sizeof(uint32_t);

	mensaje_a_enviar.id_mensaje_correlativo = id;

	return mensaje_a_enviar;
}

t_catch_pokemon descachearCatchPokemon(void* stream, uint32_t id){
	t_catch_pokemon mensaje_a_enviar;
	uint32_t largo_nombre = 0;
	uint32_t offset = 0;

	memcpy(&largo_nombre, stream + offset, sizeof(uint32_t));
	offset += sizeof(uint32_t);

	mensaje_a_enviar.nombre_pokemon = malloc(largo_nombre + 1);
	*(mensaje_a_enviar.nombre_pokemon + largo_nombre) = '\0';

	memcpy(mensaje_a_enviar.nombre_pokemon, stream + offset, largo_nombre);
	offset += largo_nombre;
	memcpy(&(mensaje_a_enviar.pos_x), stream + offset, sizeof(uint32_t));
	offset += sizeof(uint32_t);
	memcpy(&(mensaje_a_enviar.pos_y), stream + offset, sizeof(uint32_t));
	offset += sizeof(uint32_t);

	mensaje_a_enviar.id_mensaje = id;

	return mensaje_a_enviar;
}

t_caught_pokemon descachearCaughtPokemon(void* stream, uint32_t id){
	t_caught_pokemon mensaje_a_enviar;
	uint32_t offset = 0;

	memcpy(&(mensaje_a_enviar.atrapo_pokemon), stream + offset, sizeof(uint32_t));
	offset += sizeof(uint32_t);

	mensaje_a_enviar.id_mensaje_correlativo = id;

	return mensaje_a_enviar;
}

t_get_pokemon* descachearGetPokemon(void* stream, uint32_t id){
	t_get_pokemon* mensaje_a_enviar = malloc(sizeof(t_get_pokemon));
	uint32_t largo_nombre = 0;
	uint32_t offset = 0;

	memcpy(&largo_nombre, stream + offset, sizeof(uint32_t));
	offset += sizeof(uint32_t);

	mensaje_a_enviar->nombre_pokemon = malloc(largo_nombre + 1);
	*(mensaje_a_enviar->nombre_pokemon + largo_nombre) = '\0';

	memcpy(mensaje_a_enviar->nombre_pokemon, stream + offset, largo_nombre);
	offset += largo_nombre;

	mensaje_a_enviar->id_mensaje = id;

	return mensaje_a_enviar;
}

t_localized_pokemon descachearLocalizedPokemon(void* stream, uint32_t id){
	t_localized_pokemon mensaje_a_enviar;
	uint32_t largo_nombre = 0;
	uint32_t offset = 0;
	uint32_t pos_x = 0;
	uint32_t pos_y = 0;
	char* posicionesString = string_new();
 
	memcpy(&largo_nombre, stream + offset, sizeof(uint32_t));
	offset += sizeof(uint32_t);

	mensaje_a_enviar.nombre_pokemon = malloc(largo_nombre + 1);
	*(mensaje_a_enviar.nombre_pokemon + largo_nombre) = '\0';

	memcpy(mensaje_a_enviar.nombre_pokemon, stream + offset, largo_nombre);
	offset += largo_nombre;
	memcpy(&(mensaje_a_enviar.cant_pos), stream + offset, sizeof(uint32_t));
	offset += sizeof(uint32_t);

	
	if (mensaje_a_enviar.cant_pos >0){
		string_append(&posicionesString,"[");
		for (uint32_t i = 1; i<=mensaje_a_enviar.cant_pos; i++){
			memcpy(&pos_x, stream + offset, sizeof (uint32_t));
			offset+=sizeof(uint32_t);
			memcpy(&pos_y, stream + offset, sizeof (uint32_t));
			offset+=sizeof(uint32_t);
			string_append(&posicionesString,string_itoa(pos_x));
			string_append(&posicionesString,"|");
			string_append(&posicionesString,string_itoa(pos_y));

			if(i < mensaje_a_enviar.cant_pos) string_append(&posicionesString,",");
		}
		string_append(&posicionesString,"]");
	}
	mensaje_a_enviar.posiciones=posicionesString;
	mensaje_a_enviar.id_mensaje_correlativo = id;

	return mensaje_a_enviar;
}

char* fecha_y_hora_actual(){
	time_t tiempo = time(0);
	struct tm *tlocal = localtime(&tiempo);
	char* output = malloc(128);
	strftime(output,128,"%d/%m/%y %H:%M:%S",tlocal);

	output[127] = '\0';

	return output;
}

void dump_cache(){
	int tam_lista = list_size(particiones);
	t_particion* particion_buscada;

	char* fecha_y_hora = fecha_y_hora_actual();

	FILE* archivo_dump;
	archivo_dump = fopen("archivo_dump.txt", "w+");

	fprintf(archivo_dump, "---------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------\n");
	fprintf(archivo_dump, "Dump: %s\n", fecha_y_hora);

	for (int i = 0; i < tam_lista ; i++){
		
		particion_buscada = list_get(particiones, i);

		char* cola = colaParaLogs(particion_buscada->tipo_mensaje);

		switch(config_broker->algoritmo_memoria){
			case PARTICIONES:{
				if(particion_buscada->libre == 0){
					fprintf(archivo_dump, "Partición %d: %p - %p.\t\t\t[X]\t\t\tSize: %db\t\t\tLRU: %ld.%06ld\t\t\tCola: %s\t\tID: %d\n", i, (cache + particion_buscada->base), (cache + particion_buscada->base + particion_buscada->tamanio), particion_buscada->tamanio, particion_buscada->time_ultima_ref.tv_sec, particion_buscada->time_ultima_ref.tv_usec, cola, particion_buscada->id);
				}else{
					fprintf(archivo_dump, "Partición %d: %p - %p.\t\t\t[L]\t\t\tSize: %db\n", i, (cache + particion_buscada->base), (cache + particion_buscada->base + particion_buscada->tamanio), particion_buscada->tamanio);
				}
				break;
			}
			case BS:{
				if(particion_buscada->libre == 0){
					fprintf(archivo_dump, "Partición %d: %p - %p.\t\t\t[X]\t\t\tSize: %db\t\t\tLRU: %ld.%06ld\t\t\tCola: %s\t\tID: %d\n", i, (cache + particion_buscada->base), (cache + particion_buscada->base + particion_buscada->tamanio), (int)pow(2, particion_buscada->buddy_i), particion_buscada->time_ultima_ref.tv_sec, particion_buscada->time_ultima_ref.tv_usec, cola, particion_buscada->id);
				}else{
					fprintf(archivo_dump, "Partición %d: %p - %p.\t\t\t[L]\t\t\tSize: %db\n", i, (cache + particion_buscada->base), (cache + particion_buscada->base + particion_buscada->tamanio), (int)pow(2, particion_buscada->buddy_i));
				}
				break;
			}
		}
		
	}

	fprintf(archivo_dump, "---------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------\n");

	fclose(archivo_dump);
}

void controlador_de_seniales(int signal){
	switch(signal){
		case SIGUSR1:{
			// 9. Ejecución de Dump de cache (solo informar que se solicitó el mismo).
			log_info(logBroker, "Se solicitó Dump de la Caché.");
			log_info(logBrokerInterno, "Se solicitó Dump de la Caché.");

			dump_cache();
			break;
		}
		default:{
			log_info(logBrokerInterno, "Verificar la señal enviada.");
			break;
		}
	}
}

/* FUNCIONES - CONEXIÓN */

void atenderCliente(int socket){
	int socket_cliente = socket;
	log_info(logBrokerInterno,"Atender Cliente %d: ", socket_cliente);
	op_code cod_op = recibirOperacion(socket_cliente);
	switch(cod_op){
		case NEW_POKEMON:{
			atenderMensajeNewPokemon(socket_cliente);
			break;
		}
		case APPEARED_POKEMON:{
			atenderMensajeAppearedPokemon(socket_cliente);
			break;
		}
		case CATCH_POKEMON:{
			atenderMensajeCatchPokemon(socket_cliente);
			break;
		}
		case CAUGHT_POKEMON:{
			atenderMensajeCaughtPokemon(socket_cliente);
			break;
		}
		case GET_POKEMON:{
			atenderMensajeGetPokemon(socket_cliente);
			break;
		}
		case LOCALIZED_POKEMON:{
			atenderMensajeLocalizedPokemon(socket_cliente);
			break;
		}
		case SUSCRIBE_TEAM:{
			atenderSuscripcionTeam(socket_cliente);
			break;
		}
		case SUSCRIBE_GAMECARD:{
			atenderSuscripcionGameCard(socket_cliente);
			break;
		}
		case SUSCRIBE_GAMEBOY:{
			atenderSuscripcionGameBoy(socket_cliente);
			break;
		}
		default:{
			log_warning(logBrokerInterno, "No se pudo conectar ningún proceso.");
			break;
		}
	}
}

void atenderMensajeNewPokemon(int socket_cliente){
	process_code tipo_proceso = recibirTipoProceso(socket_cliente);
//	log_info(logBrokerInterno, "Tipo de Proceso %d", tipo_proceso);
	uint32_t id_proceso = recibirIDProceso(socket_cliente);
//	log_info(logBrokerInterno, "ID de Proceso %d", id_proceso);
	
	t_new_pokemon* new_pokemon = recibirNewPokemon(socket_cliente);

	switch(tipo_proceso){
		case P_TEAM:{
			// 1. Conexión de un proceso al broker.
			log_info(logBroker, "Se conectó el Team %d.", id_proceso);
			log_info(logBrokerInterno, "Se conectó el Team %d.", id_proceso);
		
			// 3. Llegada de un nuevo mensaje a una cola de mensajes.
			log_info(logBroker, "Llegó un Mensaje NEW_POKEMON %s %d %d %d del Team %d.",new_pokemon->nombre_pokemon,new_pokemon->pos_x,new_pokemon->pos_y,new_pokemon->cantidad, id_proceso);
			log_info(logBrokerInterno, "Llegó un Mensaje NEW_POKEMON %s %d %d %d del Team %d.",new_pokemon->nombre_pokemon,new_pokemon->pos_x,new_pokemon->pos_y,new_pokemon->cantidad, id_proceso);

			break;	
		}
		case P_GAMECARD:{
			// 1. Conexión de un proceso al broker.
			log_info(logBroker, "NO se debe conectar el Game Card para enviar un NEW_POKEMON.");
			log_info(logBrokerInterno, "NO se debe conectar el Game Card para enviar un NEW_POKEMON.");
			
			break;
		}
		case P_GAMEBOY:{
			// 1. Conexión de un proceso al broker.
			log_info(logBroker, "Se conectó el Game Boy %d.", id_proceso);
			log_info(logBrokerInterno, "Se conectó el Game Boy %d.", id_proceso);

			// 3. Llegada de un nuevo mensaje a una cola de mensajes.
			log_info(logBroker, "Llegó un Mensaje NEW_POKEMON %s %d %d %d del Game Boy %d.",new_pokemon->nombre_pokemon,new_pokemon->pos_x,new_pokemon->pos_y,new_pokemon->cantidad, id_proceso);
			log_info(logBrokerInterno, "Llegó un Mensaje NEW_POKEMON %s %d %d %d del Game Boy %d.",new_pokemon->nombre_pokemon,new_pokemon->pos_x,new_pokemon->pos_y,new_pokemon->cantidad, id_proceso);

			break;
		}
		default:{
			log_info(logBrokerInterno, "Verificar el Tipo de Proceso y/o ID de Proceso enviado.");
			break;
		}
	}

	uint32_t id_mensaje = 0;

	encolarNewPokemon(new_pokemon);

	devolverID(socket_cliente,&id_mensaje);

	new_pokemon->id_mensaje = id_mensaje;

	cachearNewPokemon(new_pokemon);
	
	int tam_lista_suscriptores = list_size(cola_new->suscriptores);
	
	for(int j = 0; j < tam_lista_suscriptores; j++){
		t_suscriptor* suscriptor = list_get(cola_new->suscriptores, j);
		
		t_new_aux* new = malloc(sizeof(t_new_aux));
		new->socket = suscriptor->socket_suscriptor;
		new->id_suscriptor = suscriptor->id_suscriptor;
		new->mensaje = new_pokemon;
		new->id_mensaje = id_mensaje;

		pthread_t hiloNew;
		pthread_create(&hiloNew, NULL, (void*)enviarNewASuscriptor, new);
		pthread_detach(hiloNew);
	}
}

void enviarNewASuscriptor(t_new_aux* aux){
	t_new_pokemon* new_pokemon = aux->mensaje;
	int socket = aux->socket;
	uint32_t id_suscriptor = aux->id_suscriptor;
	uint32_t id_mensaje = aux->id_mensaje;

	int enviado = enviarNewPokemon(socket, *new_pokemon,P_BROKER,0);	
	
	if(enviado > 0){
		uint32_t ack = recibirACK(socket);
		if(ack == 1){
			log_info(logBrokerInterno, "Se reenvió NEW_POKEMON %s %d %d %d [%d] al socket %d", new_pokemon->nombre_pokemon, new_pokemon->pos_x, new_pokemon->pos_y, new_pokemon->cantidad, new_pokemon->id_mensaje,socket);
			log_info(logBrokerInterno,"Se recibió el ACK %d",ack);
			agregarSuscriptor(id_mensaje, id_suscriptor);
		}else{
			list_add(nodo_new->susc_no_ack, (void*)id_suscriptor);
		}
	}else{
		// 4. Envío de un mensaje a un suscriptor específico.	
		log_warning(logBroker, "NO se envió el Mensaje con ID de Mensaje %d.", new_pokemon->id_mensaje);
		log_warning(logBrokerInterno, "NO se envió el Mensaje con ID de Mensaje %d.", new_pokemon->id_mensaje);
	}
		
	pthread_mutex_lock(&sem_nodo_new);
	if(list_is_empty(nodo_appeared->susc_no_ack)){
		pthread_mutex_unlock(&sem_nodo_new);
		log_debug(logBrokerInterno, "Se elimina el nodo con ID de Mensaje [%d].", nodo_new->mensaje->id_mensaje);
		pthread_mutex_lock(&sem_cola_new);

		bool comparar_id_mensaje(void *element){
			t_nodo_cola_new* nodo = element;
				
			if(nodo_new->mensaje->id_mensaje == nodo->mensaje->id_mensaje){
				log_warning(logBrokerInterno, "Los IDs de Mensaje son IGUALES.");
				return (true);
			}else return (false);
		}
		
		list_remove_by_condition(cola_new->nodos, comparar_id_mensaje);
		pthread_mutex_unlock(&sem_cola_new);
	}else{
		pthread_mutex_unlock(&sem_nodo_new);	
	}
}

void atenderMensajeAppearedPokemon(int socket_cliente){
	process_code tipo_proceso = recibirTipoProceso(socket_cliente);
//	log_info(logBrokerInterno, "Tipo de Proceso %d", tipo_proceso);
	uint32_t id_proceso = recibirIDProceso(socket_cliente);
//	log_info(logBrokerInterno, "ID de Proceso %d", id_proceso);
	
	t_appeared_pokemon* appeared_pokemon = recibirAppearedPokemon(socket_cliente);

	switch(tipo_proceso){
		case P_TEAM:{
			// 1. Conexión de un proceso al broker.
			log_info(logBroker, "NO se debe conectar el Team para enviar un APPEARED_POKEMON.");
			log_info(logBrokerInterno, "NO se debe conectar el Team para enviar un APPEARED_POKEMON.");
			
			break;	
		}
		case P_GAMECARD:{
			// 1. Conexión de un proceso al broker.
			log_info(logBroker, "Se conectó el Game Card %d.", id_proceso);
			log_info(logBrokerInterno, "Se conectó el Game Card %d.", id_proceso);
			
			if(appeared_pokemon->id_mensaje_correlativo > 0){
				// 3. Llegada de un nuevo mensaje a una cola de mensajes.
				log_info(logBroker, "Llegó un Mensaje APPEARED_POKEMON %s %d %d %d.",appeared_pokemon->nombre_pokemon,appeared_pokemon->pos_x,appeared_pokemon->pos_y, appeared_pokemon->id_mensaje_correlativo);
				log_info(logBrokerInterno, "Llegó un Mensaje APPEARED_POKEMON %s %d %d %d.",appeared_pokemon->nombre_pokemon,appeared_pokemon->pos_x,appeared_pokemon->pos_y, appeared_pokemon->id_mensaje_correlativo);

			}else{
				// 3. Llegada de un nuevo mensaje a una cola de mensajes.
				log_info(logBroker, "Llegó un Mensaje APPEARED_POKEMON %s %d %d.",appeared_pokemon->nombre_pokemon,appeared_pokemon->pos_x,appeared_pokemon->pos_y);
				log_info(logBrokerInterno, "Llegó un Mensaje APPEARED_POKEMON %s %d %d.",appeared_pokemon->nombre_pokemon,appeared_pokemon->pos_x,appeared_pokemon->pos_y);
			}

			break;
		}
		case P_GAMEBOY:{
			// 1. Conexión de un proceso al broker.
			log_info(logBroker, "Se conectó el Game Boy %d.", id_proceso);
			log_info(logBrokerInterno, "Se conectó el Game Boy %d.", id_proceso);

			if(appeared_pokemon->id_mensaje_correlativo > 0){
				// 3. Llegada de un nuevo mensaje a una cola de mensajes.
				log_info(logBroker, "Llegó un Mensaje APPEARED_POKEMON %s %d %d %d del Game Boy %d.",appeared_pokemon->nombre_pokemon,appeared_pokemon->pos_x,appeared_pokemon->pos_y, appeared_pokemon->id_mensaje_correlativo, id_proceso);
				log_info(logBrokerInterno, "Llegó un Mensaje APPEARED_POKEMON %s %d %d %d del Game Boy %d.",appeared_pokemon->nombre_pokemon,appeared_pokemon->pos_x,appeared_pokemon->pos_y, appeared_pokemon->id_mensaje_correlativo, id_proceso);
			}else{
				// 3. Llegada de un nuevo mensaje a una cola de mensajes.
				log_info(logBroker, "Llegó un Mensaje APPEARED_POKEMON %s %d %d del Game Boy %d.",appeared_pokemon->nombre_pokemon,appeared_pokemon->pos_x,appeared_pokemon->pos_y, id_proceso);
				log_info(logBrokerInterno, "Llegó un Mensaje APPEARED_POKEMON %s %d %d del Game Boy %d.",appeared_pokemon->nombre_pokemon,appeared_pokemon->pos_x,appeared_pokemon->pos_y, id_proceso);
			}
			
			break;
		}
		default:{
			log_info(logBrokerInterno, "Verificar el Tipo de Proceso y/o ID de Proceso enviado.");
			break;
		}
	}

	uint32_t id_mensaje = 0;

	encolarAppearedPokemon(appeared_pokemon);

	devolverID(socket_cliente,&id_mensaje);
//	appeared_pokemon->id_mensaje_correlativo = id_mensaje;

	cachearAppearedPokemon(appeared_pokemon);

	int tam_lista_suscriptores = list_size(cola_appeared->suscriptores);
	
	for(int j = 0; j < tam_lista_suscriptores; j++){
		t_suscriptor* suscriptor = list_get(cola_appeared->suscriptores, j);
		
		t_appeared_aux* appeared = malloc(sizeof(t_appeared_aux));
		appeared->socket = suscriptor->socket_suscriptor;
		appeared->id_suscriptor = suscriptor->id_suscriptor;
		appeared->mensaje = appeared_pokemon;
		appeared->id_mensaje = id_mensaje;

		pthread_t hiloAppeared;
		pthread_create(&hiloAppeared, NULL, (void*)enviarAppearedASuscriptor, appeared);
		pthread_detach(hiloAppeared);
	}	
}

void enviarAppearedASuscriptor(t_appeared_aux* aux){
	t_appeared_pokemon* appeared_pokemon = aux->mensaje;
	int socket = aux->socket;
	uint32_t id_suscriptor = aux->id_suscriptor;
	uint32_t id_mensaje = aux->id_mensaje;

	int enviado = enviarAppearedPokemon(socket, *appeared_pokemon, P_BROKER, 0);

	if(enviado > 0){
			uint32_t ack = recibirACK(socket);
			if(ack == 1){
				log_info(logBrokerInterno, "Se reenvió APPEARED_POKEMON %s %d %d [%d] al socket %d", appeared_pokemon->nombre_pokemon, appeared_pokemon->pos_x, appeared_pokemon->pos_y, appeared_pokemon->id_mensaje_correlativo,socket);
				log_info(logBrokerInterno,"Se recibió el ACK %d",ack);
				agregarSuscriptor(id_mensaje, id_suscriptor);
			}else{
				log_info(logBrokerInterno, "NO ACK");
				pthread_mutex_lock(&sem_nodo_appeared);
				list_add(nodo_appeared->susc_no_ack, (void*)id_suscriptor);
				pthread_mutex_unlock(&sem_nodo_appeared);
			}
	}else{
		// 4. Envío de un mensaje a un suscriptor específico.
		log_warning(logBroker, "NO se envió el Mensaje con ID de Mensaje Correlativo %d.", appeared_pokemon->id_mensaje_correlativo);
		log_warning(logBrokerInterno, "NO se envió el Mensaje con ID de Mensaje Correlativo %d.", appeared_pokemon->id_mensaje_correlativo);
	}
	
	pthread_mutex_lock(&sem_nodo_appeared);
	if(list_is_empty(nodo_appeared->susc_no_ack)){
		pthread_mutex_unlock(&sem_nodo_appeared);
		log_debug(logBrokerInterno, "Se elimina el nodo con ID de Mensaje Correlativo [%d].", nodo_appeared->mensaje->id_mensaje_correlativo);
		pthread_mutex_lock(&sem_cola_appeared);

		bool comparar_id_mensaje(void *element){
			t_nodo_cola_appeared* nodo = element;
				
			if(nodo_appeared->mensaje->id_mensaje_correlativo == nodo->mensaje->id_mensaje_correlativo){
				log_warning(logBrokerInterno, "Los IDs Correlativos son IGUALES.");
				return (true);
			}else return (false);
		}
		
		list_remove_by_condition(cola_appeared->nodos, comparar_id_mensaje);
		pthread_mutex_unlock(&sem_cola_appeared);
	}else{
		pthread_mutex_unlock(&sem_nodo_appeared);	
	}
}

void atenderMensajeCatchPokemon(int socket_cliente){
	process_code tipo_proceso = recibirTipoProceso(socket_cliente);
//	log_info(logBrokerInterno, "Tipo de Proceso %d", tipo_proceso);
	uint32_t id_proceso = recibirIDProceso(socket_cliente);
//	log_info(logBrokerInterno, "ID de Proceso %d", id_proceso);

	t_catch_pokemon* catch_pokemon = recibirCatchPokemon(socket_cliente);

	switch(tipo_proceso){
		case P_TEAM:{
			// 1. Conexión de un proceso al broker.
			log_info(logBroker, "Se conectó el Team %d.", id_proceso);
			log_info(logBrokerInterno, "Se conectó el Team %d.", id_proceso);
		
			// 3. Llegada de un nuevo mensaje a una cola de mensajes.
			log_info(logBroker, "Llegó un Mensaje CATCH_POKEMON %s %d %d del Team %d.",catch_pokemon->nombre_pokemon,catch_pokemon->pos_x,catch_pokemon->pos_y, id_proceso);
			log_info(logBrokerInterno, "Llegó un Mensaje CATCH_POKEMON %s %d %d del Team %d.",catch_pokemon->nombre_pokemon,catch_pokemon->pos_x,catch_pokemon->pos_y, id_proceso);

			break;	
		}
		case P_GAMECARD:{
			// 1. Conexión de un proceso al broker.
			log_info(logBroker, "NO se debe conectar el Game Card para enviar un CATCH_POKEMON.");
			log_info(logBrokerInterno, "NO se debe conectar el Game Card para enviar un CATCH_POKEMON.");
			
			break;
		}
		case P_GAMEBOY:{
			// 1. Conexión de un proceso al broker.
			log_info(logBroker, "Se conectó el Game Boy %d.", id_proceso);
			log_info(logBrokerInterno, "Se conectó el Game Boy %d.", id_proceso);

			// 3. Llegada de un nuevo mensaje a una cola de mensajes.
			log_info(logBroker, "Llegó un Mensaje CATCH_POKEMON %s %d %d del Game Boy %d.",catch_pokemon->nombre_pokemon,catch_pokemon->pos_x,catch_pokemon->pos_y, id_proceso);
			log_info(logBrokerInterno, "Llegó un Mensaje CATCH_POKEMON %s %d %d del Game Boy %d.",catch_pokemon->nombre_pokemon,catch_pokemon->pos_x,catch_pokemon->pos_y, id_proceso);
			
			break;
		}
		default:{
			log_info(logBrokerInterno, "Verificar el Tipo de Proceso y/o ID de Proceso enviado.");
			break;
		}
	}

	uint32_t id_mensaje = 0;

	encolarCatchPokemon(catch_pokemon);

	devolverID(socket_cliente,&id_mensaje);
	catch_pokemon->id_mensaje = id_mensaje;

	cachearCatchPokemon(catch_pokemon);

	int tam_lista_suscriptores = list_size(cola_catch->suscriptores);
	
	for(int j = 0; j < tam_lista_suscriptores; j++){
		t_suscriptor* suscriptor = list_get(cola_catch->suscriptores, j);
		
		t_catch_aux* catch = malloc(sizeof(t_catch_aux));
		catch->socket = suscriptor->socket_suscriptor;
		catch->id_suscriptor = suscriptor->id_suscriptor;
		catch->mensaje = catch_pokemon;
		catch->id_mensaje = id_mensaje;

		pthread_t hiloCatch;
		pthread_create(&hiloCatch, NULL, (void*)enviarCatchASuscriptor, catch);
		pthread_detach(hiloCatch);
	}
}

void enviarCatchASuscriptor(t_catch_aux* aux){
	t_catch_pokemon* catch_pokemon = aux->mensaje;
	int socket = aux->socket;
	uint32_t id_suscriptor = aux->id_suscriptor;
	uint32_t id_mensaje = aux->id_mensaje;
	
	int enviado = enviarCatchPokemon(socket, *catch_pokemon,P_BROKER,0);
			
	if(enviado > 0){
		uint32_t ack = recibirACK(socket);
		if(ack == 1){
			log_info(logBrokerInterno, "Se reenvió CATCH_POKEMON %s %d %d [%d] al socket %d", catch_pokemon->nombre_pokemon, catch_pokemon->pos_x, catch_pokemon->pos_y, catch_pokemon->id_mensaje,socket);
			log_info(logBrokerInterno,"Se recibió el ACK %d",ack);
			agregarSuscriptor(id_mensaje, id_suscriptor);
		}
	}else{
			// 4. Envío de un mensaje a un suscriptor específico.
			log_warning(logBroker, "NO se envió el Mensaje con ID de Mensaje %d.", catch_pokemon->id_mensaje);
			log_warning(logBrokerInterno, "NO se envió el Mensaje con ID de Mensaje %d.", catch_pokemon->id_mensaje);
	}

	pthread_mutex_lock(&sem_nodo_catch);
	if(list_is_empty(nodo_catch->susc_no_ack)){
		pthread_mutex_unlock(&sem_nodo_catch);
		log_debug(logBrokerInterno, "Se elimina el nodo con ID de Mensaje [%d].", nodo_catch->mensaje->id_mensaje);
		pthread_mutex_lock(&sem_cola_catch);

		bool comparar_id_mensaje(void *element){
			t_nodo_cola_catch* nodo = element;
				
			if(nodo_catch->mensaje->id_mensaje == nodo->mensaje->id_mensaje){
				log_warning(logBrokerInterno, "Los IDs Correlativos son IGUALES.");
				return (true);
			}else return (false);
		}
		
		list_remove_by_condition(cola_catch->nodos, comparar_id_mensaje);
		pthread_mutex_unlock(&sem_cola_catch);
	}else{
		pthread_mutex_unlock(&sem_nodo_catch);	
	}
}

void atenderMensajeCaughtPokemon(int socket_cliente){
	process_code tipo_proceso = recibirTipoProceso(socket_cliente);
//	log_info(logBrokerInterno, "Tipo de Proceso %d", tipo_proceso);
	uint32_t id_proceso = recibirIDProceso(socket_cliente);
//	log_info(logBrokerInterno, "ID de Proceso %d", id_proceso);

	t_caught_pokemon* caught_pokemon = recibirCaughtPokemon(socket_cliente);

	switch(tipo_proceso){
		case P_TEAM:{
			// 1. Conexión de un proceso al broker.
			log_info(logBroker, "NO se debe conectar el Team para enviar un CAUGHT_POKEMON.");
			log_info(logBrokerInterno, "NO se debe conectar el Team para enviar un CAUGHT_POKEMON.");
			break;	
		}
		case P_GAMECARD:{
			// 1. Conexión de un proceso al broker.
			log_info(logBroker, "Se conectó el Game Card %d.", id_proceso);
			log_info(logBrokerInterno, "Se conectó el Game Card %d.", id_proceso);
			// 3. Llegada de un nuevo mensaje a una cola de mensajes.
			log_info(logBroker, "Llegó un Mensaje CAUGHT_POKEMON %d [%d] del Game Card %d.",caught_pokemon->atrapo_pokemon,caught_pokemon->id_mensaje_correlativo, id_proceso);
			log_info(logBrokerInterno, "Llegó un Mensaje CAUGHT_POKEMON %d [%d] del Game Card %d.",caught_pokemon->atrapo_pokemon,caught_pokemon->id_mensaje_correlativo, id_proceso);
			break;
		}
		case P_GAMEBOY:{
			// 1. Conexión de un proceso al broker.
			log_info(logBroker, "Se conectó el Game Boy %d.", id_proceso);
			log_info(logBrokerInterno, "Se conectó el Game Boy %d.", id_proceso);
			// 3. Llegada de un nuevo mensaje a una cola de mensajes.
			log_info(logBroker, "Llegó un Mensaje CAUGHT_POKEMON %d [%d] del Game Boy %d.",caught_pokemon->atrapo_pokemon,caught_pokemon->id_mensaje_correlativo, id_proceso);
			log_info(logBrokerInterno, "Llegó un Mensaje CAUGHT_POKEMON %d [%d] del Game Boy %d.",caught_pokemon->atrapo_pokemon,caught_pokemon->id_mensaje_correlativo, id_proceso);
			break;
		}
		default:{
			log_info(logBrokerInterno, "Verificar el Tipo de Proceso y/o ID de Proceso enviado.");
			break;
		}
	}

	uint32_t id_mensaje = 0;

	encolarCaughtPokemon(caught_pokemon);

	devolverID(socket_cliente,&id_mensaje);
	//caught_pokemon->id_mensaje_correlativo = id_mensaje;

	cachearCaughtPokemon(caught_pokemon);

	int tam_lista_suscriptores = list_size(cola_caught->suscriptores);
	
	for(int j = 0; j < tam_lista_suscriptores; j++){
		t_suscriptor* suscriptor = list_get(cola_caught->suscriptores, j);
	
		t_caught_aux* caught = malloc(sizeof(t_caught_aux));
		caught->socket = suscriptor->socket_suscriptor;
		caught->id_suscriptor = suscriptor->id_suscriptor;
		caught->mensaje = caught_pokemon;
		caught->id_mensaje = id_mensaje;

		pthread_t hiloCaught;
		pthread_create(&hiloCaught, NULL, (void*)enviarCaughtASuscriptor, caught);
		pthread_detach(hiloCaught);
	}
}

void enviarCaughtASuscriptor(t_caught_aux* aux){
	t_caught_pokemon* caught_pokemon = aux->mensaje;
	int socket = aux->socket;
	uint32_t id_suscriptor = aux->id_suscriptor;
	uint32_t id_mensaje = aux->id_mensaje;

	int enviado = enviarCaughtPokemon(socket, *caught_pokemon,P_BROKER,0);
		
	if(enviado > 0){
		uint32_t ack = recibirACK(socket);
		if(ack == 1){
			log_info(logBrokerInterno, "Se reenvió CAUGHT %u [%u] al socket %d", caught_pokemon->atrapo_pokemon,caught_pokemon->id_mensaje_correlativo,socket);
			log_info(logBrokerInterno,"Se recibió el ACK %d",ack);
			agregarSuscriptor(id_mensaje, id_suscriptor);
		}
	}else{
		// 4. Envío de un mensaje a un suscriptor específico.
		log_warning(logBroker, "NO se envió el Mensaje con ID de Mensaje Correlativo %d.", caught_pokemon->id_mensaje_correlativo);
		log_warning(logBrokerInterno, "NO se envió el Mensaje con ID de Mensaje Correlativo %d.", caught_pokemon->id_mensaje_correlativo);
	}

	pthread_mutex_lock(&sem_nodo_caught);
	if(list_is_empty(nodo_caught->susc_no_ack)){
		pthread_mutex_unlock(&sem_nodo_caught);
		log_debug(logBrokerInterno, "Se elimina el nodo con ID de Mensaje Correlativo [%d].", nodo_caught->mensaje->id_mensaje_correlativo);
		pthread_mutex_lock(&sem_cola_caught);

		bool comparar_id_mensaje(void *element){
			t_nodo_cola_caught* nodo = element;
				
			if(nodo_caught->mensaje->id_mensaje_correlativo == nodo->mensaje->id_mensaje_correlativo){
				log_warning(logBrokerInterno, "Los IDs Correlativos son IGUALES.");
				return (true);
			}else return (false);
		}
		
		list_remove_by_condition(cola_caught->nodos, comparar_id_mensaje);
		pthread_mutex_unlock(&sem_cola_caught);
	}else{
		pthread_mutex_unlock(&sem_nodo_caught);	
	}
}

void atenderMensajeGetPokemon(int socket_cliente){
	process_code tipo_proceso = recibirTipoProceso(socket_cliente);
//	log_info(logBrokerInterno, "Tipo de Proceso %d", tipo_proceso);
	uint32_t id_proceso = recibirIDProceso(socket_cliente);
//	log_info(logBrokerInterno, "ID de Proceso %d", id_proceso);

	t_get_pokemon* get_pokemon = recibirGetPokemon(socket_cliente);

	switch(tipo_proceso){
		case P_TEAM:{
			// 1. Conexión de un proceso al broker.
			log_info(logBroker, "Se conectó el Team %d.", id_proceso);
			log_info(logBrokerInterno, "Se conectó el Team %d.", id_proceso);
		
			// 3. Llegada de un nuevo mensaje a una cola de mensajes.
			log_info(logBroker, "Llegó un Mensaje GET_POKEMON %s del Team %d.",get_pokemon->nombre_pokemon, id_proceso);
			log_info(logBrokerInterno, "Llegó un Mensaje GET_POKEMON %s del Team %d.",get_pokemon->nombre_pokemon, id_proceso);

			break;	
		}
		case P_GAMECARD:{
			// 1. Conexión de un proceso al broker.
			log_info(logBroker, "NO se debe conectar el Game Card para enviar un GET_POKEMON.");
			log_info(logBrokerInterno, "NO se debe conectar el Game Card para enviar un GET_POKEMON.");
			break;
		}
		case P_GAMEBOY:{
			// 1. Conexión de un proceso al broker.
			log_info(logBroker, "Se conectó el Game Boy %d.", id_proceso);
			log_info(logBrokerInterno, "Se conectó el Game Boy %d.", id_proceso);

			// 3. Llegada de un nuevo mensaje a una cola de mensajes.
			log_info(logBroker, "Llegó un Mensaje GET_POKEMON %s del Game Boy %d.",get_pokemon->nombre_pokemon, id_proceso);
			log_info(logBrokerInterno, "Llegó un Mensaje GET_POKEMON %s del Game Boy %d.",get_pokemon->nombre_pokemon, id_proceso);
			break;
		}
		default:{
			log_info(logBrokerInterno, "Verificar el Tipo de Proceso y/o ID de Proceso enviado.");
			break;
		}
	}

	uint32_t id_mensaje = 0;

	encolarGetPokemon(get_pokemon);

	devolverID(socket_cliente,&id_mensaje);
	get_pokemon->id_mensaje = id_mensaje;

	cachearGetPokemon(get_pokemon);

	int tam_lista_suscriptores = list_size(cola_get->suscriptores);
	
	for(int j = 0; j < tam_lista_suscriptores; j++){
		t_suscriptor* suscriptor = list_get(cola_get->suscriptores, j);

			int enviado = enviarGetPokemon(suscriptor->socket_suscriptor, *get_pokemon,P_BROKER,0);
			
			if(enviado > 0){
				uint32_t ack = recibirACK(suscriptor->socket_suscriptor);
				if(ack == 1){
					log_info(logBrokerInterno, "Se reenvió GET_POKEMON %s [%d] al socket %d", get_pokemon->nombre_pokemon, get_pokemon->id_mensaje,suscriptor->socket_suscriptor);
					log_info(logBrokerInterno,"Se recibió el ACK %d",ack);
					agregarSuscriptor(id_mensaje, suscriptor->id_suscriptor);
				}
			}else{
				// 4. Envío de un mensaje a un suscriptor específico.
				log_warning(logBroker, "NO se envió el Mensaje con ID de Mensaje %d.", get_pokemon->id_mensaje);
				log_warning(logBrokerInterno, "NO se envió el Mensaje con ID de Mensaje %d.", get_pokemon->id_mensaje);
			}
		
	}
}

/*void enviarGetASuscriptor(t_get_aux* aux){
	BASTAAA, ME CANSÉ
}*/

void atenderMensajeLocalizedPokemon(int socket_cliente){
	process_code tipo_proceso = recibirTipoProceso(socket_cliente);
//	log_info(logBrokerInterno, "Tipo de Proceso %d", tipo_proceso);
	uint32_t id_proceso = recibirIDProceso(socket_cliente);
//	log_info(logBrokerInterno, "ID de Proceso %d", id_proceso);

	t_localized_pokemon* localized_pokemon = recibirLocalizedPokemon(socket_cliente);

	switch(tipo_proceso){
		case P_TEAM:{
			// 1. Conexión de un proceso al broker.
			log_info(logBroker, "NO se debe conectar el Team para enviar un LOCALIZED_POKEMON.");
			log_info(logBrokerInterno, "NO se debe conectar el Team para enviar un LOCALIZED_POKEMON.");
			break;	
		}
		case P_GAMECARD:{
			// 1. Conexión de un proceso al broker.
			log_info(logBroker, "Se conectó el Game Card %d.", id_proceso);
			log_info(logBrokerInterno, "Se conectó el Game Card %d.", id_proceso);

			// 3. Llegada de un nuevo mensaje a una cola de mensajes.
			log_info(logBroker, "Llegó un Mensaje LOCALIZED_POKEMON %s %d %s [%d]\n",localized_pokemon->nombre_pokemon,localized_pokemon->cant_pos,localized_pokemon->posiciones,localized_pokemon->id_mensaje_correlativo, id_proceso);
			log_info(logBrokerInterno, "Llegó un Mensaje LOCALIZED_POKEMON %s %d %s [%d]\n",localized_pokemon->nombre_pokemon,localized_pokemon->cant_pos,localized_pokemon->posiciones,localized_pokemon->id_mensaje_correlativo, id_proceso);

			break;
		}
		case P_GAMEBOY:{
			// 1. Conexión de un proceso al broker.
			log_info(logBroker, "NO se debe conectar el Game Boy para enviar un LOCALIZED_POKEMON.");
			log_info(logBrokerInterno, "NO se debe conectar el Game Boy para enviar un LOCALIZED_POKEMON.");
			break;
		}
		default:{
			log_info(logBrokerInterno, "Verificar el Tipo de Proceso y/o ID de Proceso enviado.");
			break;
		}
	}
	
	uint32_t id_mensaje = 0;

	encolarLocalizedPokemon(localized_pokemon);

	devolverID(socket_cliente,&id_mensaje);
//	localized_pokemon->id_mensaje_correlativo = id_mensaje;

	cachearLocalizedPokemon(localized_pokemon);

	int tam_lista_suscriptores = list_size(cola_localized->suscriptores);
	
	for(int j = 0; j < tam_lista_suscriptores; j++){
		t_suscriptor* suscriptor = list_get(cola_localized->suscriptores, j);
		

			int enviado = enviarLocalizedPokemon(suscriptor->socket_suscriptor, *localized_pokemon,P_BROKER,0);
		
			if(enviado > 0){
				uint32_t ack = recibirACK(suscriptor->socket_suscriptor);
				if(ack == 1){
					log_info(logBrokerInterno, "Se reenvió LOCALIZED_POKEMON %s %d %s [%d] al socket %d", localized_pokemon->nombre_pokemon,localized_pokemon->cant_pos,localized_pokemon->posiciones,localized_pokemon->id_mensaje_correlativo,suscriptor->socket_suscriptor);log_info(logBrokerInterno,"Se recibió el ACK %d",ack);
					log_info(logBrokerInterno,"Se recibió el ACK %d",ack);
					agregarSuscriptor(id_mensaje, suscriptor->id_suscriptor);
				}
			}else{
				// 4. Envío de un mensaje a un suscriptor específico.
				log_warning(logBroker, "NO se envió el Mensaje con ID de Mensaje Correlativo %d.", localized_pokemon->id_mensaje_correlativo);
				log_warning(logBrokerInterno, "NO se envió el Mensaje con ID de Mensaje Correlativo %d.", localized_pokemon->id_mensaje_correlativo);
			}

		
	}
}

void atenderSuscripcionTeam(int socket_cliente){
	t_suscribe* suscribe_team = recibirSuscripcion(SUSCRIBE_TEAM,socket_cliente);

	t_suscriptor* suscriptor = malloc(sizeof(t_suscriptor));
	suscriptor->socket_suscriptor = socket_cliente;
	log_info(logBrokerInterno, "Socket Team %d", suscriptor->socket_suscriptor);
	suscriptor->id_suscriptor = suscribe_team->id_proceso;
	log_info(logBrokerInterno, "ID Team %d", suscriptor->id_suscriptor);

	suscribir(suscriptor, suscribe_team->cola_suscribir);

	char* cola = colaParaLogs(suscribe_team->cola_suscribir);

	// 2. Suscripción de un proceso a una cola de mensajes.
	log_info(logBroker, "Se suscribe el Team %d a la Cola de Mensajes %s", suscribe_team->id_proceso, cola);
	log_info(logBrokerInterno, "Se suscribe el Team %d a la Cola de Mensajes %s",  suscribe_team->id_proceso, cola);

	switch(suscribe_team->cola_suscribir){
		case APPEARED_POKEMON:{
			enviarAppearedPokemonCacheados(socket_cliente, suscribe_team);
			break;
		}
		case CAUGHT_POKEMON:{
			enviarCaughtPokemonCacheados(socket_cliente, suscribe_team);
			break;
		}
		case LOCALIZED_POKEMON:{
			enviarLocalizedPokemonCacheados(socket_cliente, suscribe_team);
			break;
		}
		default:{
			log_warning(logBrokerInterno, "No se pudo enviar mensajes cacheados.");
			break;
		}
	}
	free(suscribe_team);
} 

void atenderSuscripcionGameCard(int socket_cliente){
	t_suscribe* suscribe_gamecard = recibirSuscripcion(SUSCRIBE_GAMECARD,socket_cliente);
	
	t_suscriptor* suscriptor = malloc(sizeof(t_suscriptor));
	suscriptor->socket_suscriptor = socket_cliente;
	log_info(logBrokerInterno, "Socket Game Card %d", suscriptor->socket_suscriptor);
	suscriptor->id_suscriptor = suscribe_gamecard->id_proceso;
	log_info(logBrokerInterno, "ID Game Card %d", suscriptor->id_suscriptor);

	suscribir(suscriptor, suscribe_gamecard->cola_suscribir);

	char* cola = colaParaLogs(suscribe_gamecard->cola_suscribir);

	// 2. Suscripción de un proceso a una cola de mensajes.
	log_info(logBroker, "Se suscribe el Game Card %d a la Cola de Mensajes %s", suscribe_gamecard->id_proceso, cola);
	log_info(logBrokerInterno, "Se suscribe el Game Card %d a la Cola de Mensajes %s", suscribe_gamecard->id_proceso, cola);

	switch(suscribe_gamecard->cola_suscribir){
		case NEW_POKEMON:{
			enviarNewPokemonCacheados(socket_cliente, suscribe_gamecard);
			break;
		}
		case CATCH_POKEMON:{
			enviarCatchPokemonCacheados(socket_cliente, suscribe_gamecard);
			break;
		}
		case GET_POKEMON:{
			enviarGetPokemonCacheados(socket_cliente, suscribe_gamecard);
			break;
		}
		default:{
			log_warning(logBrokerInterno, "No se pudo enviar mensajes cacheados.");
			break;
		}
	}
	free(suscribe_gamecard);
}

void atenderSuscripcionGameBoy(int socket_cliente){
	t_suscribe* suscribe_gameboy = recibirSuscripcion(SUSCRIBE_GAMEBOY,socket_cliente);

	t_suscriptor* suscriptor = malloc(sizeof(t_suscriptor));
	suscriptor->socket_suscriptor = socket_cliente;
	log_info(logBrokerInterno, "Socket Game Boy %d", suscriptor->socket_suscriptor);
	suscriptor->id_suscriptor = suscribe_gameboy->id_proceso;
	log_info(logBrokerInterno, "ID Game Boy %d", suscriptor->id_suscriptor);

	int index = suscribir(suscriptor, suscribe_gameboy->cola_suscribir);

	char* cola = colaParaLogs(suscribe_gameboy->cola_suscribir);

	// 2. Suscripción de un proceso a una cola de mensajes.
	log_info(logBroker, "Se suscribe el Game Boy %d a la Cola de Mensajes %s", suscribe_gameboy->id_proceso, cola);
	log_info(logBrokerInterno, "Se suscribe el Game Boy %d a la Cola de Mensajes %s", suscribe_gameboy->id_proceso, cola);

	switch(suscribe_gameboy->cola_suscribir){
		case NEW_POKEMON:{
			enviarNewPokemonCacheados(socket_cliente, suscribe_gameboy);
			break;
		}
		case APPEARED_POKEMON:{
			enviarAppearedPokemonCacheados(socket_cliente, suscribe_gameboy);
			break;
		}
		case CATCH_POKEMON:{
			enviarCatchPokemonCacheados(socket_cliente, suscribe_gameboy);
			break;
		}
		case CAUGHT_POKEMON:{
			enviarCaughtPokemonCacheados(socket_cliente, suscribe_gameboy);
			break;
		}
		case GET_POKEMON:{
			enviarGetPokemonCacheados(socket_cliente, suscribe_gameboy);
			break;
		}
		/*case LOCALIZED_POKEMON:{
			enviarLocalizedPokemonCacheados(socket_cliente, suscribe_gameboy->cola_suscribir, suscribe_gameboy->id_proceso);
			break;
		}*/
		default:{
			log_warning(logBrokerInterno, "No se pudo enviar mensajes cacheados.");
			break;
		}
	}

	sleep(suscribe_gameboy->timeout);
	desuscribir(index,suscribe_gameboy->cola_suscribir, suscribe_gameboy->id_proceso);
	close(socket_cliente);
	free(suscriptor);
	free(suscribe_gameboy);
}

/* FUNCIONES - PROCESAMIENTO */

int suscribir(t_suscriptor* suscriptor, op_code cola){
	int index = 0;
	switch(cola){
		case NEW_POKEMON:{
			pthread_mutex_lock(&sem_cola_new);
			index= list_add(cola_new->suscriptores,suscriptor);
			pthread_mutex_unlock(&sem_cola_new);
			sem_post(&mensajes_new);
			break;
		}
		case APPEARED_POKEMON:{
			pthread_mutex_lock(&sem_cola_appeared);
			index= list_add(cola_appeared->suscriptores,suscriptor);
			pthread_mutex_unlock(&sem_cola_appeared);
			sem_post(&mensajes_appeared);
			break;
		}
		case CATCH_POKEMON:{
			pthread_mutex_lock(&sem_cola_catch);
			index= list_add(cola_catch->suscriptores,suscriptor);
			pthread_mutex_unlock(&sem_cola_catch);
			sem_post(&mensajes_catch);
			break;
		}
		case CAUGHT_POKEMON:{
			pthread_mutex_lock(&sem_cola_caught);
			index= list_add(cola_caught->suscriptores,suscriptor);
			pthread_mutex_unlock(&sem_cola_caught);
			sem_post(&mensajes_caught);
			break;
		}
		case GET_POKEMON:{
			pthread_mutex_lock(&sem_cola_get);
			index= list_add(cola_get->suscriptores,suscriptor);
			pthread_mutex_unlock(&sem_cola_get);
			sem_post(&mensajes_get);
			break;
		}
		case LOCALIZED_POKEMON:{
			pthread_mutex_lock(&sem_cola_localized);
			index= list_add(cola_localized->suscriptores,suscriptor);
			pthread_mutex_unlock(&sem_cola_localized);
			sem_post(&mensajes_localized);
			break;
		}
		default:
			index = -1;
			log_warning(logBrokerInterno, "No se suscribe ningún proceso.");
	}
	return index;
}

void desuscribir(int index,op_code cola, uint32_t id_proceso){
	switch(cola){
		case NEW_POKEMON:{
			sem_wait(&mensajes_new); // No se puede sacar si la Cola de Mensajes está vacía
			pthread_mutex_lock(&sem_cola_new);
			list_remove(cola_new->suscriptores,index);
			pthread_mutex_unlock(&sem_cola_new);
			// 2. Suscripción de un proceso a una cola de mensajes.
			log_info(logBroker, "Finaliza la suscripción del Game Boy %d de la cola NEW_POKEMON", id_proceso);
			log_info(logBrokerInterno, "Finaliza la suscripción del Game Boy %d de la cola NEW_POKEMON", id_proceso);
			break;
		}
		case APPEARED_POKEMON:{
			sem_wait(&mensajes_appeared);
			pthread_mutex_lock(&sem_cola_appeared);
			list_remove(cola_appeared->suscriptores,index);
			pthread_mutex_unlock(&sem_cola_appeared);
			// 2. Suscripción de un proceso a una cola de mensajes.
			log_info(logBroker, "Finaliza la suscripción del Game Boy %d de la cola APPEARED_POKEMON", id_proceso);
			log_info(logBrokerInterno, "Finaliza la suscripción del Game Boy %d de la cola APPEARED_POKEMON", id_proceso);
			break;
		}
		case CATCH_POKEMON:{
			sem_wait(&mensajes_catch);
			pthread_mutex_lock(&sem_cola_catch);
			list_remove(cola_catch->suscriptores,index);
			pthread_mutex_unlock(&sem_cola_catch);
			// 2. Suscripción de un proceso a una cola de mensajes.
			log_info(logBroker, "Finaliza la suscripción del Game Boy %d de la cola CATCH_POKEMON", id_proceso);
			log_info(logBrokerInterno, "Finaliza la suscripción del Game Boy %d de la cola CATCH_POKEMON", id_proceso);
			break;
		}
		case CAUGHT_POKEMON:{
			sem_wait(&mensajes_caught);
			pthread_mutex_lock(&sem_cola_caught);
			list_remove(cola_caught->suscriptores,index);
			pthread_mutex_unlock(&sem_cola_caught);
			// 2. Suscripción de un proceso a una cola de mensajes.
			log_info(logBroker, "Finaliza la suscripción del Game Boy %d de la cola CAUGHT_POKEMON", id_proceso);
			log_info(logBrokerInterno, "Finaliza la suscripción del Game Boy %d de la cola CAUGHT_POKEMON", id_proceso);
			break;
		}
		case GET_POKEMON:{
			sem_wait(&mensajes_get);
			pthread_mutex_lock(&sem_cola_get);
			list_remove(cola_get->suscriptores,index);
			pthread_mutex_unlock(&sem_cola_get);
			// 2. Suscripción de un proceso a una cola de mensajes.
			log_info(logBroker, "Finaliza la suscripción del Game Boy %d de la cola GET_POKEMON", id_proceso);
			log_info(logBrokerInterno, "Finaliza la suscripción del Game Boy %d de la cola GET_POKEMON", id_proceso);
			break;
		}
		case LOCALIZED_POKEMON:{
			sem_wait(&mensajes_localized);
			pthread_mutex_lock(&sem_cola_localized);
			list_remove(cola_localized->suscriptores,index);
			pthread_mutex_unlock(&sem_cola_localized);
			// 2. Suscripción de un proceso a una cola de mensajes.
			log_info(logBroker, "Finaliza la suscripción del Game Boy %d de la cola LOCALIZED_POKEMON", id_proceso);
			log_info(logBrokerInterno, "Finaliza la suscripción del Game Boy %d de la cola LOCALIZED_POKEMON", id_proceso);
			break;
		}
		default:
			break;
	}
}

void agregarSuscriptor(uint32_t id_mensaje, uint32_t id_suscriptor){
	int tam = list_size(particiones);
	bool encontrado = 0;

	for(int i = 0; i < tam && encontrado == 0; i++){
		t_particion* particion = list_get(particiones, i);

		if(particion->id == id_mensaje){
			sem_wait(&mx_particiones);
			list_add(particion->susc_enviados,(void*)id_suscriptor);
			sem_post(&mx_particiones);
			encontrado = 1;
		}
	}
}

void encolarNewPokemon(t_new_pokemon* mensaje){
	nodo_new = malloc(sizeof(t_nodo_cola_new));
	nodo_new->mensaje = mensaje;
	nodo_new->susc_enviados = list_create();
	nodo_new->susc_ack = list_create();
	nodo_new->susc_no_ack = list_create();

	list_add(cola_new->nodos,nodo_new);
}

void encolarAppearedPokemon(t_appeared_pokemon* mensaje){
	nodo_appeared = malloc(sizeof(t_nodo_cola_appeared));
	nodo_appeared->mensaje = mensaje;
	nodo_appeared->susc_enviados = list_create();
	nodo_appeared->susc_ack = list_create();
	nodo_appeared->susc_no_ack = list_create();

	list_add(cola_appeared->nodos,nodo_appeared);
}

void encolarCatchPokemon(t_catch_pokemon* mensaje){
	nodo_catch = malloc(sizeof(t_nodo_cola_catch));
	nodo_catch->mensaje = mensaje;
	nodo_catch->susc_enviados = list_create();
	nodo_catch->susc_ack = list_create();
	nodo_catch->susc_no_ack = list_create();

	list_add(cola_catch->nodos,nodo_catch);
}

void encolarCaughtPokemon(t_caught_pokemon* mensaje){
	nodo_caught = malloc(sizeof(t_nodo_cola_caught));
	nodo_caught->mensaje = mensaje;
	nodo_caught->susc_enviados = list_create();
	nodo_caught->susc_ack = list_create();
	nodo_caught->susc_no_ack = list_create();

	list_add(cola_caught->nodos,nodo_caught);
}

void encolarGetPokemon(t_get_pokemon* mensaje){
	nodo_get = malloc(sizeof(t_nodo_cola_get));
	nodo_get->mensaje = mensaje;
	nodo_get->susc_enviados = list_create();
	nodo_get->susc_ack = list_create();
	nodo_get->susc_no_ack = list_create();

	list_add(cola_get->nodos,nodo_get);
}

void encolarLocalizedPokemon(t_localized_pokemon* mensaje){
	nodo_localized = malloc(sizeof(t_nodo_cola_localized));
	nodo_localized->mensaje = mensaje;
	nodo_localized->susc_enviados = list_create();
	nodo_localized->susc_ack = list_create();
	nodo_localized->susc_no_ack = list_create();

	list_add(cola_localized->nodos,nodo_localized);
}

/* FUNCIONES - COMUNICACIÓN */

void tipoYIDProceso(int socket_cliente){
	process_code tipo_proceso = recibirTipoProceso(socket_cliente);
//	log_info(logBrokerInterno, "Tipo de Proceso %d", tipo_proceso);
	uint32_t id_proceso = recibirIDProceso(socket_cliente);
//	log_info(logBrokerInterno, "ID de Proceso %d", id_proceso);

	switch(tipo_proceso){
		case P_TEAM:{
			// 1. Conexión de un proceso al broker.
			log_info(logBroker, "Se conectó el Team %d.", id_proceso);
			log_info(logBrokerInterno, "Se conectó el Team %d.", id_proceso);
			break;	
		}
		case P_GAMECARD:{
			// 1. Conexión de un proceso al broker.
			log_info(logBroker, "Se conectó el Game Card %d.", id_proceso);
			log_info(logBrokerInterno, "Se conectó el Game Card %d.", id_proceso);
			break;
		}
		case P_GAMEBOY:{
			// 1. Conexión de un proceso al broker.
			log_info(logBroker, "Se conectó el Game Boy %d.", id_proceso);
			log_info(logBrokerInterno, "Se conectó el Game Boy %d.", id_proceso);
			break;
		}
		default:{
			log_info(logBrokerInterno, "Verificar el Tipo de Proceso y/o ID de Proceso enviado.");
			break;
		}
	}
}

int devolverID(int socket,uint32_t* id_mensaje){
	sem_wait(&identificador);
	ID_MENSAJE ++;
	uint32_t id = ID_MENSAJE;
	sem_post(&identificador);

	(*id_mensaje) = id;

	void*stream = malloc(sizeof(uint32_t));

	memcpy(stream, &(id), sizeof(uint32_t));

	int enviado = send(socket, stream, sizeof(uint32_t), 0);

	if(enviado != -1){
		log_info(logBrokerInterno,"Se asignó y envió el ID_MENSAJE %d",*id_mensaje);
	}else{
		log_info(logBrokerInterno,"No se pudo enviar el ID_MENSAJE %d",*id_mensaje);
	}

	free(stream);
	return enviado;
}

void enviarNewPokemonCacheados(int socket, t_suscribe* suscriptor){

	int tam_lista = list_size(particiones);
	t_particion* particion_buscada;
	bool me_pase = false;
	for(int i = 0; i < tam_lista && me_pase == false ; i++){
		sem_wait(&mx_particiones);
		if(i >= list_size(particiones)){
			me_pase = true;
			sem_post(&mx_particiones);
			log_warning(logBrokerInterno,"Sali por pasarme de indice en descachear NEW");
			continue;
		}
		particion_buscada = list_get(particiones, i);
	
		if(particion_buscada->libre == 0 && particion_buscada->tipo_mensaje == suscriptor->cola_suscribir){
			void* stream = malloc(particion_buscada->tamanio);
			memcpy(stream, cache + particion_buscada->base, particion_buscada->tamanio);
			sem_post(&mx_particiones);

			bool comparar_id_proceso(void *element){
				uint32_t id_proceso = suscriptor->id_proceso;
				uint32_t id_nodo = (uint32_t)element;

				if(id_proceso == id_nodo){
					log_warning(logBrokerInterno, "Ya se envió el Mensaje New con ID de Mensaje [%d] al Suscriptor %d", particion_buscada->id, suscriptor->id_proceso);
					return (true);
				}else return (false);
			}

			if(!list_any_satisfy(particion_buscada->susc_enviados, comparar_id_proceso)){
				
				t_new_pokemon* descacheado = descachearNewPokemon(stream, particion_buscada->id);

				struct timeval time_aux;
				gettimeofday(&time_aux, NULL);
				particion_buscada->time_ultima_ref = time_aux;

				log_info(logBrokerInterno, "Descacheado nombre %s", descacheado->nombre_pokemon);
				log_info(logBrokerInterno, "Descacheado pos x %d", descacheado->pos_x);
				log_info(logBrokerInterno, "Descacheado pos y %d", descacheado->pos_y);
				log_info(logBrokerInterno, "Descacheado cantidad %d", descacheado->cantidad);
				log_info(logBrokerInterno, "Descacheado id mensaje %d", descacheado->id_mensaje);

				

			
					int enviado = enviarNewPokemon(socket, *descacheado,P_BROKER,0);

					int ack = recibirACK(socket);
					log_info(logBrokerInterno, "ACK %d", ack);

					char* cola = colaParaLogs(particion_buscada->tipo_mensaje);

					if(enviado == -1){
						// 4. Envío de un mensaje a un suscriptor específico.
						log_warning(logBroker, "NO se envió el Mensaje con ID de Mensaje %d.", descacheado->id_mensaje);
						log_warning(logBrokerInterno, "NO se envió el Mensaje con ID de Mensaje %d.", descacheado->id_mensaje);
					}else{
						list_add(particion_buscada->susc_enviados, (void*)suscriptor->id_proceso);
						switch(suscriptor->tipo_suscripcion){
							case SUSCRIBE_TEAM:{
								// 4. Envío de un mensaje a un suscriptor específico.
								log_info(logBroker, "NO se debe enviar este Mensaje al Team.");
								log_info(logBrokerInterno, "NO se debe enviar este Mensaje al Team.");
								break;
							}
							case SUSCRIBE_GAMECARD:{
								// 4. Envío de un mensaje a un suscriptor específico.
								log_info(logBroker, "Se envió el Mensaje: %s %s %d %d %d con ID de Mensaje %d al Game Card %d.", cola, descacheado->nombre_pokemon, descacheado->pos_x, descacheado->pos_y, descacheado->cantidad, descacheado->id_mensaje, suscriptor->id_proceso);
								log_info(logBrokerInterno, "Se envió el Mensaje: %s %s %d %d %d con ID de Mensaje %d al Game Card %d,", cola, descacheado->nombre_pokemon, descacheado->pos_x, descacheado->pos_y, descacheado->cantidad, descacheado->id_mensaje, suscriptor->id_proceso);
								confirmacionDeRecepcionGameCard(ack, suscriptor, descacheado->id_mensaje);
								break;
							}
							case SUSCRIBE_GAMEBOY:{
								// 4. Envío de un mensaje a un suscriptor específico.
								log_info(logBroker, "Se envió el Mensaje: %s %s %d %d %d con ID de Mensaje %d al Game Boy %d.", cola, descacheado->nombre_pokemon, descacheado->pos_x, descacheado->pos_y, descacheado->cantidad, descacheado->id_mensaje, suscriptor->id_proceso);
								log_info(logBrokerInterno, "Se envió el Mensaje: %s %s %d %d %d con ID de Mensaje %d al Game Boy %d,", cola, descacheado->nombre_pokemon, descacheado->pos_x, descacheado->pos_y, descacheado->cantidad, descacheado->id_mensaje, suscriptor->id_proceso);
								confirmacionDeRecepcionGameBoy(ack, suscriptor, descacheado->id_mensaje);
								break;
							}
							default: break;
						}
					}
					
				
				free(descacheado->nombre_pokemon);
				free(descacheado);
				free(stream);
			}
		}else{
			sem_post(&mx_particiones);
		}
	}
}

void enviarAppearedPokemonCacheados(int socket, t_suscribe* suscriptor){
	int tam_lista = list_size(particiones);
	t_particion* particion_buscada;
	bool me_pase = false;
	for(int i = 0; i < tam_lista && me_pase == false ; i++){
		sem_wait(&mx_particiones);
		if(i >= list_size(particiones)){
			me_pase = true;
			sem_post(&mx_particiones);
			log_warning(logBrokerInterno,"Sali por pasarme de indice en descachear APPEARED");
			continue;
		}
		particion_buscada = list_get(particiones, i);
	
		if(particion_buscada->libre == 0 && particion_buscada->tipo_mensaje == suscriptor->cola_suscribir){	
			void* stream = malloc(particion_buscada->tamanio);
			memcpy(stream, cache + particion_buscada->base, particion_buscada->tamanio);
			sem_post(&mx_particiones);
			bool comparar_id_proceso(void *element){
				uint32_t id_proceso = (uint32_t) suscriptor->id_proceso;
				uint32_t id_nodo = (uint32_t) element;

				if(id_proceso == id_nodo){
					log_warning(logBrokerInterno, "Ya se envió el Mensaje Appeared con ID de Mensaje Appeared Correlativo [%d] al Suscriptor %d", particion_buscada->id, suscriptor->id_proceso);
					return (true);
				}else return (false);
			}

			if(!list_any_satisfy(particion_buscada->susc_enviados, comparar_id_proceso)){

				t_appeared_pokemon descacheado = descachearAppearedPokemon(stream, particion_buscada->id);

				struct timeval time_aux;
				gettimeofday(&time_aux, NULL);
				particion_buscada->time_ultima_ref = time_aux;

				log_info(logBrokerInterno, "Descacheado nombre %s", descacheado.nombre_pokemon);
				log_info(logBrokerInterno, "Descacheado pos x %d", descacheado.pos_x);
				log_info(logBrokerInterno, "Descacheado pos y %d", descacheado.pos_y);
				log_info(logBrokerInterno, "Descacheado id correlativo %d", descacheado.id_mensaje_correlativo);
				


					int enviado = enviarAppearedPokemon(socket, descacheado,P_BROKER,0);

					int ack = recibirACK(socket);
					log_info(logBrokerInterno, "ACK %d", ack);

					char* cola = colaParaLogs(particion_buscada->tipo_mensaje);

					if(enviado == -1){
						// 4. Envío de un mensaje a un suscriptor específico.
						log_warning(logBroker, "NO se envió el Mensaje con ID de Mensaje Correlativo %d.", descacheado.id_mensaje_correlativo);
						log_warning(logBrokerInterno, "NO se envió el Mensaje con ID de Mensaje Correlativo %d.", descacheado.id_mensaje_correlativo);
					}else{
						list_add(particion_buscada->susc_enviados, (void*)suscriptor->id_proceso);
						switch(suscriptor->tipo_suscripcion){
							case SUSCRIBE_TEAM:{
								// 4. Envío de un mensaje a un suscriptor específico.
								log_info(logBroker, "Se envió el Mensaje: %s %s %d %d con ID de Mensaje Correlativo %d al Team %d.", cola, descacheado.nombre_pokemon, descacheado.pos_x, descacheado.pos_y, descacheado.id_mensaje_correlativo, suscriptor->id_proceso);
								log_info(logBrokerInterno, "Se envió el Mensaje: %s %s %d %d con ID de Mensaje Correlativo %d.", cola, descacheado.nombre_pokemon, descacheado.pos_x, descacheado.pos_y, descacheado.id_mensaje_correlativo, suscriptor->id_proceso);
								confirmacionDeRecepcionTeam(ack, suscriptor, descacheado.id_mensaje_correlativo);
								break;
							}
							case SUSCRIBE_GAMECARD:{
								// 4. Envío de un mensaje a un suscriptor específico.
								log_info(logBroker, "NO se debe enviar este Mensaje al Game Card.");
								log_info(logBrokerInterno, "NO se debe enviar este Mensaje al Game Card.");
								break;
							}
							case SUSCRIBE_GAMEBOY:{
								// 4. Envío de un mensaje a un suscriptor específico.
								log_info(logBroker, "Se envió el Mensaje: %s %s %d %d con ID de Mensaje Correlativo %d al Game Boy %d.", cola, descacheado.nombre_pokemon, descacheado.pos_x, descacheado.pos_y, descacheado.id_mensaje_correlativo, suscriptor->id_proceso);
								log_info(logBrokerInterno, "Se envió el Mensaje: %s %s %d %d con ID de Mensaje Correlativo %d al Game Boy %d.", cola, descacheado.nombre_pokemon, descacheado.pos_x, descacheado.pos_y, descacheado.id_mensaje_correlativo, suscriptor->id_proceso);
								confirmacionDeRecepcionGameBoy(ack, suscriptor, descacheado.id_mensaje_correlativo);
								break;
							}
							default: break;
						}
					}
				
				free(descacheado.nombre_pokemon);
				free(stream);
			}
		}else{
			sem_post(&mx_particiones);
		}
	}
}

void enviarCatchPokemonCacheados(int socket, t_suscribe* suscriptor){
	int tam_lista = list_size(particiones);
	t_particion* particion_buscada;

	bool me_pase = false;
	for(int i = 0; i < tam_lista && me_pase == false ; i++){
		sem_wait(&mx_particiones);
		if(i >= list_size(particiones)){
			me_pase = true;
			sem_post(&mx_particiones);
			log_warning(logBrokerInterno,"Sali por pasarme de indice en descachear CATCH");
			continue;
		}
		particion_buscada = list_get(particiones, i);

		if(particion_buscada->libre == 0 && particion_buscada->tipo_mensaje == suscriptor->cola_suscribir){
			void* stream = malloc(particion_buscada->tamanio);
			memcpy(stream, cache + particion_buscada->base, particion_buscada->tamanio);
			sem_post(&mx_particiones);

			bool comparar_id_proceso(void *element){
				uint32_t id_proceso = (uint32_t)suscriptor->id_proceso;
				uint32_t id_nodo = (uint32_t)element;

				if(id_proceso == id_nodo){
					log_warning(logBrokerInterno, "Ya se envió el Mensaje Catch con ID de Mensaje [%d] al Suscriptor %d", particion_buscada->id, suscriptor->id_proceso);
					return (true);
				}else return (false);
			}

			if(!list_any_satisfy(particion_buscada->susc_enviados, comparar_id_proceso)){
				

				t_catch_pokemon descacheado = descachearCatchPokemon(stream, particion_buscada->id);

				struct timeval time_aux;
				gettimeofday(&time_aux, NULL);
				particion_buscada->time_ultima_ref = time_aux;

				log_info(logBrokerInterno, "Descacheado nombre %s", descacheado.nombre_pokemon);
				log_info(logBrokerInterno, "Descacheado pos x %d", descacheado.pos_x);
				log_info(logBrokerInterno, "Descacheado pos y %d", descacheado.pos_y);
				log_info(logBrokerInterno, "Descacheado id %d", descacheado.id_mensaje);
				
				int enviado = enviarCatchPokemon(socket, descacheado,P_BROKER,0);

				int ack = recibirACK(socket);
				log_info(logBrokerInterno, "ACK %d", ack);
					
				char* cola = colaParaLogs(particion_buscada->tipo_mensaje);

				if(enviado == -1){
					log_warning(logBroker, "NO se envió el Mensaje  con ID de Mensaje %d.", descacheado.id_mensaje);
					log_warning(logBrokerInterno, "NO se envió el Mensaje con ID de Mensaje %d.", descacheado.id_mensaje);
				}else{
					list_add(particion_buscada->susc_enviados, (void*)suscriptor->id_proceso);
					switch(suscriptor->tipo_suscripcion){
						case SUSCRIBE_TEAM:{
							// 4. Envío de un mensaje a un suscriptor específico.
							log_info(logBroker, "NO se debe enviar este Mensaje al Team.");
							log_info(logBrokerInterno, "NO se debe enviar este Mensaje al Team.");
							break;
						}
						case SUSCRIBE_GAMECARD:{
							// 4. Envío de un mensaje a un suscriptor específico.
							log_info(logBroker, "Se envió el Mensaje: %s %s %d %d con ID de Mensaje %d al Game Card %d.", cola, descacheado.nombre_pokemon, descacheado.pos_x, descacheado.pos_y, descacheado.id_mensaje, suscriptor->id_proceso);
							log_info(logBrokerInterno, "Se envió el Mensaje: %s %s %d %d con ID de Mensaje %d al Game Card %d.", cola, descacheado.nombre_pokemon, descacheado.pos_x, descacheado.pos_y, descacheado.id_mensaje, suscriptor->id_proceso);
							confirmacionDeRecepcionGameCard(ack, suscriptor, descacheado.id_mensaje);
							break;
						}
						case SUSCRIBE_GAMEBOY:{
							// 4. Envío de un mensaje a un suscriptor específico.
							log_info(logBroker, "Se envió el Mensaje: %s %s %d %d con ID de Mensaje %d al Game Boy %d.", cola, descacheado.nombre_pokemon, descacheado.pos_x, descacheado.pos_y, descacheado.id_mensaje, suscriptor->id_proceso);
							log_info(logBrokerInterno, "Se envió el Mensaje: %s %s %d %d con ID de Mensaje %d al Game Boy %d.", cola, descacheado.nombre_pokemon, descacheado.pos_x, descacheado.pos_y, descacheado.id_mensaje, suscriptor->id_proceso);
							confirmacionDeRecepcionGameBoy(ack, suscriptor, descacheado.id_mensaje);
							break;
						}
						default: break;
					}
				}
				free(descacheado.nombre_pokemon);
				free(stream);
			}
		}else{
			sem_post(&mx_particiones);
		}
	}
}

void enviarCaughtPokemonCacheados(int socket, t_suscribe* suscriptor){
	int tam_lista = list_size(particiones);
	t_particion* particion_buscada;
	bool me_pase = false;
	for(int i = 0; i < tam_lista && me_pase == false ; i++){
		sem_wait(&mx_particiones);
		if(i >= list_size(particiones)){
			me_pase = true;
			sem_post(&mx_particiones);
			log_warning(logBrokerInterno,"Sali por pasarme de indice en descachear caught");
			continue;
		}
		particion_buscada = list_get(particiones, i);

		if(particion_buscada->libre == 0 && particion_buscada->tipo_mensaje == suscriptor->cola_suscribir){
			void* stream = malloc(particion_buscada->tamanio);
			memcpy(stream, cache + particion_buscada->base, particion_buscada->tamanio);
			sem_post(&mx_particiones);

			bool comparar_id_proceso(void *element){
				uint32_t id_proceso = (uint32_t)suscriptor->id_proceso;
				uint32_t id_nodo = (uint32_t)element;

				if(id_proceso == id_nodo){
					log_warning(logBrokerInterno, "Ya se envió el Mensaje Caught con ID de Mensaje Correlativo [%d] al Suscriptor %d", particion_buscada->id, suscriptor->id_proceso);
					return (true);
				}else return (false);
			}

			if(!list_any_satisfy(particion_buscada->susc_enviados, comparar_id_proceso)){
				t_caught_pokemon descacheado = descachearCaughtPokemon(stream, particion_buscada->id);

				struct timeval time_aux;
				gettimeofday(&time_aux, NULL);
				particion_buscada->time_ultima_ref = time_aux;

				log_info(logBrokerInterno, "Descacheado atrapo pokemon %d", descacheado.atrapo_pokemon);
				log_info(logBrokerInterno, "Descacheado id correlativo %d", descacheado.id_mensaje_correlativo);

					int enviado = enviarCaughtPokemon(socket, descacheado,P_BROKER,0);

					if(enviado == -1){
						log_warning(logBroker, "NO se envió el Mensaje con ID de Mensaje Correlativo %d.", descacheado.id_mensaje_correlativo);
						log_warning(logBrokerInterno, "NO se envió el Mensaje con ID de Mensaje Correlativo %d.", descacheado.id_mensaje_correlativo);
					}else{
						int ack = recibirACK(socket);
						log_info(logBrokerInterno, "ACK %d", ack);

						char* cola = colaParaLogs(particion_buscada->tipo_mensaje);
						list_add(particion_buscada->susc_enviados, (void*)suscriptor->id_proceso);
						switch(suscriptor->tipo_suscripcion){
							case SUSCRIBE_TEAM:{
								// 4. Envío de un mensaje a un suscriptor específico.
								log_info(logBroker, "Se envió el Mensaje: %s %d con ID de Mensaje Correlativo %d al Team %d.", cola, descacheado.atrapo_pokemon, descacheado.id_mensaje_correlativo, suscriptor->id_proceso);
								log_info(logBrokerInterno, "Se envió el Mensaje: %s %d con ID de Mensaje Correlativo %d al Team %d.", cola, descacheado.atrapo_pokemon, descacheado.id_mensaje_correlativo, suscriptor->id_proceso);
								confirmacionDeRecepcionTeam(ack, suscriptor, descacheado.id_mensaje_correlativo);
								break;
							}
							case SUSCRIBE_GAMECARD:{
								log_info(logBroker, "NO se debe enviar este Mensaje al Game Card.");
								log_info(logBrokerInterno, "NO se debe enviar este Mensaje al Game Card.");
								break;
							}
							case SUSCRIBE_GAMEBOY:{
								// 4. Envío de un mensaje a un suscriptor específico.
								log_info(logBroker, "Se envió el Mensaje: %s %d con ID de Mensaje Correlativo %d al Game Boy %d.", cola, descacheado.atrapo_pokemon, descacheado.id_mensaje_correlativo, suscriptor->id_proceso);
								log_info(logBrokerInterno, "Se envió el Mensaje: %s %d con ID de Mensaje Correlativo %d al Game Boy %d.", cola, descacheado.atrapo_pokemon, descacheado.id_mensaje_correlativo, suscriptor->id_proceso);
								confirmacionDeRecepcionGameBoy(ack, suscriptor, descacheado.id_mensaje_correlativo);
								break;			
							}
							default: break;
						}
					}
				
				free(stream);
			}
		}else{
			sem_post(&mx_particiones);
		}
	}
}

void enviarGetPokemonCacheados(int socket, t_suscribe* suscriptor){
	int tam_lista = list_size(particiones);
	t_particion* particion_buscada;
	bool me_pase = false;
	for(int i = 0; i < tam_lista && me_pase == false ; i++){
		sem_wait(&mx_particiones);
		if(i >= list_size(particiones)){
			me_pase = true;
			sem_post(&mx_particiones);
			log_warning(logBrokerInterno,"Sali por pasarme de indice en descachear NEW");
			continue;
		}
		particion_buscada = list_get(particiones, i);

		if(particion_buscada->libre == 0 && particion_buscada->tipo_mensaje == suscriptor->cola_suscribir){
			void* stream = malloc(particion_buscada->tamanio);
			memcpy(stream, cache + particion_buscada->base, particion_buscada->tamanio);
			sem_post(&mx_particiones);
			bool comparar_id_proceso(void *element){
				uint32_t id_proceso = (uint32_t) suscriptor->id_proceso;
				uint32_t id_nodo = (uint32_t) element;

				if(id_proceso == id_nodo){
					log_warning(logBrokerInterno, "Ya se envió el Mensaje Get con ID de Mensaje [%d] al Suscriptor %d", particion_buscada->id, suscriptor->id_proceso);
					return (true);
				}else return (false);
			}

			if(!list_any_satisfy(particion_buscada->susc_enviados, comparar_id_proceso)){
				
				t_get_pokemon* descacheado = descachearGetPokemon(stream, particion_buscada->id);

				struct timeval time_aux;
				gettimeofday(&time_aux, NULL);
				particion_buscada->time_ultima_ref = time_aux;

				log_info(logBrokerInterno, "Descacheado nombre %s", descacheado->nombre_pokemon);
				log_info(logBrokerInterno, "Descacheado id %d", descacheado->id_mensaje);


					int enviado = enviarGetPokemon(socket, *descacheado,P_BROKER,0);

					int ack = recibirACK(socket);
					log_info(logBrokerInterno, "ACK %d", ack);

					char* cola = colaParaLogs(particion_buscada->tipo_mensaje);
					
					if(enviado == -1){
						log_warning(logBroker, "NO se envió el Mensaje con ID de Mensaje %d.", descacheado->id_mensaje);
						log_warning(logBrokerInterno, "NO se envió el Mensaje con ID de Mensaje %d.", descacheado->id_mensaje);
					}else{
						list_add(particion_buscada->susc_enviados, (void*)suscriptor->id_proceso);
						switch(suscriptor->tipo_suscripcion){
							case SUSCRIBE_TEAM:{
								// 4. Envío de un mensaje a un suscriptor específico.
								log_info(logBroker, "NO se debe enviar este Mensaje al Team.");
								log_info(logBrokerInterno, "NO se debe enviar este Mensaje al Team.");
								break;
							}
							case SUSCRIBE_GAMECARD:{
								// 4. Envío de un mensaje a un suscriptor específico.
								log_info(logBroker, "Se envió el Mensaje: %s %s con ID de Mensaje %d.", cola, descacheado->nombre_pokemon, descacheado->id_mensaje);
								log_info(logBrokerInterno, "Se envió el Mensaje: %s %s con ID de Mensaje %d.", cola, descacheado->nombre_pokemon, descacheado->id_mensaje);
								confirmacionDeRecepcionGameCard(ack, suscriptor, descacheado->id_mensaje);
								break;
							}
							case SUSCRIBE_GAMEBOY:{
								// 4. Envío de un mensaje a un suscriptor específico.
								log_info(logBroker, "Se envió el Mensaje: %s %s con ID de Mensaje %d.", cola, descacheado->nombre_pokemon, descacheado->id_mensaje);
								log_info(logBrokerInterno, "Se envió el Mensaje: %s %s con ID de Mensaje %d.", cola, descacheado->nombre_pokemon, descacheado->id_mensaje);
								confirmacionDeRecepcionGameBoy(ack, suscriptor, descacheado->id_mensaje);
								break;
							}
							default: break;
						}
					}
				
				free(descacheado->nombre_pokemon);
				free(descacheado);
				free(stream);
			}
			
		}else{
			sem_post(&mx_particiones);
		}
	}
}

void enviarLocalizedPokemonCacheados(int socket, t_suscribe* suscriptor){
	int tam_lista = list_size(particiones);
	t_particion* particion_buscada;
	bool me_pase = false;
	for(int i = 0; i < tam_lista && me_pase == false ; i++){
		sem_wait(&mx_particiones);
		if(i >= list_size(particiones)){
			me_pase = true;
			sem_post(&mx_particiones);
			log_warning(logBrokerInterno,"Sali por pasarme de indice en descachear LOCALIZED");
			continue;
		}
		particion_buscada = list_get(particiones, i);

		if(particion_buscada->libre == 0 && particion_buscada->tipo_mensaje == suscriptor->cola_suscribir){
			void* stream = malloc(particion_buscada->tamanio);
			memcpy(stream, cache + particion_buscada->base, particion_buscada->tamanio);
			sem_post(&mx_particiones);

			bool comparar_id_proceso(void *element){
				uint32_t id_proceso = (uint32_t)suscriptor->id_proceso;
				uint32_t id_nodo = (uint32_t)element;

				if(id_proceso == id_nodo){
					log_warning(logBrokerInterno, "Ya se envió el Mensaje Localized con ID de Mensaje Correlativo [%d] al Suscriptor %d", particion_buscada->id, suscriptor->id_proceso);
					return (true);
				}else return (false);
			}

			if(!list_any_satisfy(particion_buscada->susc_enviados, comparar_id_proceso)){
				
				t_localized_pokemon descacheado = descachearLocalizedPokemon(stream, particion_buscada->id);

				struct timeval time_aux;
				gettimeofday(&time_aux, NULL);
				particion_buscada->time_ultima_ref = time_aux;

				char* cola = colaParaLogs(particion_buscada->tipo_mensaje);


					int enviado = enviarLocalizedPokemon(socket, descacheado,P_BROKER,0);

					int ack = recibirACK(socket);
					log_info(logBrokerInterno, "ACK %d", ack);
				
				// 4. Envío de un mensaje a un suscriptor específico.
				if (enviado>0){			
					log_info(logBroker, "Se envió el Mensaje: %s %s %d %s con ID de Mensaje Correlativo %d.", cola, descacheado.nombre_pokemon, descacheado.cant_pos, descacheado.posiciones, descacheado.id_mensaje_correlativo);
					log_info(logBrokerInterno, "Se envió el Mensaje: %s %s %d %s con ID de Mensaje Correlativo %d.", cola, descacheado.nombre_pokemon, descacheado.cant_pos, descacheado.posiciones, descacheado.id_mensaje_correlativo);

					log_info(logBrokerInterno, "Descacheado id correlativo %d", descacheado.id_mensaje_correlativo);
				} else 
					log_info(logBrokerInterno, "No se pudo enviar el Mensaje: %s %s %d %s con ID de Mensaje Correlativo %d.", cola, descacheado.nombre_pokemon, descacheado.cant_pos, descacheado.posiciones, descacheado.id_mensaje_correlativo);	
				
				free(stream);
			}
		}else{
			sem_post(&mx_particiones);
		}
	}
}

void confirmacionDeRecepcionTeam(int ack, t_suscribe* suscribe_team, uint32_t id_mensaje){
	if(ack == 1){
		// 5. Confirmación de recepción de un suscriptor al envío de un mensaje previo.
		log_info(logBroker, "Confirmación de Recepción del Mensaje con ID %d del Suscriptor Team %d.", id_mensaje, suscribe_team->id_proceso);
		log_info(logBrokerInterno, "Confirmación de Recepción del Mensaje con ID %d del Suscriptor Team %d.", id_mensaje, suscribe_team->id_proceso);
		// agregar el suscriptor a la lista de los que retornaron ACK
	}else{
		log_info(logBroker, "El Team %d NO recibió el Mensaje con ID %d.", suscribe_team->id_proceso, id_mensaje);
		log_info(logBrokerInterno, "El Team %d NO recibió el Mensaje con ID %d.", suscribe_team->id_proceso, id_mensaje);

		switch(suscribe_team->cola_suscribir){
			case APPEARED_POKEMON: list_add(nodo_appeared->susc_no_ack, (void *)suscribe_team->id_proceso);
				break;
			case CAUGHT_POKEMON: list_add(nodo_caught->susc_no_ack, (void *)suscribe_team->id_proceso);
				break;
			case LOCALIZED_POKEMON: list_add(nodo_localized->susc_no_ack, (void *)suscribe_team->id_proceso);
				break;
			default: break;
		}
	}
}

void confirmacionDeRecepcionGameCard(int ack, t_suscribe* suscribe_gamecard, uint32_t id_mensaje){
	if(ack == 1){
		// 5. Confirmación de recepción de un suscriptor al envío de un mensaje previo.
		log_info(logBroker, "Confirmación de Recepción del Mensaje con ID %d del Suscriptor Game Card %d.", id_mensaje, suscribe_gamecard->id_proceso);
		log_info(logBrokerInterno, "Confirmación de Recepción del Mensaje con ID %d del Suscriptor GameCard %d.", id_mensaje, suscribe_gamecard->id_proceso);
	}else{
		log_info(logBroker, "El Game Card %d NO recibió el Mensaje con ID %d.", suscribe_gamecard->id_proceso, id_mensaje);
		log_info(logBrokerInterno, "El Game Card %d NO recibió el Mensaje con ID %d.", suscribe_gamecard->id_proceso, id_mensaje);

		switch(suscribe_gamecard->cola_suscribir){
			case NEW_POKEMON: list_add(nodo_new->susc_no_ack, (void *)suscribe_gamecard->id_proceso);
				break;
			case CATCH_POKEMON: list_add(nodo_catch->susc_no_ack, (void *)suscribe_gamecard->id_proceso);
				break;
			case GET_POKEMON: list_add(nodo_get->susc_no_ack,(void *)suscribe_gamecard->id_proceso);
				break;
			default: break;
		}
	}
}

void confirmacionDeRecepcionGameBoy(int ack, t_suscribe* suscribe_gameboy, uint32_t id_mensaje){
	if(ack == 1){
		// 5. Confirmación de recepción de un suscriptor al envío de un mensaje previo.
		log_info(logBroker, "Confirmación de Recepción del Mensaje con ID %d del Suscriptor Game Boy %d.", id_mensaje, suscribe_gameboy->id_proceso);
		log_info(logBrokerInterno, "Confirmación de Recepción del Mensaje con ID %d del Suscriptor Game Boy %d.", id_mensaje, suscribe_gameboy->id_proceso);
	}else{
		log_info(logBroker, "El Game Boy %d NO recibió el Mensaje con ID %d.", suscribe_gameboy->id_proceso, id_mensaje);
		log_info(logBrokerInterno, "El Game Boy %d NO recibió el Mensaje con ID %d.", suscribe_gameboy->id_proceso, id_mensaje);
	
		switch(suscribe_gameboy->cola_suscribir){
			case NEW_POKEMON: list_add(nodo_new->susc_no_ack, (void *)suscribe_gameboy->id_proceso);
				break;
			case APPEARED_POKEMON: list_add(nodo_appeared->susc_no_ack,(void *) suscribe_gameboy->id_proceso);
				break;
			case CATCH_POKEMON: list_add(nodo_catch->susc_no_ack, (void *)suscribe_gameboy->id_proceso);
				break;
			case CAUGHT_POKEMON: list_add(nodo_caught->susc_no_ack, (void *)suscribe_gameboy->id_proceso);
				break;
			case GET_POKEMON: list_add(nodo_get->susc_no_ack, (void *)suscribe_gameboy->id_proceso);
				break;
			case LOCALIZED_POKEMON: list_add(nodo_localized->susc_no_ack, (void *)suscribe_gameboy->id_proceso);
				break;
			default: break;
		}
	}
}

int main(void){
	logBroker = log_create("broker.log", "Broker", 0, LOG_LEVEL_TRACE);
	logBrokerInterno = log_create("brokerInterno.log", "Broker Interno", 1, LOG_LEVEL_TRACE);

	log_trace(logBroker, "****************************************** PROCESO BROKER ******************************************");

	inicializarColas();
	inicializarSemaforos();
	inicializarMemoria();

	sem_init(&mx_particiones,0,(unsigned int) 1);

	socketServidorBroker = crearSocketServidor(config_broker->ip_broker, config_broker->puerto_broker);

	if(socketServidorBroker != -1){
		log_info(logBrokerInterno,"Socket Servidor %d.", socketServidorBroker);
		
		signal(SIGUSR1, &controlador_de_seniales);

		while(1){
			cliente = aceptarCliente(socketServidorBroker);

			pthread_t hiloCliente;
			pthread_create(&hiloCliente, NULL, (void*)atenderCliente, (int*)cliente);
			pthread_detach(hiloCliente);
		}

		close(socketServidorBroker);

		log_info(logBrokerInterno, "Se cerró el Socket Servidor %d.", socketServidorBroker);
	}else{
		log_warning(logBrokerInterno, "No se pudo crear el Servidor Broker.");
	}

	sem_destroy(&mx_particiones);

	log_destroy(logBrokerInterno);
	log_destroy(logBroker);
	list_destroy_and_destroy_elements(particiones,destruir_particion);
	return 0;
} 
