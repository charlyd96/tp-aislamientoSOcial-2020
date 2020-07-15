/*
 * broker.c
 *
 *  Created on: 24 abr. 2020
 *      Author: aislamientoSOcial
 */

#include "broker.h"

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
		if(strcmp(algoritmo_mem,"PD") == 0) config_broker->algoritmo_memoria = PD;
		if(strcmp(algoritmo_mem,"BUDDY") == 0) config_broker->algoritmo_memoria = BUDDY;

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

	if(config_broker->algoritmo_memoria == BUDDY){
		//Exponente en base 2 que representa a la memoria (para buddy)
		buddy_U = (uint32_t)ceil(log10(config_broker->tam_memoria)/log10(2));
		log_info(logBrokerInterno,"EL U es %d",buddy_U);
		particionInicial->buddy_i = buddy_U;
	}
	particionInicial->libre = true;
	particionInicial->base = 0;
	particionInicial->tamanio = config_broker->tam_memoria;

	list_add(particiones,particionInicial);
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
		int diferenciaActual = config_broker->tam_memoria;
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
int buscarHuecoBuddy(int i){
	int largo = list_size(particiones);
	for(int x = 0; x < largo; x++){
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
	t_particion* buddy_der = malloc(sizeof(t_particion));
	*buddy_der = *buddy_izq;
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
}
/**
 * algoritmo recursivo para buscar una partición buddy adecuada.
 * se van partiendo a la mitad los buddys de ser necesario
 */
int obtenerHuecoBuddy(int i){
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
int buscarParticionYAlocar(int largo_stream,void* stream,op_code tipo_msg,uint32_t id){
	sem_wait(&mx_particiones);
	int indice = -1;

	if(config_broker->algoritmo_memoria == PD){
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
	if(config_broker->algoritmo_memoria == BUDDY){
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
	if(config_broker->algoritmo_memoria == PD){
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

		part_libre->base = part_libre->base + largo_stream;
		part_libre->tamanio = part_libre->tamanio - largo_stream;

		//Y acá cambiar [{...part_libre...}] por [{nueva},{libre_mas_chica}]
		//Pasos: Insertar la nueva -> [{nueva},{...part_libre...}]
		list_add_in_index(particiones,indice,part_nueva);
		//y luego eliminar/reemplazar la part_libre original
		if(part_libre->tamanio == 0){
			list_remove(particiones,indice+1);
		}else{
			list_replace(particiones,indice+1,part_libre);
		}

		char* cola = colaParaLogs(part_nueva->tipo_mensaje);

		// 6. Almacenado de un mensaje dentro de la memoria (indicando posición de inicio de su partición).
		log_info(logBroker, "Se almacena el Mensaje %s en la Partición con posición de inicio %d (%p).", cola, part_nueva->base, part_nueva->base);
		//log_info(logBrokerInterno, "Se almacena el Mensaje %s en la Partición con posición de inicio %d (%p).", cola, part_nueva->base, part_nueva->base);

		log_info(logBrokerInterno, "ID_MENSAJE %d, asigno partición base %d y tamanio %d",id, part_nueva->base,part_nueva->tamanio);
	}
	if(config_broker->algoritmo_memoria == BUDDY){

		part_libre->libre = false;
		part_libre->tipo_mensaje = tipo_msg;
		part_libre->id = id;
		struct timeval current_time;
		gettimeofday(&current_time, NULL);
		part_libre->time_creacion = current_time; //Hora actual del sistema
		part_libre->time_ultima_ref = current_time; //Hora actual del sistema

		list_replace(particiones,indice,part_libre);
		log_info(logBroker, "ID_MENSAJE %d, asigno partición base %d y i %d",id, part_libre->base,part_libre->buddy_i);
		//log_info(logBrokerInterno, "ID_MENSAJE %d, asigno partición base %d y i %d",id, part_libre->base,part_libre->buddy_i);

		char* cola = colaParaLogs(part_libre->tipo_mensaje);

		// 6. Almacenado de un mensaje dentro de la memoria (indicando posición de inicio de su partición).
		log_info(logBroker, "Se almacena el Mensaje %s en la Partición con posición de inicio %d (%p).", cola, part_libre->base, part_libre->base);
		//log_info(logBrokerInterno, "Se almacena el Mensaje %s en la Partición con posición de inicio %d (%p).", cola, part_libre->base, part_libre->base);
	}
	//-> DESMUTEAR LISTA DE PARTICIONES
	sem_post(&mx_particiones);
	return 1;
}

void liberarParticion(int indice_victima){
	t_particion* part_liberar = list_get(particiones,indice_victima);
	part_liberar->libre = true;
	list_replace(particiones, indice_victima, part_liberar);

	// 7. Eliminado de una partición de memoria (indicado la posición de inicio de la misma).
	log_info(logBroker, "Se elimina la Partición con posición de inicio %d (%p).", part_liberar->base, part_liberar->base);
	log_info(logBroker, "Se elimina la Partición con posición de inicio %d (%p).", part_liberar->base, part_liberar->base);
	//Consolidar

	//Si no es la última partición
	if(indice_victima + 1 < list_size(particiones)){
		t_particion* part_der = list_get(particiones,indice_victima+1);
		if(part_der->libre == true){
			//Fusionar
			part_liberar->tamanio = part_liberar->tamanio + part_der->tamanio;
			list_remove(particiones,indice_victima + 1);
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
			list_remove(particiones,indice_victima -1);
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
		}
		break;
		case LRU:{
			indice_victima = victimaSegunLRU();
		}
		break;
		default:
			log_info(logBrokerInterno,"No matcheo reemplazo");
			return;
		break;
	}
	//Eliminar partición. Si tiene un compañero (buddy) del mismo tamaño y vacío, consolidar
	t_particion* part_liberar = list_get(particiones,indice_victima);
	part_liberar->libre = true;
	uint32_t i = part_liberar->buddy_i;
	list_replace(particiones, indice_victima, part_liberar);

	// 7. Eliminado de una partición de memoria (indicado la posición de inicio de la misma).
	log_info(logBroker, "Se elimina la Partición con posición de inicio %d (%p).", part_liberar->base, part_liberar->base);
	log_info(logBroker, "Se elimina la Partición con posición de inicio %d (%p).", part_liberar->base, part_liberar->base);

	//Consolidar buddys
	bool huboConsolidacion;
	int i_aux = i;
	do{
		huboConsolidacion = false;
		//Si no es la última partición
		if(indice_victima + 1 < list_size(particiones)){
			t_particion* part_der = list_get(particiones,indice_victima+1);
			if(part_der->libre == true && part_der->buddy_i == i_aux){
				//Fusionar
				part_liberar->buddy_i = part_liberar->buddy_i + 1;
				list_remove(particiones,indice_victima + 1);
				// 8. Asociación de bloques (para buddy system), indicar que particiones se asociaron (indicar posición inicio de ambas particiones).
				log_info(logBroker, "Asociación de Particiones: Partición %p con Partición %p.", part_liberar->base, part_der->base);
				log_info(logBrokerInterno,"Se consolida con particion buddy indice %d, i %d",indice_victima+1,i_aux);
				huboConsolidacion = true;
				i_aux++;
			}
		}
		//Si no es la primera partición
		if(indice_victima > 0){
			t_particion* part_izq = list_get(particiones,indice_victima-1);
			if(part_izq->libre == true && part_izq->buddy_i == i_aux){
				//Fusionar
				part_liberar->base = part_izq->base;
				part_liberar->buddy_i = part_liberar->buddy_i + 1;
				list_remove(particiones,indice_victima -1);
				// 8. Asociación de bloques (para buddy system), indicar que particiones se asociaron (indicar posición inicio de ambas particiones).
				log_info(logBroker, "Asociación de Particiones: Partición %p con Partición %p.", part_liberar->base, part_izq->base);
				log_info(logBrokerInterno,"Se consolida con particion indice %d",indice_victima-1);
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
		}
		break;
		case LRU:{
			indice_victima = victimaSegunLRU();
		}
		break;
		default:
			log_info(logBrokerInterno,"No matcheo reemplazo");
		break;
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
	list_clean(particiones);

	uint32_t offset = 0;
	for(int i=0; i < cant_particiones; i++){
		t_particion* part = list_get(lista_aux, i);
		if(part->libre == false){
			memmove(cache+offset,cache+part->base,part->tamanio);
			part->base = offset;
			offset += part->tamanio;
			list_add(particiones,part);
		}
	}
	t_particion* espacio_libre = malloc(sizeof(t_particion));
	espacio_libre->libre   = true;
	espacio_libre->base    = offset;
	espacio_libre->tamanio = config_broker->tam_memoria - offset;
	list_add(particiones,espacio_libre);
}

int cachearNewPokemon(t_new_pokemon* msg){
	uint32_t largo_nombre = strlen(msg->nombre_pokemon); //Sin el \0
	uint32_t largo_stream = 4*sizeof(uint32_t) + largo_nombre;

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

	int result = buscarParticionYAlocar(largo_stream,stream,NEW_POKEMON,msg->id_mensaje);

	return result;
}

int cachearAppearedPokemon(t_appeared_pokemon* msg){
	uint32_t largo_nombre = strlen(msg->nombre_pokemon); //Sin el \0
	uint32_t largo_stream = 3 * sizeof(uint32_t) + largo_nombre;

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

	int result = buscarParticionYAlocar(largo_stream,stream,APPEARED_POKEMON,msg->id_mensaje_correlativo);

	return result;
}

int cachearCatchPokemon(t_catch_pokemon* msg){
	uint32_t largo_nombre = strlen(msg->nombre_pokemon); //Sin el \0
	uint32_t largo_stream = 3 * sizeof(uint32_t) + largo_nombre;

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

	int result = buscarParticionYAlocar(largo_stream,stream,CATCH_POKEMON,msg->id_mensaje);

	return result;
}

int cachearCaughtPokemon(t_caught_pokemon* msg){
	uint32_t largo_stream = sizeof(uint32_t);

	if(largo_stream < config_broker->tam_minimo_particion){
		largo_stream = config_broker->tam_minimo_particion;
	}

	void* stream = malloc(largo_stream);

	memcpy(stream, &(msg->atrapo_pokemon), sizeof(uint32_t));
	
	int result = buscarParticionYAlocar(largo_stream,stream,CAUGHT_POKEMON,msg->id_mensaje_correlativo);

	return result;
}

int cachearGetPokemon(t_get_pokemon* msg){
	uint32_t largo_nombre = strlen(msg->nombre_pokemon); //Sin el \0
	uint32_t largo_stream = sizeof(uint32_t) + largo_nombre;

	if(largo_stream < config_broker->tam_minimo_particion){
		largo_stream = config_broker->tam_minimo_particion;
	}

	//Serializo el msg
	void* stream = malloc(largo_stream);
	uint32_t offset = 0;
	memcpy(stream + offset, &largo_nombre, sizeof(uint32_t));
	offset += sizeof(uint32_t);
	memcpy(stream + offset, msg->nombre_pokemon, largo_nombre);	//Copio "pikachu" sin el \0

	int result = buscarParticionYAlocar(largo_stream,stream,GET_POKEMON,msg->id_mensaje);

	return result;
}

int cachearLocalizedPokemon(t_localized_pokemon* msg){
	uint32_t largo_nombre = strlen(msg->nombre_pokemon); //Sin el \0
	uint32_t largo_stream = 2 * sizeof(uint32_t) + largo_nombre + 2* sizeof(uint32_t) * msg->cant_pos;

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

	uint32_t pos_x, pos_y;
	char** pos_list = string_get_string_as_array(msg->posiciones);
	for(int i=0; i<(msg->cant_pos); i++){
		char** pos_pair = string_split(pos_list[i],"|");
		pos_x = atoi(pos_pair[0]);
		pos_y = atoi(pos_pair[1]);

		memcpy(stream + offset, &(pos_x), sizeof(uint32_t));
		offset += sizeof(uint32_t);
		memcpy(stream + offset, &(pos_y), sizeof(uint32_t));
		offset += sizeof(uint32_t);
	}

	int result = buscarParticionYAlocar(largo_stream,stream,LOCALIZED_POKEMON,msg->id_mensaje_correlativo);

	return result;
}

t_new_pokemon descachearNewPokemon(void* stream, uint32_t id){
	t_new_pokemon mensaje_a_enviar;
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
	memcpy(&(mensaje_a_enviar.cantidad), stream + offset, sizeof(uint32_t));
	offset += sizeof(uint32_t);

	mensaje_a_enviar.id_mensaje = id;

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

t_get_pokemon descachearGetPokemon(void* stream, uint32_t id){
	t_get_pokemon mensaje_a_enviar;
	uint32_t largo_nombre = 0;
	uint32_t offset = 0;

	memcpy(&largo_nombre, stream + offset, sizeof(uint32_t));
	offset += sizeof(uint32_t);

	mensaje_a_enviar.nombre_pokemon = malloc(largo_nombre + 1);
	*(mensaje_a_enviar.nombre_pokemon + largo_nombre) = '\0';

	memcpy(mensaje_a_enviar.nombre_pokemon, stream + offset, largo_nombre);
	offset += largo_nombre;

	mensaje_a_enviar.id_mensaje = id;

	return mensaje_a_enviar;
}

t_localized_pokemon descachearLocalizedPokemon(void* stream, uint32_t id){
	t_localized_pokemon mensaje_a_enviar;
	uint32_t largo_nombre = 0;
	uint32_t offset = 0;

	memcpy(&largo_nombre, stream + offset, sizeof(uint32_t));
	offset += sizeof(uint32_t);

	mensaje_a_enviar.nombre_pokemon = malloc(largo_nombre + 1);
	*(mensaje_a_enviar.nombre_pokemon + largo_nombre) = '\0';

	memcpy(mensaje_a_enviar.nombre_pokemon, stream + offset, largo_nombre);
	offset += largo_nombre;
	memcpy(&(mensaje_a_enviar.cant_pos), stream + offset, sizeof(uint32_t));
	offset += sizeof(uint32_t);

	uint32_t pos_x, pos_y;
	char** pos_list = string_get_string_as_array(mensaje_a_enviar.posiciones);

	for(int i = 1; i <= mensaje_a_enviar.cant_pos; i++){
		char** pos_pair = string_split(pos_list[i],"|");
		pos_x = atoi(pos_pair[0]);
		pos_y = atoi(pos_pair[1]);

		memcpy(&(pos_x), stream + offset, sizeof(uint32_t));
		offset += sizeof(uint32_t);
		memcpy(&(pos_y), stream + offset, sizeof(uint32_t));
		offset += sizeof(uint32_t);
	}

	mensaje_a_enviar.id_mensaje_correlativo = id;

	return mensaje_a_enviar;
}

char* fecha_y_hora_actual(){
	time_t tiempo = time(0);
	struct tm *tlocal = localtime(&tiempo);
	char* output = malloc(128);
	strftime(output,128,"%d/%m/%y %H:%M:%S",tlocal);
	printf("%s\n",output);

	output[127] = '\0';

	return output;
}

void dump_cache(){
	int tam_lista = list_size(particiones);
	t_particion* particion_buscada;

	char* fecha_y_hora = fecha_y_hora_actual();

	FILE* archivo_dump;
	archivo_dump = fopen("archivo_dump.txt", "w+");

	fprintf(archivo_dump, "-----------------------------------------------------------------------------------------------------------------------------------------------------------------------------------\n");
	fprintf(archivo_dump, "Dump: %s\n", fecha_y_hora);

	for (int i = 0; i < tam_lista ; i++){
		log_info(logBrokerInterno, "Estoy en el Dump");

		particion_buscada = list_get(particiones, i);

		if(particion_buscada->libre == 0){
			fprintf(archivo_dump, "Partición %d: %05p - %05p.		[X]		Size: %db		LRU: %ld.%06ld		Cola:%d		ID:%d\n", i, (cache + particion_buscada->base), (cache + particion_buscada->base + particion_buscada->tamanio), particion_buscada->tamanio, particion_buscada->time_ultima_ref.tv_sec, particion_buscada->time_ultima_ref.tv_usec, particion_buscada->tipo_mensaje, particion_buscada->id);
		}else{
			fprintf(archivo_dump, "Partición %d: %05p - %05p.		[L]		Size: %db\n", i, (cache + particion_buscada->base), (cache + particion_buscada->base + particion_buscada->tamanio), particion_buscada->tamanio);
		}
	}

	fprintf(archivo_dump, "-----------------------------------------------------------------------------------------------------------------------------------------------------------------------------------\n");

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
			// 1. Conexión de un proceso al broker.
			log_info(logBroker, "Se conectó un Team.");
			log_info(logBrokerInterno, "Se conectó un Team.");

			atenderMensajeNewPokemon(socket_cliente);
			break;
		}
		case APPEARED_POKEMON:{
			// 1. Conexión de un proceso al broker.
			log_info(logBroker, "Se conectó un Game Card.");
			log_info(logBrokerInterno, "Se conectó un Game Card.");

			atenderMensajeAppearedPokemon(socket_cliente);
			break;
		}
		case CATCH_POKEMON:{
			// 1. Conexión de un proceso al broker.
			log_info(logBroker, "Se conectó un Team.");
			log_info(logBrokerInterno, "Se conectó un Team.");

			atenderMensajeCatchPokemon(socket_cliente);
			break;
		}
		case CAUGHT_POKEMON:{
			// 1. Conexión de un proceso al broker.
			log_info(logBroker, "Se conectó un Game Card.");
			log_info(logBrokerInterno, "Se conectó un Game Card.");

			atenderMensajeCaughtPokemon(socket_cliente);
			break;
		}
		case GET_POKEMON:{
			// 1. Conexión de un proceso al broker.
			log_info(logBroker, "Se conectó un Team.");
			log_info(logBrokerInterno, "Se conectó un Team.");

			atenderMensajeGetPokemon(socket_cliente);
			break;
		}
		case LOCALIZED_POKEMON:{
			// 1. Conexión de un proceso al broker.
			log_info(logBroker, "Se conectó un Game Card.");
			log_info(logBrokerInterno, "Se conectó un Game Card.");

			atenderMensajeLocalizedPokemon(socket_cliente);
			break;
		}
		case SUSCRIBE_TEAM:{
			atenderSuscripcionTeam(socket_cliente);
			break;
		}
		case SUSCRIBE_GAMECARD:{
		//	atenderSuscripcionGameCard(socket_cliente);
			break;
		}
		case SUSCRIBE_GAMEBOY:{
			atenderSuscripcionGameBoy(socket_cliente);
			break;
		}
		default:{
			log_info(logBrokerInterno, "No se pudo conectar ningún proceso.");
			break;
		}
	}
}

void atenderMensajeNewPokemon(int socket_cliente){
	t_new_pokemon* new_pokemon = recibirNewPokemon(socket_cliente);

	// 3. Llegada de un nuevo mensaje a una cola de mensajes.
	log_info(logBroker, "Llegó un Mensaje NEW_POKEMON %s %d %d %d.",new_pokemon->nombre_pokemon,new_pokemon->pos_x,new_pokemon->pos_y,new_pokemon->cantidad);
	log_info(logBrokerInterno, "Llegó un Mensaje NEW_POKEMON %s %d %d %d.",new_pokemon->nombre_pokemon,new_pokemon->pos_x,new_pokemon->pos_y,new_pokemon->cantidad);

	uint32_t id_mensaje;

	encolarNewPokemon(new_pokemon);

	int enviado = devolverID(socket_cliente,&id_mensaje);
	new_pokemon->id_mensaje = id_mensaje;

	int cacheado = cachearNewPokemon(new_pokemon);
}

void atenderMensajeAppearedPokemon(int socket_cliente){
	t_appeared_pokemon* appeared_pokemon = recibirAppearedPokemon(socket_cliente);

	if(appeared_pokemon->id_mensaje_correlativo > 0){
		// 3. Llegada de un nuevo mensaje a una cola de mensajes.
		log_info(logBroker, "Llegó un Mensaje APPEARED_POKEMON %s %d %d %d.",appeared_pokemon->nombre_pokemon,appeared_pokemon->pos_x,appeared_pokemon->pos_y, appeared_pokemon->id_mensaje_correlativo);
		log_info(logBrokerInterno, "Llegó un Mensaje APPEARED_POKEMON %s %d %d %d.",appeared_pokemon->nombre_pokemon,appeared_pokemon->pos_x,appeared_pokemon->pos_y, appeared_pokemon->id_mensaje_correlativo);

	}else{
		// 3. Llegada de un nuevo mensaje a una cola de mensajes.
		log_info(logBroker, "Llegó un Mensaje APPEARED_POKEMON %s %d %d.",appeared_pokemon->nombre_pokemon,appeared_pokemon->pos_x,appeared_pokemon->pos_y);
		log_info(logBrokerInterno, "Llegó un Mensaje APPEARED_POKEMON %s %d %d.",appeared_pokemon->nombre_pokemon,appeared_pokemon->pos_x,appeared_pokemon->pos_y);
	}

	uint32_t id_mensaje;

	encolarAppearedPokemon(appeared_pokemon);

	int enviado = devolverID(socket_cliente,&id_mensaje);
//	appeared_pokemon->id_mensaje_correlativo = id_mensaje;

	int cacheado = cachearAppearedPokemon(appeared_pokemon);
}

void atenderMensajeCatchPokemon(int socket_cliente){
	t_catch_pokemon* catch_pokemon = recibirCatchPokemon(socket_cliente);

	// 3. Llegada de un nuevo mensaje a una cola de mensajes.
	log_info(logBroker, "Llegó un Mensaje CATCH_POKEMON %s %d %d.",catch_pokemon->nombre_pokemon,catch_pokemon->pos_x,catch_pokemon->pos_y);
	log_info(logBrokerInterno, "Llegó un Mensaje CATCH_POKEMON %s %d %d.",catch_pokemon->nombre_pokemon,catch_pokemon->pos_x,catch_pokemon->pos_y);

	uint32_t id_mensaje;

	encolarCatchPokemon(catch_pokemon);

	int enviado = devolverID(socket_cliente,&id_mensaje);
	catch_pokemon->id_mensaje = id_mensaje;

	int cacheado = cachearCatchPokemon(catch_pokemon);
}

void atenderMensajeCaughtPokemon(int socket_cliente){
	t_caught_pokemon* caught_pokemon = recibirCaughtPokemon(socket_cliente);

	if(caught_pokemon->id_mensaje_correlativo >= 0){
		// 3. Llegada de un nuevo mensaje a una cola de mensajes.
		log_info(logBroker, "Llegó un Mensaje CAUGHT_POKEMON %d %d.",caught_pokemon->id_mensaje_correlativo,caught_pokemon->atrapo_pokemon);
		log_info(logBrokerInterno, "Llegó un Mensaje CAUGHT_POKEMON %d %d.",caught_pokemon->id_mensaje_correlativo,caught_pokemon->atrapo_pokemon);
	}else{
		// 3. Llegada de un nuevo mensaje a una cola de mensajes.
		log_info(logBroker, "Llegó un Mensaje CAUGHT_POKEMON %d.",caught_pokemon->atrapo_pokemon);
		log_info(logBrokerInterno, "Llegó un Mensaje CAUGHT_POKEMON %d.",caught_pokemon->atrapo_pokemon);
	}

	uint32_t id_mensaje;

	encolarCaughtPokemon(caught_pokemon);

	int enviado = devolverID(socket_cliente,&id_mensaje);
	//caught_pokemon->id_mensaje_correlativo = id_mensaje;

	int cacheado = cachearCaughtPokemon(caught_pokemon);

	int tam_lista_suscriptores = list_size(cola_caught->suscriptores);
	
	for(int j = 0; j < tam_lista_suscriptores; j++){
		t_suscriptor* suscriptor = list_get(cola_caught->suscriptores, j);
		enviarCaughtPokemon(suscriptor->socket_suscriptor, *caught_pokemon);
	}
}

void atenderMensajeGetPokemon(int socket_cliente){
	t_get_pokemon* get_pokemon = recibirGetPokemon(socket_cliente);

	// 3. Llegada de un nuevo mensaje a una cola de mensajes.
	log_info(logBroker, "Llegó un Mensaje GET_POKEMON %s.",get_pokemon->nombre_pokemon);
	log_info(logBrokerInterno, "Llegó un Mensaje GET_POKEMON %s.",get_pokemon->nombre_pokemon);
	uint32_t id_mensaje;

	encolarGetPokemon(get_pokemon);

	int enviado = devolverID(socket_cliente,&id_mensaje);
	get_pokemon->id_mensaje = id_mensaje;

	int cacheado = cachearGetPokemon(get_pokemon);
}

void atenderMensajeLocalizedPokemon(int socket_cliente){
	t_localized_pokemon* localized_pokemon = recibirLocalizedPokemon(socket_cliente);

	// 3. Llegada de un nuevo mensaje a una cola de mensajes.
	log_info(logBroker, "Llegó un Mensaje LOCALIZED_POKEMON.");
	log_info(logBrokerInterno, "Llegó un Mensaje LOCALIZED_POKEMON.");

	uint32_t id_mensaje;

	encolarLocalizedPokemon(localized_pokemon);

	int enviado = devolverID(socket_cliente,&id_mensaje);
//	localized_pokemon->id_mensaje_correlativo = id_mensaje;

	int cacheado = cachearLocalizedPokemon(localized_pokemon);
}

void atenderSuscripcionTeam(int socket_cliente){
	t_suscribe* suscribe_team = recibirSuscripcion(SUSCRIBE_TEAM,socket_cliente);

	t_suscriptor* suscriptor = malloc(sizeof(t_suscriptor));
	suscriptor->socket_suscriptor = socket_cliente;
	suscriptor->id_suscriptor = suscribe_team->id_proceso;

	suscribir(suscriptor, suscribe_team->cola_suscribir);

	char* cola = colaParaLogs(suscribe_team->cola_suscribir);

	// 2. Suscripción de un proceso a una cola de mensajes.
	log_info(logBroker, "Se suscribe el Team %d a la Cola de Mensajes %s", suscribe_team->id_proceso, cola);
	log_info(logBrokerInterno, "Se suscribe el Team %d a la Cola de Mensajes %s",  suscribe_team->id_proceso, cola);

	switch(suscribe_team->cola_suscribir){
		case APPEARED_POKEMON:{
			enviarAppearedPokemonCacheados(socket_cliente, suscribe_team->cola_suscribir);
		//	confirmacionDeRecepcionTeam(socket_cliente, suscribe_team->id_proceso);
			break;
		}
		case CAUGHT_POKEMON:{
			enviarCaughtPokemonCacheados(socket_cliente, suscribe_team->cola_suscribir);
		//	confirmacionDeRecepcionTeam(socket_cliente, suscribe_team->id_proceso);
			break;
		}
		case LOCALIZED_POKEMON:{
			enviarLocalizedPokemonCacheados(socket_cliente, suscribe_team->cola_suscribir);
		//	confirmacionDeRecepcionTeam(socket_cliente, suscribe_team->id_proceso);
			break;
		}
		default:{
			log_info(logBrokerInterno, "No se pudo enviar mensajes cacheados.");
			break;
		}
	}
} 

/*void atenderSuscripcionGameCard(int socket_cliente){
	t_suscribe* suscribe_gamecard = recibirSuscripcion(SUSCRIBE_GAMECARD,socket_cliente);

	int index = suscribir(socket_cliente,suscribe_gamecard->cola_suscribir);

	char* cola = colaParaLogs(suscribe_gamecard->cola_suscribir);

	// 2. Suscripción de un proceso a una cola de mensajes.
	log_info(logBroker, "Se suscribe el Game Card %d a la Cola de Mensajes %s", suscribe_gamecard->id_proceso, cola);
	log_info(logBrokerInterno, "Se suscribe el Game Card %d a la Cola de Mensajes %s", suscribe_gamecard->id_proceso, cola);

	switch(suscribe_gamecard->cola_suscribir){
		case NEW_POKEMON:{
			enviarNewPokemonCacheados(socket_cliente, suscribe_gamecard->cola_suscribir);
		//	confirmacionDeRecepcionGameCard(socket_cliente, suscribe_gamecard->id_proceso);
			break;
		}
		case CATCH_POKEMON:{
			enviarCatchPokemonCacheados(socket_cliente, suscribe_gamecard->cola_suscribir);
		//	confirmacionDeRecepcionGameCard(socket_cliente, suscribe_gamecard->id_proceso);
			break;
		}
		case GET_POKEMON:{
			enviarGetPokemonCacheados(socket_cliente, suscribe_gamecard->cola_suscribir);
		//	confirmacionDeRecepcionGameCard(socket_cliente, suscribe_gamecard->id_proceso);
			break;
		}
		default:{
			log_info(logBrokerInterno, "No se pudo enviar mensajes cacheados.");
			break;
		}
	}
} */

void atenderSuscripcionGameBoy(int socket_cliente){
	t_suscribe* suscribe_gameboy = recibirSuscripcion(SUSCRIBE_GAMEBOY,socket_cliente);

	int index = suscribir(socket_cliente,suscribe_gameboy->cola_suscribir);

	char* cola = colaParaLogs(suscribe_gameboy->cola_suscribir);

	// 2. Suscripción de un proceso a una cola de mensajes.
	log_info(logBroker, "Se suscribe el Game Boy a la Cola de Mensajes %s", cola);
	log_info(logBrokerInterno, "Se suscribe el Game Boy a la Cola de Mensajes %s", cola);

	switch(suscribe_gameboy->cola_suscribir){
		case NEW_POKEMON:{
			enviarNewPokemonCacheados(socket_cliente, suscribe_gameboy->cola_suscribir);
			break;
		}
		case APPEARED_POKEMON:{
			enviarAppearedPokemonCacheados(socket_cliente, suscribe_gameboy->cola_suscribir);
			break;
		}
		case CATCH_POKEMON:{
			enviarCatchPokemonCacheados(socket_cliente, suscribe_gameboy->cola_suscribir);
			break;
		}
		case CAUGHT_POKEMON:{
			enviarCaughtPokemonCacheados(socket_cliente, suscribe_gameboy->cola_suscribir);
			break;
		}
		case GET_POKEMON:{
			enviarGetPokemonCacheados(socket_cliente, suscribe_gameboy->cola_suscribir);
			break;
		}
		/*case LOCALIZED_POKEMON:{
			enviarLocalizedPokemonCacheados(socket_cliente, suscribe_gameboy->cola_suscribir);
			break;
		}*/
		default:{
			log_info(logBrokerInterno, "No se pudo enviar mensajes cacheados.");
			break;
		}
	}

	sleep(suscribe_gameboy->timeout);
	desuscribir(index,suscribe_gameboy->cola_suscribir);
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
			log_info(logBrokerInterno, "No se suscribe ningún proceso.");
	}
	return index;
}

void desuscribir(int index,op_code cola){
	switch(cola){
		case NEW_POKEMON:{
			sem_wait(&mensajes_new); // No se puede sacar si la Cola de Mensajes está vacía
			pthread_mutex_lock(&sem_cola_new);
			list_remove(cola_new->suscriptores,index);
			pthread_mutex_unlock(&sem_cola_new);
			break;
		}
		case APPEARED_POKEMON:{
			sem_wait(&mensajes_appeared);
			pthread_mutex_lock(&sem_cola_appeared);
			list_remove(cola_appeared->suscriptores,index);
			pthread_mutex_unlock(&sem_cola_appeared);
			break;
		}
		case CATCH_POKEMON:{
			sem_wait(&mensajes_catch);
			pthread_mutex_lock(&sem_cola_catch);
			list_remove(cola_catch->suscriptores,index);
			pthread_mutex_unlock(&sem_cola_catch);
			break;
		}
		case CAUGHT_POKEMON:{
			sem_wait(&mensajes_caught);
			pthread_mutex_lock(&sem_cola_caught);
			list_remove(cola_caught->suscriptores,index);
			pthread_mutex_unlock(&sem_cola_caught);
			break;
		}
		case GET_POKEMON:{
			sem_wait(&mensajes_get);
			pthread_mutex_lock(&sem_cola_get);
			list_remove(cola_get->suscriptores,index);
			pthread_mutex_unlock(&sem_cola_get);
			break;
		}
		case LOCALIZED_POKEMON:{
			sem_wait(&mensajes_localized);
			pthread_mutex_lock(&sem_cola_localized);
			list_remove(cola_localized->suscriptores,index);
			pthread_mutex_unlock(&sem_cola_localized);
			break;
		}
		default:
			break;
	}
}

void encolarNewPokemon(t_new_pokemon* mensaje){
	nodo_new = malloc(sizeof(t_nodo_cola_new));
	nodo_new->mensaje = mensaje;
	nodo_new->susc_enviados = list_create();
	nodo_new->susc_ack = list_create();

	list_add(cola_new->nodos,nodo_new);
}

void encolarAppearedPokemon(t_appeared_pokemon* mensaje){
	nodo_appeared = malloc(sizeof(t_nodo_cola_appeared));
	nodo_appeared->mensaje = mensaje;
	nodo_appeared->susc_enviados = list_create();
	nodo_appeared->susc_ack = list_create();

	list_add(cola_appeared->nodos,nodo_appeared);
}

void encolarCatchPokemon(t_catch_pokemon* mensaje){
	nodo_catch = malloc(sizeof(t_nodo_cola_catch));
	nodo_catch->mensaje = mensaje;
	nodo_catch->susc_enviados = list_create();
	nodo_catch->susc_ack = list_create();

	list_add(cola_catch->nodos,nodo_catch);
}

void encolarCaughtPokemon(t_caught_pokemon* mensaje){
	nodo_caught = malloc(sizeof(t_nodo_cola_caught));
	nodo_caught->mensaje = mensaje;
	nodo_caught->susc_enviados = list_create();
	nodo_caught->susc_ack = list_create();

	list_add(cola_caught->nodos,nodo_caught);
}

void encolarGetPokemon(t_get_pokemon* mensaje){
	nodo_get = malloc(sizeof(t_nodo_cola_get));
	nodo_get->mensaje = mensaje;
	nodo_get->susc_enviados = list_create();
	nodo_get->susc_ack = list_create();

	list_add(cola_get->nodos,nodo_get);
}

void encolarLocalizedPokemon(t_localized_pokemon* mensaje){
	nodo_localized = malloc(sizeof(t_nodo_cola_localized));
	nodo_localized->mensaje = mensaje;
	nodo_localized->susc_enviados = list_create();
	nodo_localized->susc_ack = list_create();

	list_add(cola_localized->nodos,nodo_localized);
}

/* FUNCIONES - COMUNICACIÓN */

int devolverID(int socket,uint32_t* id_mensaje){
	sem_wait(&identificador);
	ID_MENSAJE ++;
	uint32_t id = ID_MENSAJE;
	sem_post(&identificador);

	(*id_mensaje) = id;

	void*stream = malloc(sizeof(uint32_t));

	memcpy(stream, &(id), sizeof(uint32_t));

	int enviado = send(socket, stream, sizeof(uint32_t), 0);

	log_info(logBrokerInterno,"ID mensaje asignado %d", id);
	return enviado;
}

void enviarNewPokemonCacheados(int socket, op_code tipo_mensaje){
	int tam_lista = list_size(particiones);
	t_particion* particion_buscada;

	for(int i = 0; i < tam_lista ; i++){
		particion_buscada = list_get(particiones, i);

		if(particion_buscada->libre == 0 && particion_buscada->tipo_mensaje == tipo_mensaje){
			void* stream = malloc(particion_buscada->tamanio);
			memcpy(stream, cache + particion_buscada->base, particion_buscada->tamanio);

			t_new_pokemon descacheado = descachearNewPokemon(stream, particion_buscada->id);

			log_info(logBrokerInterno, "Descacheado nombre %s", descacheado.nombre_pokemon);
			log_info(logBrokerInterno, "Descacheado pos x %d", descacheado.pos_x);
			log_info(logBrokerInterno, "Descacheado pos y %d", descacheado.pos_y);
			log_info(logBrokerInterno, "Descacheado cantidad %d", descacheado.cantidad);
			log_info(logBrokerInterno, "Descacheado id mensaje x %d", descacheado.id_mensaje);

			//Actualizo time ultima ref
			struct timeval time_aux;
			gettimeofday(&time_aux, NULL);
			particion_buscada->time_ultima_ref = time_aux;

			int enviado = enviarNewPokemon(socket, descacheado);

			int ack = recibirACK(socket);
			log_info(logBrokerInterno, "ACK %d", ack);

			list_add(nodo_new->susc_enviados, socket);

			char* cola = colaParaLogs(particion_buscada->tipo_mensaje);

			if(enviado == -1){
				log_error(logBroker, "NO se envió el Mensaje.");
				log_error(logBrokerInterno, "NO se envió el Mensaje.");
			}else{
				// 4. Envío de un mensaje a un suscriptor específico.
				log_info(logBroker, "Se envió el Mensaje: %s %s %d %d %d con ID de Mensaje %d.", cola, descacheado.nombre_pokemon, descacheado.pos_x, descacheado.pos_y, descacheado.cantidad, descacheado.id_mensaje);
				log_info(logBrokerInterno, "Se envió el Mensaje: %s %s %d %d %d con ID de Mensaje %d.", cola, descacheado.nombre_pokemon, descacheado.pos_x, descacheado.pos_y, descacheado.cantidad, descacheado.id_mensaje);
			}
		}
	}
}

void enviarAppearedPokemonCacheados(int socket, op_code tipo_mensaje){
	int tam_lista = list_size(particiones);
	t_particion* particion_buscada;

	for(int i = 0; i < tam_lista ; i++){
		particion_buscada = list_get(particiones, i);

		if(particion_buscada->libre == 0 && particion_buscada->tipo_mensaje == tipo_mensaje){
			void* stream = malloc(particion_buscada->tamanio);
			memcpy(stream, cache + particion_buscada->base, particion_buscada->tamanio);

			t_appeared_pokemon descacheado = descachearAppearedPokemon(stream, particion_buscada->id);

			log_info(logBrokerInterno, "Descacheado nombre %s", descacheado.nombre_pokemon);
			log_info(logBrokerInterno, "Descacheado pos x %d", descacheado.pos_x);
			log_info(logBrokerInterno, "Descacheado pos y %d", descacheado.pos_y);
			log_info(logBrokerInterno, "Descacheado id correlativo %d", descacheado.id_mensaje_correlativo);
			//Actualizo time ultima ref
			struct timeval time_aux;
			gettimeofday(&time_aux, NULL);
			particion_buscada->time_ultima_ref = time_aux;

			int enviado = enviarAppearedPokemon(socket, descacheado);

			int ack = recibirACK(socket);
			log_info(logBrokerInterno, "ACK %d", ack);

			list_add(nodo_appeared->susc_enviados, socket);

			char* cola = colaParaLogs(particion_buscada->tipo_mensaje);

			if(enviado == -1){
				log_info(logBroker, "NO se envia");
				log_info(logBrokerInterno, "NO se envia");
			}else{
				// 4. Envío de un mensaje a un suscriptor específico.
				log_info(logBroker, "Se envió el Mensaje: %s %s %d %d con ID de Mensaje Correlativo %d.", cola, descacheado.nombre_pokemon, descacheado.pos_x, descacheado.pos_y, descacheado.id_mensaje_correlativo);
				log_info(logBrokerInterno, "Se envió el Mensaje: %s %s %d %d con ID de Mensaje Correlativo %d.", cola, descacheado.nombre_pokemon, descacheado.pos_x, descacheado.pos_y, descacheado.id_mensaje_correlativo);
			}
		}
	}
}

void enviarCatchPokemonCacheados(int socket, op_code tipo_mensaje){
	int tam_lista = list_size(particiones);
	t_particion* particion_buscada;

	for(int i = 0; i < tam_lista ; i++){
		particion_buscada = list_get(particiones, i);

		if(particion_buscada->libre == 0 && particion_buscada->tipo_mensaje == tipo_mensaje){
			void* stream = malloc(particion_buscada->tamanio);
			memcpy(stream, cache + particion_buscada->base, particion_buscada->tamanio);

			t_catch_pokemon descacheado = descachearCatchPokemon(stream, particion_buscada->id);

			log_info(logBrokerInterno, "Descacheado nombre %s", descacheado.nombre_pokemon);
			log_info(logBrokerInterno, "Descacheado pos x %d", descacheado.pos_x);
			log_info(logBrokerInterno, "Descacheado pos y %d", descacheado.pos_y);
			log_info(logBrokerInterno, "Descacheado id %d", descacheado.id_mensaje);
			//Actualizo time ultima ref
			struct timeval time_aux;
			gettimeofday(&time_aux, NULL);
			particion_buscada->time_ultima_ref = time_aux;

			int enviado = enviarCatchPokemon(socket, descacheado);

			int ack = recibirACK(socket);
			log_info(logBrokerInterno, "ACK %d", ack);

			list_add(nodo_catch->susc_enviados, socket);

			char* cola = colaParaLogs(particion_buscada->tipo_mensaje);

			if(enviado == -1){
				log_error(logBroker, "NO se envió el Mensaje.");
				log_error(logBrokerInterno, "NO se envió el Mensaje.");
			}else{
				// 4. Envío de un mensaje a un suscriptor específico.
				log_info(logBroker, "Se envió el Mensaje: %s %s %d %d con ID de Mensaje %d.", cola, descacheado.nombre_pokemon, descacheado.pos_x, descacheado.pos_y, descacheado.id_mensaje);
				log_info(logBrokerInterno, "Se envió el Mensaje: %s %s %d %d con ID de Mensaje %d.", cola, descacheado.nombre_pokemon, descacheado.pos_x, descacheado.pos_y, descacheado.id_mensaje);
			}
		}
	}
}

void enviarCaughtPokemonCacheados(int socket, op_code tipo_mensaje){
	int tam_lista = list_size(particiones);
	t_particion* particion_buscada;

	for(int i = 0; i < tam_lista ; i++) {
		particion_buscada = list_get(particiones, i);

		if(particion_buscada->libre == 0 && particion_buscada->tipo_mensaje == tipo_mensaje){
			struct timeval time_aux;
			gettimeofday(&time_aux, NULL);
			particion_buscada->time_ultima_ref = time_aux;

			void* stream = malloc(particion_buscada->tamanio);
			memcpy(stream, cache + particion_buscada->base, particion_buscada->tamanio);

			t_caught_pokemon descacheado = descachearCaughtPokemon(stream, particion_buscada->id);

			log_info(logBrokerInterno, "Descacheado atrapo pokemon %d", descacheado.atrapo_pokemon);
			log_info(logBrokerInterno, "Descacheado id correlativo %d", descacheado.id_mensaje_correlativo);

			int enviado = enviarCaughtPokemon(socket, descacheado);

			int ack = recibirACK(socket);
			log_info(logBrokerInterno, "ACK %d", ack);

			list_add(nodo_caught->susc_enviados, socket);

			char* cola = colaParaLogs(particion_buscada->tipo_mensaje);

			if(enviado == -1){
				log_error(logBroker, "NO se envió el Mensaje.");
				log_error(logBrokerInterno, "NO se envió el Mensaje.");
			}else{
				// 4. Envío de un mensaje a un suscriptor específico.
				log_info(logBroker, "Se envió el Mensaje: %s %d con ID de Mensaje Correlativo %d.", cola, descacheado.atrapo_pokemon, descacheado.id_mensaje_correlativo);
				log_info(logBrokerInterno, "Se envió el Mensaje: %s %d con ID de Mensaje Correlativo %d.", cola, descacheado.atrapo_pokemon, descacheado.id_mensaje_correlativo);
			}
		}
	}
}

void enviarGetPokemonCacheados(int socket, op_code tipo_mensaje){
	int tam_lista = list_size(particiones);
	t_particion* particion_buscada;

	for(int i = 0; i < tam_lista ; i++) {
		particion_buscada = list_get(particiones, i);

		if(particion_buscada->libre == 0 && particion_buscada->tipo_mensaje == tipo_mensaje){
			void* stream = malloc(particion_buscada->tamanio);
			memcpy(stream, cache + particion_buscada->base, particion_buscada->tamanio);

			t_get_pokemon descacheado = descachearGetPokemon(stream, particion_buscada->id);

			log_info(logBrokerInterno, "Descacheado nombre %s", descacheado.nombre_pokemon);
			log_info(logBrokerInterno, "Descacheado id %d", descacheado.id_mensaje);

			//Actualizo time ultima ref
			struct timeval time_aux;
			gettimeofday(&time_aux, NULL);
			particion_buscada->time_ultima_ref = time_aux;

			int enviado = enviarGetPokemon(socket, descacheado);

			int ack = recibirACK(socket);
			log_info(logBrokerInterno, "ACK %d", ack);

			list_add(nodo_get->susc_enviados, socket);

			char* cola = colaParaLogs(particion_buscada->tipo_mensaje);
			
			if(enviado == -1){
				log_error(logBroker, "NO se envió el Mensaje.");
				log_error(logBrokerInterno, "NO se envió el Mensaje.");
			}else{
				// 4. Envío de un mensaje a un suscriptor específico.
				log_info(logBroker, "Se envió el Mensaje: %s %s con ID de Mensaje %d.", cola, descacheado.nombre_pokemon, descacheado.id_mensaje);
				log_info(logBrokerInterno, "Se envió el Mensaje: %s %s con ID de Mensaje %d.", cola, descacheado.nombre_pokemon, descacheado.id_mensaje);
			}
		}
	}
}

void enviarLocalizedPokemonCacheados(int socket, op_code tipo_mensaje){
	int tam_lista = list_size(particiones);
	t_particion* particion_buscada;

	for(int i = 0; i < tam_lista ; i++) {
		particion_buscada = list_get(particiones, i);

		if(particion_buscada->libre == 0 && particion_buscada->tipo_mensaje == tipo_mensaje){
			void* stream = malloc(particion_buscada->tamanio);
			memcpy(stream, cache + particion_buscada->base, particion_buscada->tamanio);

			t_localized_pokemon descacheado = descachearLocalizedPokemon(stream, particion_buscada->id);

			log_info(logBrokerInterno, "Descacheado nombre %s", descacheado.nombre_pokemon);
			log_info(logBrokerInterno, "Decacsheado cant %d", descacheado.cant_pos);

			uint32_t pos_x, pos_y;
			char** pos_list = string_get_string_as_array(descacheado.posiciones);

			for(int i = 1; i <= descacheado.cant_pos; i++){
				char** pos_pair = string_split(pos_list[i],"|");
				pos_x = atoi(pos_pair[0]);
				pos_y = atoi(pos_pair[1]);

				log_info(logBrokerInterno, "Descacheado pos x %d", pos_x);
				log_info(logBrokerInterno, "Descacheado pos y %d", pos_y);

				char* cola = colaParaLogs(particion_buscada->tipo_mensaje);

				// 4. Envío de un mensaje a un suscriptor específico.
				log_info(logBroker, "Se envió el Mensaje: %s %s %d %d %d con ID de Mensaje Correlativo %d.", cola, descacheado.nombre_pokemon, descacheado.cant_pos, pos_x, pos_y, descacheado.id_mensaje_correlativo);
				log_info(logBrokerInterno, "Se envió el Mensaje: %s %s %d %d %d con ID de Mensaje Correlativo %d.", cola, descacheado.nombre_pokemon, descacheado.cant_pos, pos_x, pos_y, descacheado.id_mensaje_correlativo);
			}

			log_info(logBrokerInterno, "Descacheado id correlativo %d", descacheado.id_mensaje_correlativo);

			//Actualizo time ultima ref
			struct timeval time_aux;
			gettimeofday(&time_aux, NULL);
			particion_buscada->time_ultima_ref = time_aux;

			enviarLocalizedPokemon(socket, descacheado);

			list_add(nodo_localized->susc_enviados, socket);

			int ack = recibirACK(socket);
			log_info(logBrokerInterno, "ACK %d", ack);
		}
	}
}

void confirmacionDeRecepcionTeam(int socket, t_suscribe* suscribe_team){
	int ack = recibirACK(socket);
	log_info(logBrokerInterno, "ACK %d", ack);
	if(recibirACK(socket) == 1){
		// 5. Confirmación de recepción de un suscriptor al envío de un mensaje previo.
		log_info(logBroker, "Confirmación de Recepción del Mensaje del Suscriptor Team %d.", suscribe_team->id_proceso);
		log_info(logBrokerInterno, "Confirmación de Recepción del Mensaje del Suscriptor Team %d.", suscribe_team->id_proceso);
		// agregar el suscriptor a la lista de los que retornaron ACK
	}else{
		log_info(logBroker, "El Team %d NO recibió el Mensaje.", suscribe_team->id_proceso);
		log_info(logBrokerInterno, "El Team %d NO recibió el Mensaje.", suscribe_team->id_proceso);
	}
}

void confirmacionDeRecepcionGameCard(int socket, t_suscribe* suscribe_gamecard){
	if(recibirACK(socket) == 1){
		// 5. Confirmación de recepción de un suscriptor al envío de un mensaje previo.
		log_info(logBroker, "Confirmación de Recepción del Mensaje del Suscriptor Game Card %d.", suscribe_gamecard->id_proceso);
		log_info(logBrokerInterno, "Confirmación de Recepción del Mensaje del Suscriptor GameCard %d.", suscribe_gamecard->id_proceso);
	}else{
		log_info(logBroker, "El Game Card %d NO recibió el Mensaje.", suscribe_gamecard->id_proceso);
		log_info(logBrokerInterno, "El Game Card %d NO recibió el Mensaje.", suscribe_gamecard->id_proceso);
	}
}

int main(void){

	logBroker = log_create("broker.log", "Broker", 1, LOG_LEVEL_INFO);
	logBrokerInterno = log_create("brokerInterno.log", "Broker Interno", 1, LOG_LEVEL_INFO);

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
		log_info(logBrokerInterno, "No se pudo crear el Servidor Broker.");
	}

	sem_destroy(&mx_particiones);

	log_destroy(logBrokerInterno);
	log_destroy(logBroker);

	return 0;
} 
