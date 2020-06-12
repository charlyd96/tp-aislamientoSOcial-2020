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
		log_error(logBrokerInterno, "ERROR: Verificar path del archivo.\n");
	    return -1;
	}

	config_broker = malloc(sizeof(t_configuracion));
	config_ruta = config_create(pathConfigBroker);

	if (config_ruta != NULL){
	    config_broker->tam_memoria = config_get_int_value(config_ruta, "TAMANO_MEMORIA");
		config_broker->tam_minimo_particion = config_get_int_value(config_ruta, "TAMANO_MINIMO_PARTICION");
	    config_broker->algoritmo_reemplazo = config_get_string_value(config_ruta, "ALGORITMO_REEMPLAZO");;
	    config_broker->ip_broker = config_get_string_value(config_ruta, "IP_BROKER");
	    config_broker->puerto_broker = config_get_string_value(config_ruta, "PUERTO_BROKER");
	    config_broker->frecuencia_compatacion = config_get_int_value(config_ruta, "FRECUENCIA_COMPACTACION");

	    algoritmo_mem = config_get_string_value(config_ruta, "ALGORITMO_MEMORIA");
		if(strcmp(algoritmo_mem,"PD") == 0) config_broker->algoritmo_memoria = PD;
		if(strcmp(algoritmo_mem,"BS") == 0) config_broker->algoritmo_memoria = BUDDY;

		algoritmo_libre = config_get_string_value(config_ruta, "ALGORITMO_PARTICION_LIBRE");
	    if(strcmp(algoritmo_libre,"FF") == 0) config_broker->algoritmo_particion_libre = FF;
	    if(strcmp(algoritmo_libre,"BF") == 0) config_broker->algoritmo_particion_libre = BF;
	}

	if(config_broker == NULL){
	    log_error(logBrokerInterno, "ERROR: No se pudo levantar el archivo de configuración.\n");
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
}

void inicializarMemoria(){
	cache = malloc(config_broker->tam_memoria);
	particiones = list_create();

	//Partición libre inicial
	t_particion* particionInicial = malloc(sizeof(t_particion));
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
 * el campo "id" de la partición depende del tipo de mensaje puede referirse al id_mensaje o id_mensaje_correlativo
 * Esta funcion es para reutilizar en cada cachearTalMensaje()
 */
int buscarParticionYAlocar(int largo_stream,void* stream,op_code tipo_msg,uint32_t id){
	//-> MUTEAR LISTA DE PARTICIONES
	//buscarParticionLibre(largo_stream) devuelve el índice de la particion libre o -1 si no encuentra
	int indice = buscarParticionLibre(largo_stream);
	while (indice < 0) {
		//-> Aplicar algoritmo para hacer espacio
		//Buscar de nuevo
		indice = buscarParticionLibre(largo_stream);
	}
	//Copiar estr. admin. de la particion
	t_particion* part_libre = list_get(particiones, indice);

	//alocar: (cache es un puntero void* al inicio de la caché)
	//cache + base de la part_libre es el puntero al inicio de la partición libre
	memcpy(cache + (part_libre->base), stream, largo_stream);

	//actualizar estructura administrativa (lista de particiones)
	t_particion* part_nueva = malloc(sizeof(t_particion));
	part_nueva->libre = false;
	part_nueva->tipo_mensaje = tipo_msg;
	part_nueva->base = part_libre->base;
	part_nueva->tamanio = largo_stream;
	part_nueva->id = id;

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
	log_info(logBroker,"Partición nueva: indice %d, ubicacion %d, largo %d",indice,part_nueva->base,largo_stream);
	//-> DESMUTEAR LISTA DE PARTICIONES

	return 1;
}

int cachearNewPokemon(t_new_pokemon* msg){
	uint32_t largo_nombre = strlen(msg->nombre_pokemon); //Sin el \0
	uint32_t largo_stream = sizeof(uint32_t) + largo_nombre;

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
	uint32_t largo_stream = sizeof(uint32_t) + largo_nombre;

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
	uint32_t largo_stream = sizeof(uint32_t) + largo_nombre;

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

	void* stream = malloc(largo_stream);

	memcpy(stream, &largo_stream, sizeof(uint32_t));

	int result = buscarParticionYAlocar(largo_stream,stream,CAUGHT_POKEMON,msg->id_mensaje_correlativo);

	return result;
}

int cachearGetPokemon(t_get_pokemon* msg){
	uint32_t largo_nombre = strlen(msg->nombre_pokemon); //Sin el \0
	uint32_t largo_stream = sizeof(uint32_t) + largo_nombre;
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
	uint32_t largo_stream = sizeof(uint32_t) + largo_nombre;

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

/* FUNCIONES - CONEXIÓN */

void atenderCliente(int socket_cliente){
	printf("Atender Cliente %d: \n", socket_cliente);
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
			log_info(logBrokerInterno, "No se pudo conectar ningún proceso.");
			break;
		}
	}
}

void atenderMensajeNewPokemon(int socket_cliente){
	t_new_pokemon* new_pokemon = recibirNewPokemon(socket_cliente);
	log_info(logBroker, "Llegó el mensaje NEW_POKEMON.");

	uint32_t id_mensaje;

	encolarNewPokemon(new_pokemon);

	int enviado = devolverID(socket_cliente,&id_mensaje);
	new_pokemon->id_mensaje = id_mensaje;

	int cacheado = cachearNewPokemon(new_pokemon);
}

void atenderMensajeAppearedPokemon(int socket_cliente){
	t_appeared_pokemon* appeared_pokemon = recibirAppearedPokemon(socket_cliente);
	log_info(logBroker, "Llegó el mensaje APPEARED_POKEMON.");

	uint32_t id_mensaje;

	encolarAppearedPokemon(appeared_pokemon);

	int enviado = devolverID(socket_cliente,&id_mensaje);
//	appeared_pokemon->id_mensaje_correlativo = id_mensaje;

	int cacheado = cachearAppearedPokemon(appeared_pokemon);
}

void atenderMensajeCatchPokemon(int socket_cliente){
	t_catch_pokemon* catch_pokemon = recibirCatchPokemon(socket_cliente);
	log_info(logBroker, "Llegó el mensaje CATCH_POKEMON.");

	uint32_t id_mensaje;

	encolarCatchPokemon(catch_pokemon);

	int enviado = devolverID(socket_cliente,&id_mensaje);
	catch_pokemon->id_mensaje = id_mensaje;

	int cacheado = cachearCatchPokemon(catch_pokemon);
}

void atenderMensajeCaughtPokemon(int socket_cliente){
	t_caught_pokemon* caught_pokemon = recibirCaughtPokemon(socket_cliente);
	log_info(logBroker, "Llegó el mensaje CAUGHT_POKEMON.");

	uint32_t id_mensaje;

	encolarCaughtPokemon(caught_pokemon);

	int enviado = devolverID(socket_cliente,&id_mensaje);
//	caught_pokemon->id_mensaje_correlativo = id_mensaje;

	int cacheado = cachearCaughtPokemon(caught_pokemon);
}

void atenderMensajeGetPokemon(int socket_cliente){
	t_get_pokemon* get_pokemon = recibirGetPokemon(socket_cliente);
	log_info(logBroker, "Llegó el mensaje GET_POKEMON.");

	uint32_t id_mensaje;

	encolarGetPokemon(get_pokemon);

	int enviado = devolverID(socket_cliente,&id_mensaje);
	get_pokemon->id_mensaje = id_mensaje;

	int cacheado = cachearGetPokemon(get_pokemon);
}

void atenderMensajeLocalizedPokemon(int socket_cliente){
	t_localized_pokemon* localized_pokemon = recibirLocalizedPokemon(socket_cliente);
	log_info(logBroker, "Llegó el mensaje LOCALIZED_POKEMON.");

	uint32_t id_mensaje;

	encolarLocalizedPokemon(localized_pokemon);

	int enviado = devolverID(socket_cliente,&id_mensaje);
//	localized_pokemon->id_mensaje_correlativo = id_mensaje;

	int cacheado = cachearLocalizedPokemon(localized_pokemon);

	//hilo-enviarMensajeASuscriptores(mensaje);
	//hilo-recibirACK(socket_cliente);

	//si retorna ACK elimino el mensaje de la COLA
	//si no retorna ACK ?
}

void atenderSuscripcionTeam(int socket_cliente){
	t_suscribe* suscribe_team = recibirSuscripcion(SUSCRIBE_TEAM,socket_cliente);

	int index = suscribir(socket_cliente,suscribe_team->cola_suscribir);
	log_info(logBroker, "Se suscribió al team, indice %d",index);

	//enviar mensajes cacheados (recorrer la lista de particiones)
}

void atenderSuscripcionGameBoy(int socket_cliente){
	t_suscribe* suscribe_gameboy = recibirSuscripcion(SUSCRIBE_GAMEBOY,socket_cliente);

	int index = suscribir(socket_cliente,suscribe_gameboy->cola_suscribir);
	log_info(logBroker, "Se suscribió al Gameboy, indice %d",index);

	sleep(suscribe_gameboy->timeout);
	desuscribir(index,suscribe_gameboy->cola_suscribir);
}

void atenderSuscripcionGameCard(int socket_cliente){
	t_suscribe* suscribe_gamecard = recibirSuscripcion(SUSCRIBE_GAMECARD,socket_cliente);

	int index = suscribir(socket_cliente,suscribe_gamecard->cola_suscribir);
	log_info(logBroker, "Se suscribió al gamecard, indice %d",index);
}

/* FUNCIONES - PROCESAMIENTO */

int suscribir(int socket, op_code cola){
	int index = 0;
	switch(cola){
		case NEW_POKEMON:{
			pthread_mutex_lock(&sem_cola_new);
			index= list_add(cola_new->suscriptores,&socket);
			pthread_mutex_unlock(&sem_cola_new);
			sem_post(&sem_cola_new);
			break;
		}
		case APPEARED_POKEMON:{
			pthread_mutex_lock(&sem_cola_appeared);
			index= list_add(cola_appeared->suscriptores,&socket);
			pthread_mutex_unlock(&sem_cola_appeared);
			sem_post(&sem_cola_appeared);
			break;
		}
		case CATCH_POKEMON:{
			pthread_mutex_lock(&sem_cola_catch);
			index= list_add(cola_catch->suscriptores,&socket);
			pthread_mutex_unlock(&sem_cola_catch);
			sem_post(&sem_cola_catch);
			break;
		}
		case CAUGHT_POKEMON:{
			pthread_mutex_lock(&sem_cola_caught);
			index= list_add(cola_caught->suscriptores,&socket);
			pthread_mutex_unlock(&sem_cola_caught);
			sem_post(&sem_cola_caught);
			break;
		}
		case GET_POKEMON:{
			pthread_mutex_lock(&sem_cola_get);
			index= list_add(cola_get->suscriptores,&socket);
			pthread_mutex_unlock(&sem_cola_get);
			sem_post(&sem_cola_get);
			break;
		}
		case LOCALIZED_POKEMON:{
			pthread_mutex_lock(&sem_cola_localized);
			index= list_add(cola_localized->suscriptores,&socket);
			pthread_mutex_unlock(&sem_cola_localized);
			sem_post(&sem_cola_localized);
			break;
		}
		default:
			index = -1;
			log_info(logBrokerInterno, "No se suscribe ningún proceso.");
	}
	return index;
}
//A optimizar / rediseñar
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
	t_nodo_cola_new* nodo = malloc(sizeof(t_nodo_cola_new));
	nodo->mensaje = mensaje;
	nodo->susc_enviados = list_create();
	nodo->susc_ack = list_create();

	list_add(cola_new->nodos,nodo);
}

void encolarAppearedPokemon(t_appeared_pokemon* mensaje){
	t_nodo_cola_appeared* nodo = malloc(sizeof(t_nodo_cola_appeared));
	nodo->mensaje = mensaje;
	nodo->susc_enviados = list_create();
	nodo->susc_ack = list_create();

	list_add(cola_appeared->nodos,nodo);
}

void encolarCatchPokemon(t_catch_pokemon* mensaje){
	t_nodo_cola_catch* nodo = malloc(sizeof(t_nodo_cola_catch));
	nodo->mensaje = mensaje;
	nodo->susc_enviados = list_create();
	nodo->susc_ack = list_create();

	list_add(cola_catch->nodos,nodo);
}

void encolarCaughtPokemon(t_caught_pokemon* mensaje){
	t_nodo_cola_caught* nodo = malloc(sizeof(t_nodo_cola_caught));
	nodo->mensaje = mensaje;
	nodo->susc_enviados = list_create();
	nodo->susc_ack = list_create();

	list_add(cola_caught->nodos,nodo);
}

void encolarGetPokemon(t_get_pokemon* mensaje){
	t_nodo_cola_get* nodo = malloc(sizeof(t_nodo_cola_get));
	nodo->mensaje = mensaje;
	nodo->susc_enviados = list_create();
	nodo->susc_ack = list_create();

	list_add(cola_get->nodos,nodo);
}

void encolarLocalizedPokemon(t_localized_pokemon* mensaje){
	t_nodo_cola_localized* nodo = malloc(sizeof(t_nodo_cola_localized));
	nodo->mensaje = mensaje;
	nodo->susc_enviados = list_create();
	nodo->susc_ack = list_create();

	list_add(cola_localized->nodos,nodo);
}

/* FUNCIONES - COMUNICACIÓN */

int devolverID(int socket,uint32_t* id_mensaje){
	uint32_t id = ID_MENSAJE ++; //Sincronizar obviously
	(*id_mensaje) = ID_MENSAJE;
	void*stream = malloc(sizeof(uint32_t));

	memcpy(stream, &(id), sizeof(uint32_t));

	int enviado = send(socket, stream, sizeof(uint32_t), 0);

	printf("ID %d\n", id);
	return enviado;
}

int main(void){
	logBroker = log_create("broker.log", "Broker", 0, LOG_LEVEL_INFO);
	logBrokerInterno = log_create("brokerInterno.log", "Broker Interno", 0, LOG_LEVEL_INFO);

	inicializarColas();
	inicializarMemoria();
	inicializarSemaforos();

	socketServidorBroker = crearSocketServidor(config_broker->ip_broker, config_broker->puerto_broker);

	if(socketServidorBroker == -1){
		printf("No se pudo crear el Servidor Broker.");
		return -1;
	}else{
		printf("Socket Servidor %d.\n", socketServidorBroker);
	}

	while(1){
		cliente = aceptarCliente(socketServidorBroker);
		atenderCliente(cliente);
	}

	if(socketServidorBroker != -1){
		close(socketServidorBroker);
		log_info(logBrokerInterno, "Se cerró el Socket Servidor %d.", socketServidorBroker);
	}

	log_destroy(logBrokerInterno);
	log_destroy(logBroker);
}
