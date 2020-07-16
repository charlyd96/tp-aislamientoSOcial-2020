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

	    algoritmo_mem = config_get_string_value(config_ruta, "ALGORITMO_MEMORIA");
		if(strcmp(algoritmo_mem,"PD") == 0) config_broker->algoritmo_memoria = PD;
		if(strcmp(algoritmo_mem,"BS") == 0) config_broker->algoritmo_memoria = BUDDY;

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
 * el campo "id" de la partición depende del tipo de mensaje puede referirse al id_mensaje o id_mensaje_correlativo
 * Esta funcion es para reutilizar en cada cachearTalMensaje()
 */
int buscarParticionYAlocar(int largo_stream,void* stream,op_code tipo_msg,uint32_t id){
	sem_wait(&mx_particiones);
	//buscarParticionLibre(largo_stream) devuelve el índice de la particion libre o -1 si no encuentra
	int indice = buscarParticionLibre(largo_stream);
	int cant_intentos_fallidos = 0;
	log_info(logBrokerInterno,"indice %d",indice);
	while (indice < 0) {
		cant_intentos_fallidos++;
		if(cant_intentos_fallidos < config_broker->frecuencia_compatacion){
			log_info(logBrokerInterno,"Entro a eliminar particion");
			eliminarParticion();
		}else{
			log_info(logBrokerInterno,"Compactar");
			compactarParticiones();
			cant_intentos_fallidos = 0;
		}
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
	part_nueva->time_creacion = time(0); //Hora actual del sistema
	part_nueva->time_ultima_ref = time(0); //Hora actual del sistema

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

	// 6. Almacenado de un mensaje dentro de la memoria (indicando posición de inicio de su partición).
	log_info(logBroker, "Nuevo mensaje %d con inicio de su partición en %d y tamanio %d", part_nueva->tipo_mensaje, part_nueva->base,part_nueva->tamanio);

	//-> DESMUTEAR LISTA DE PARTICIONES
	sem_post(&mx_particiones);
	return 1;
}

void eliminarParticion(){
	t_algoritmo_reemplazo algoritmo = config_broker->algoritmo_reemplazo;
	int indice_victima = 0;
	int cant_particiones = list_size(particiones);
	time_t time_aux = time(0);
	switch(algoritmo){
		case FIFO:{
			log_info(logBrokerInterno,"Reemplazo con fifo");
			//Eliminar la partición con con time_creación más viejo (el long int menor)
			for(int i=0; i<cant_particiones; i++){
				t_particion* part = list_get(particiones,i);
				//Si se creó antes que time_aux
				if(part->time_creacion < time_aux && part->libre == false){
					time_aux = part->time_creacion;
					indice_victima = i;
				}
			}
		}
		break;
		case LRU:{
			log_info(logBrokerInterno,"Reemplazo con LRU");
			algoritmoLRU();
		}
		break;
		default:
			log_info(logBrokerInterno,"No matcheo reemplazo");
		break;
	}
	//Liberar la partición víctima
	log_info(logBrokerInterno,"Se elimina la particion con indice %d",indice_victima);
	t_particion* part_liberar = list_get(particiones,indice_victima);
	part_liberar->libre = true;
	list_add_in_index(particiones,indice_victima,part_liberar);
	list_remove(particiones,indice_victima+1);

}

void algoritmoFIFO(){
	log_info(logBrokerInterno, "Algoritmo FIFO.");
}

// Hay que pasarle las ocupadas - las disponibles
void algoritmoLRU(int particiones_a_librerar){
	log_info(logBrokerInterno, "Algoritmo LRU.");

	int i,j;
	for (i = 0; i < particiones_a_librerar; i++) {
		int cantidad_particiones = list_size(particiones);
		for (j = 0; j < cantidad_particiones; j++) {
			// Lo va a seguir la Rocío del futuro cercano
		}
	}
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

	memcpy(stream, &largo_stream, sizeof(uint32_t));

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

t_new_pokemon* descachearNewPokemon(void* stream, uint32_t id){
	t_new_pokemon* mensaje_a_enviar = malloc(sizeof(t_new_pokemon*));
	uint32_t largo_nombre = strlen(mensaje_a_enviar->nombre_pokemon) + 1;

	memcpy(&largo_nombre, stream, sizeof(largo_nombre));
	stream += sizeof(largo_nombre);
	memcpy(&(mensaje_a_enviar->nombre_pokemon), stream, sizeof(uint32_t));
	stream += sizeof(uint32_t);
	memcpy(&(mensaje_a_enviar->pos_x), stream, sizeof(uint32_t));
	stream += sizeof(uint32_t);
	memcpy(&(mensaje_a_enviar->pos_y), stream, sizeof(uint32_t));
	stream += sizeof(uint32_t);
	memcpy(&(mensaje_a_enviar->cantidad), stream, sizeof(uint32_t));
	stream += sizeof(uint32_t);

	mensaje_a_enviar->id_mensaje = id;

	memcpy(&(mensaje_a_enviar->id_mensaje), stream, sizeof(uint32_t));
	stream += sizeof(uint32_t);

	return mensaje_a_enviar;
}

t_appeared_pokemon* descachearAppearedPokemon(void* stream, uint32_t id){
	t_appeared_pokemon* mensaje_a_enviar = malloc(sizeof(t_appeared_pokemon*));
	uint32_t largo_nombre = strlen(mensaje_a_enviar->nombre_pokemon);

	memcpy(&largo_nombre, stream, sizeof(largo_nombre));
	stream += sizeof(largo_nombre);
	memcpy(&(mensaje_a_enviar->nombre_pokemon), stream, sizeof(uint32_t));
	stream += sizeof(uint32_t);
	memcpy(&(mensaje_a_enviar->pos_x), stream, sizeof(uint32_t));
	stream += sizeof(uint32_t);
	memcpy(&(mensaje_a_enviar->pos_y), stream, sizeof(uint32_t));
	stream += sizeof(uint32_t);

	mensaje_a_enviar->id_mensaje_correlativo = id;

	memcpy(&(mensaje_a_enviar->id_mensaje_correlativo), stream, sizeof(uint32_t));
	stream += sizeof(uint32_t);

	return mensaje_a_enviar;
}

t_catch_pokemon* descachearCatchPokemon(void* stream, uint32_t id){
	t_catch_pokemon* mensaje_a_enviar = malloc(sizeof(t_catch_pokemon*));
	uint32_t largo_nombre = strlen(mensaje_a_enviar->nombre_pokemon);

	memcpy(&largo_nombre, stream, sizeof(largo_nombre));
	stream += sizeof(largo_nombre);
	memcpy(&(mensaje_a_enviar->nombre_pokemon), stream, sizeof(uint32_t));
	stream += sizeof(uint32_t);
	memcpy(&(mensaje_a_enviar->pos_x), stream, sizeof(uint32_t));
	stream += sizeof(uint32_t);
	memcpy(&(mensaje_a_enviar->pos_y), stream, sizeof(uint32_t));
	stream += sizeof(uint32_t);

	mensaje_a_enviar->id_mensaje = id;

	memcpy(&(mensaje_a_enviar->id_mensaje), stream, sizeof(uint32_t));
	stream += sizeof(uint32_t);

	return mensaje_a_enviar;
}

t_caught_pokemon* descachearCaughtPokemon(void* stream, uint32_t id){
	t_caught_pokemon* mensaje_a_enviar = malloc(sizeof(t_caught_pokemon*));

	memcpy(&(mensaje_a_enviar->atrapo_pokemon), stream, sizeof(uint32_t));
	stream += sizeof(uint32_t);

	mensaje_a_enviar->id_mensaje_correlativo = id;

	memcpy(&(mensaje_a_enviar->id_mensaje_correlativo), stream, sizeof(uint32_t));
	stream += sizeof(uint32_t);

	return mensaje_a_enviar;
}

t_get_pokemon* descachearGetPokemon(void* stream, uint32_t id){
	t_get_pokemon* mensaje_a_enviar = malloc(sizeof(t_catch_pokemon*));
	uint32_t largo_nombre = strlen(mensaje_a_enviar->nombre_pokemon);

	memcpy(&largo_nombre, stream, sizeof(largo_nombre));
	stream += sizeof(largo_nombre);
	memcpy(&(mensaje_a_enviar->nombre_pokemon), stream, sizeof(uint32_t));
	stream += sizeof(uint32_t);

	mensaje_a_enviar->id_mensaje = id;

	memcpy(&(mensaje_a_enviar->id_mensaje), stream, sizeof(uint32_t));
	stream += sizeof(uint32_t);

	return mensaje_a_enviar;
}

t_localized_pokemon* descachearLocalizedPokemon(void* stream, uint32_t id){
	t_localized_pokemon* mensaje_a_enviar = malloc(sizeof(t_localized_pokemon*));
	uint32_t largo_nombre = strlen(mensaje_a_enviar->nombre_pokemon);

	memcpy(&largo_nombre, stream, sizeof(largo_nombre));
	stream += sizeof(largo_nombre);
	memcpy(&(mensaje_a_enviar->nombre_pokemon), stream, sizeof(uint32_t));
	stream += sizeof(uint32_t);
	memcpy(&(mensaje_a_enviar->cant_pos), stream, sizeof(uint32_t));
	stream += sizeof(uint32_t);

	uint32_t pos_x, pos_y;
	char** pos_list = string_get_string_as_array(mensaje_a_enviar->posiciones);

	for(int i = 1; i <= mensaje_a_enviar->cant_pos; i++){
		char** pos_pair = string_split(pos_list[i],"|");
		pos_x = atoi(pos_pair[0]);
		pos_y = atoi(pos_pair[1]);

		memcpy(&(pos_x), stream, sizeof(uint32_t));
		stream += sizeof(uint32_t);
		memcpy(&(pos_y), stream, sizeof(uint32_t));
		stream += sizeof(uint32_t);
	}

	mensaje_a_enviar->id_mensaje_correlativo = id;

	memcpy(&(mensaje_a_enviar->id_mensaje_correlativo), stream, sizeof(uint32_t));
	stream += sizeof(uint32_t);

	return mensaje_a_enviar;
}

/* FUNCIONES - CONEXIÓN */

void atenderCliente(int* socket){
	int socket_cliente = *socket;
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
			log_info(logBrokerInterno, "No se pudo conectar ningún proceso.");
			break;
		}
	}
}

void atenderMensajeNewPokemon(int socket_cliente){
	t_new_pokemon* new_pokemon = recibirNewPokemon(socket_cliente);

	// 3. Llegada de un nuevo mensaje a una cola de mensajes.
	log_info(logBroker, "Llegó un mensaje NEW_POKEMON %s %d %d %d.",new_pokemon->nombre_pokemon,new_pokemon->pos_x,new_pokemon->pos_y,new_pokemon->cantidad);

	uint32_t id_mensaje;

	encolarNewPokemon(new_pokemon);

	int enviado = devolverID(socket_cliente,&id_mensaje);
	new_pokemon->id_mensaje = id_mensaje;

	int cacheado = cachearNewPokemon(new_pokemon);
}

void atenderMensajeAppearedPokemon(int socket_cliente){
	t_appeared_pokemon* appeared_pokemon = recibirAppearedPokemon(socket_cliente);

	// 3. Llegada de un nuevo mensaje a una cola de mensajes.
	log_info(logBroker, "Llegó un mensaje APPEARED_POKEMON %s %d %d.",appeared_pokemon->nombre_pokemon,appeared_pokemon->pos_x,appeared_pokemon->pos_y);

	uint32_t id_mensaje;

	encolarAppearedPokemon(appeared_pokemon);

	int enviado = devolverID(socket_cliente,&id_mensaje);
//	appeared_pokemon->id_mensaje_correlativo = id_mensaje;

	int cacheado = cachearAppearedPokemon(appeared_pokemon);
}

void atenderMensajeCatchPokemon(int socket_cliente){
	t_catch_pokemon* catch_pokemon = recibirCatchPokemon(socket_cliente);

	// 3. Llegada de un nuevo mensaje a una cola de mensajes.
	log_info(logBroker, "Llegó un mensaje CATCH_POKEMON %s %d %d.",catch_pokemon->nombre_pokemon,catch_pokemon->pos_x,catch_pokemon->pos_y);

	uint32_t id_mensaje;

	encolarCatchPokemon(catch_pokemon);

	int enviado = devolverID(socket_cliente,&id_mensaje);
	catch_pokemon->id_mensaje = id_mensaje;

	int cacheado = cachearCatchPokemon(catch_pokemon);
}

void atenderMensajeCaughtPokemon(int socket_cliente){
	t_caught_pokemon* caught_pokemon = recibirCaughtPokemon(socket_cliente);

	// 3. Llegada de un nuevo mensaje a una cola de mensajes.
	log_info(logBroker, "Llegó un mensaje CAUGHT_POKEMON %d %d.",caught_pokemon->atrapo_pokemon,caught_pokemon->id_mensaje_correlativo);

	uint32_t id_mensaje;

	encolarCaughtPokemon(caught_pokemon);

	int enviado = devolverID(socket_cliente,&id_mensaje);
//	caught_pokemon->id_mensaje_correlativo = id_mensaje;

	int cacheado = cachearCaughtPokemon(caught_pokemon);
}

void atenderMensajeGetPokemon(int socket_cliente){
	t_get_pokemon* get_pokemon = recibirGetPokemon(socket_cliente);

	// 3. Llegada de un nuevo mensaje a una cola de mensajes.
	log_info(logBroker, "Llegó un mensaje GET_POKEMON %s.",get_pokemon->nombre_pokemon);

	uint32_t id_mensaje;

	encolarGetPokemon(get_pokemon);

	int enviado = devolverID(socket_cliente,&id_mensaje);
	get_pokemon->id_mensaje = id_mensaje;

	int cacheado = cachearGetPokemon(get_pokemon);
}

void atenderMensajeLocalizedPokemon(int socket_cliente){
	t_localized_pokemon* localized_pokemon = recibirLocalizedPokemon(socket_cliente);

	// 3. Llegada de un nuevo mensaje a una cola de mensajes.
	log_info(logBroker, "Llegó un mensaje LOCALIZED_POKEMON.");

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

	// 2. Suscripción de un proceso a una cola de mensajes.
	log_info(logBroker, "Se suscribe un Team a la Cola de Mensajes %d", suscribe_team->cola_suscribir);

	switch(suscribe_team->cola_suscribir){
		case APPEARED_POKEMON:{

			break;
		}
		case CAUGHT_POKEMON:{
			break;
		}
		case LOCALIZED_POKEMON:{
			break;
		}
		default:{
			log_info(logBrokerInterno, "No se pudo enviar mensajes cacheados.");
			break;
		}
	}
}

void atenderSuscripcionGameBoy(int socket_cliente){
	t_suscribe* suscribe_gameboy = recibirSuscripcion(SUSCRIBE_GAMEBOY,socket_cliente);

	int index = suscribir(socket_cliente,suscribe_gameboy->cola_suscribir);

	// 2. Suscripción de un proceso a una cola de mensajes.
	log_info(logBroker, "Se suscribe el Game Boy a la Cola de Mensajes %d", suscribe_gameboy->cola_suscribir);

	switch(suscribe_gameboy->cola_suscribir){
		case NEW_POKEMON:{
			enviarNewPokemonCacheados(socket_cliente, suscribe_gameboy->cola_suscribir);
			break;
		}
		default:{
			log_info(logBrokerInterno, "No se pudo enviar mensajes cacheados.");
			break;
		}
	}

	sleep(suscribe_gameboy->timeout);
	desuscribir(index,suscribe_gameboy->cola_suscribir);
}

void atenderSuscripcionGameCard(int socket_cliente){
	t_suscribe* suscribe_gamecard = recibirSuscripcion(SUSCRIBE_GAMECARD,socket_cliente);

	int index = suscribir(socket_cliente,suscribe_gamecard->cola_suscribir);

	// 2. Suscripción de un proceso a una cola de mensajes.
	log_info(logBroker, "Se suscribe un Game Card a la Cola de Mensajes %d", suscribe_gamecard->cola_suscribir);

	switch(suscribe_gamecard->cola_suscribir){
		case NEW_POKEMON:{
			enviarNewPokemonCacheados(socket_cliente, suscribe_gamecard->cola_suscribir);
			break;
		}
		case CATCH_POKEMON:{
			break;
		}
		case GET_POKEMON:{
			break;
		}
		default:{
			log_info(logBrokerInterno, "No se pudo enviar mensajes cacheados.");
			break;
		}
	}
}

/* FUNCIONES - PROCESAMIENTO */

int suscribir(int socket, op_code cola){
	int index = 0;
	switch(cola){
		case NEW_POKEMON:{
			pthread_mutex_lock(&sem_cola_new);
			index= list_add(cola_new->suscriptores,&socket);
			pthread_mutex_unlock(&sem_cola_new);
			sem_post(&mensajes_new);
			break;
		}
		case APPEARED_POKEMON:{
			pthread_mutex_lock(&sem_cola_appeared);
			index= list_add(cola_appeared->suscriptores,&socket);
			pthread_mutex_unlock(&sem_cola_appeared);
			sem_post(&mensajes_appeared);
			break;
		}
		case CATCH_POKEMON:{
			pthread_mutex_lock(&sem_cola_catch);
			index= list_add(cola_catch->suscriptores,&socket);
			pthread_mutex_unlock(&sem_cola_catch);
			sem_post(&mensajes_catch);
			break;
		}
		case CAUGHT_POKEMON:{
			pthread_mutex_lock(&sem_cola_caught);
			index= list_add(cola_caught->suscriptores,&socket);
			pthread_mutex_unlock(&sem_cola_caught);
			sem_post(&mensajes_caught);
			break;
		}
		case GET_POKEMON:{
			pthread_mutex_lock(&sem_cola_get);
			index= list_add(cola_get->suscriptores,&socket);
			pthread_mutex_unlock(&sem_cola_get);
			sem_post(&mensajes_get);
			break;
		}
		case LOCALIZED_POKEMON:{
			pthread_mutex_lock(&sem_cola_localized);
			index= list_add(cola_localized->suscriptores,&socket);
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
	uint32_t id = ID_MENSAJE ++;

	(*id_mensaje) = ID_MENSAJE;

	void*stream = malloc(sizeof(uint32_t));

	memcpy(stream, &(id), sizeof(uint32_t));

	int enviado = send(socket, stream, sizeof(uint32_t), 0);

	log_info(logBrokerInterno,"ID %d", id);
	return enviado;
}

void enviarNewPokemonCacheados(int socket, op_code tipo_mensaje){
	int tam_lista = list_size(particiones);
	t_particion* particion_buscada;

	for (int i = 0; i < tam_lista ; i++) {
		particion_buscada = list_get(particiones, i);

		if(particion_buscada->libre == 0 && particion_buscada->tipo_mensaje == tipo_mensaje){
			void* stream = malloc(particion_buscada->tamanio);

			t_new_pokemon* descacheado = descachearNewPokemon(stream, particion_buscada->id);

			enviarNewPokemon(socket, (*descacheado));
			log_info(logBrokerInterno, "Mensaje enviado %s", descacheado->nombre_pokemon);
		}
	}
}

void enviarAppearedPokemonCacheados(int socket, op_code tipo_mensaje){
	int tam_lista = list_size(particiones);
	t_particion* particion_buscada;

	for (int i = 0; i < tam_lista ; i++) {
		particion_buscada = list_get(particiones, i);

		if(particion_buscada->libre == 0 && particion_buscada->tipo_mensaje == tipo_mensaje){
			void* stream = malloc(particion_buscada->tamanio);

			t_appeared_pokemon* descacheado = descachearAppearedPokemon(stream);

			enviarAppearedPokemon(socket, (*descacheado));
			log_info(logBrokerInterno, "Mensaje enviado %s", descacheado->nombre_pokemon);
		}
	}
}

void enviarCatchPokemonCacheados(int socket, op_code tipo_mensaje){
	int tam_lista = list_size(particiones);
	t_particion* particion_buscada;

	for (int i = 0; i < tam_lista ; i++) {
		particion_buscada = list_get(particiones, i);

		if(particion_buscada->libre == 0 && particion_buscada->tipo_mensaje == tipo_mensaje){
			void* stream = malloc(particion_buscada->tamanio);

			t_catch_pokemon* descacheado = descachearCatchPokemon(stream);

			enviarCatchPokemon(socket, (*descacheado));
			log_info(logBrokerInterno, "Mensaje enviado %s", descacheado->nombre_pokemon);
		}
	}
}

void enviarCaughtPokemonCacheados(int socket, op_code tipo_mensaje){
	int tam_lista = list_size(particiones);
	t_particion* particion_buscada;

	for (int i = 0; i < tam_lista ; i++) {
		particion_buscada = list_get(particiones, i);

		if(particion_buscada->libre == 0 && particion_buscada->tipo_mensaje == tipo_mensaje){
			void* stream = malloc(particion_buscada->tamanio);

			t_caught_pokemon* descacheado = descachearCaughtPokemon(stream);

			enviarCaughtPokemon(socket, (*descacheado));
			log_info(logBrokerInterno, "Mensaje enviado");
		}
	}
}

void enviarGetPokemonCacheados(int socket, op_code tipo_mensaje){
	int tam_lista = list_size(particiones);
	t_particion* particion_buscada;

	for (int i = 0; i < tam_lista ; i++) {
		particion_buscada = list_get(particiones, i);

		if(particion_buscada->libre == 0 && particion_buscada->tipo_mensaje == tipo_mensaje){
			void* stream = malloc(particion_buscada->tamanio);

			t_get_pokemon* descacheado = descachearGetPokemon(stream);

			enviarGetPokemon(socket, (*descacheado));
			log_info(logBrokerInterno, "Mensaje enviado %s", descacheado->nombre_pokemon);
		}
	}
}

void enviarLocalizedPokemonCacheados(int socket, op_code tipo_mensaje){
	int tam_lista = list_size(particiones);
	t_particion* particion_buscada;

	for (int i = 0; i < tam_lista ; i++) {
		particion_buscada = list_get(particiones, i);

		if(particion_buscada->libre == 0 && particion_buscada->tipo_mensaje == tipo_mensaje){
			void* stream = malloc(particion_buscada->tamanio);

			t_localized_pokemon* descacheado = descachearLocalizedPokemon(stream);

			enviarLocalizedPokemon(socket, (*descacheado));
			log_info(logBrokerInterno, "Mensaje enviado %s", descacheado->nombre_pokemon);
		}
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

		while(1){
			log_info(logBrokerInterno,"While de aceptar cliente");
			cliente = aceptarCliente(socketServidorBroker);

			pthread_t hiloCliente;
			pthread_create(&hiloCliente, NULL, (void*)atenderCliente, &cliente);
			pthread_detach(hiloCliente);
		}

		close(socketServidorBroker);

		log_info(logBrokerInterno, "Se cerró el Socket Servidor %d.", socketServidorBroker);
	}else{
		log_info(logBrokerInterno,"No se pudo crear el Servidor Broker.");
	}

	sem_destroy(&mx_particiones);

	log_destroy(logBrokerInterno);
	log_destroy(logBroker);

	return 0;
}
