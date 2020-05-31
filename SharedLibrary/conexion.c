/*
 * conexion.c
 *
 *  Created on: 24 abr. 2020
 *      Author: aislamientoSOcial
 */

#include "conexion.h"

int crearSocketCliente(char* ip, char* puerto){
	t_log* logger = log_create("conexion.log", "CONEXION", 0, LOG_LEVEL_ERROR);

	int socket_cliente, intentar_conexion;
	struct addrinfo hints, *servinfo;

	memset(&hints, 0, sizeof(hints));
	hints.ai_family = AF_UNSPEC;
	hints.ai_socktype = SOCK_STREAM;
	hints.ai_flags = AI_PASSIVE;

	getaddrinfo(ip, puerto, &hints, &servinfo);

	socket_cliente = socket(servinfo->ai_family, servinfo->ai_socktype, servinfo->ai_protocol);

	intentar_conexion = connect(socket_cliente, servinfo->ai_addr, servinfo->ai_addrlen);

	if(socket_cliente == -1){
		log_error(logger, "No se pudo crear el Socket Cliente");
		log_destroy(logger);
	} else if(intentar_conexion == -1){
		log_error(logger, "No se pudo establecer conexión entre el socket y el servidor");
		log_destroy(logger);
	}

	freeaddrinfo(servinfo);
	return socket_cliente;
}

int crearSocketServidor(char* ip, char* puerto){
	t_log* logger = log_create("conexion.log", "CONEXION", 0, LOG_LEVEL_ERROR);

	int socket_servidor, intentar_bindeo;
	struct addrinfo hints, *servinfo, *p;

	memset(&hints, 0, sizeof(hints));
	hints.ai_family = AF_UNSPEC;
	hints.ai_socktype = SOCK_STREAM;
	hints.ai_flags = AI_PASSIVE;

	getaddrinfo(ip, puerto, &hints, &servinfo);

	for (p=servinfo; p != NULL; p = p->ai_next){
		if ((socket_servidor = socket(p->ai_family, p->ai_socktype, p->ai_protocol)) == -1)
			continue;

		if((intentar_bindeo = bind(socket_servidor, p->ai_addr, p->ai_addrlen)) == -1){
			close(socket_servidor);
	        continue;
	    }
	    break;
	}

	if(socket_servidor == -1){
		log_error(logger, "No se pudo crear el Socket Servidor");
		freeaddrinfo(servinfo);
		log_destroy(logger);
		return -1;
	} else if(intentar_bindeo == -1){
		log_error(logger, "No funcionó el bind, el Servidor no se pudo asociar al puerto");
		freeaddrinfo(servinfo);
		log_destroy(logger);
		return -1;
	}

	listen(socket_servidor, SOMAXCONN);

	freeaddrinfo(servinfo);

	return socket_servidor;
}

int aceptarCliente(int socket_servidor){
	struct sockaddr_in cliente;
	int size_cliente = sizeof(struct sockaddr_in);

	int socket_cliente = accept(socket_servidor, (void*) &cliente, &size_cliente);

	if(socket_cliente != -1){
		t_log* logger = log_create("conexion.log", "CONEXION", 0, LOG_LEVEL_INFO);
		log_info(logger, "Se acepta la conexión de un cliente");
		log_destroy(logger);
	}

	return socket_cliente;
}

int recibirOperacion(int socket_cliente){
	int cod_op;
	int recibido = recv(socket_cliente, &cod_op, sizeof(uint32_t), MSG_WAITALL);
	if(recibido == 0){
		return -1;
	}
	return cod_op;
}
/**
 * Envía un mensaje por el socket indicado
 *
 */
int enviarMensaje(int nroSocket,op_code operacion,t_buffer* buffer){
	t_paquete* paquete = malloc(sizeof(t_paquete));

	paquete->codigo_operacion = operacion;
	paquete->buffer = buffer;

	int tamanio_a_enviar;

	void* mensajeSerializado = serializarPaquete(paquete, &tamanio_a_enviar);
	int enviado = send(nroSocket, mensajeSerializado, tamanio_a_enviar, 0);

	if(enviado == -1){
		printf("No se envió el mensaje serializado\n");
	}else{
		printf("Se conectó y se enviaron %d bytes\n",enviado);
	}

	free(mensajeSerializado);
	eliminarPaquete(paquete);
	return enviado;
}

int enviarNewPokemon(int socket_cliente, t_new_pokemon mensaje){
	t_buffer* buffer = serializarNewPokemon(mensaje);

	return enviarMensaje(socket_cliente,NEW_POKEMON,buffer);
}
/*
 * el campo id_mensaje_correlativo debe iniciarse en 0 en caso de no querer serializarlo
 *
 */
int enviarAppearedPokemon(int socket_cliente, t_appeared_pokemon mensaje){
	t_buffer* buffer = serializarAppearedPokemon(mensaje);

	return enviarMensaje(socket_cliente,APPEARED_POKEMON,buffer);
}
int enviarGetPokemon(int socket_cliente, t_get_pokemon mensaje){
	t_buffer* buffer = serializarGetPokemon(mensaje);

	return enviarMensaje(socket_cliente,GET_POKEMON,buffer);
}

int enviarLocalizedPokemon(int socket_cliente, t_localized_pokemon mensaje){
	t_buffer* buffer = serializarLocalizedPokemon(mensaje);

	return enviarMensaje(socket_cliente,LOCALIZED_POKEMON,buffer);
}
int enviarCatchPokemon(int socket_cliente, t_catch_pokemon mensaje){
	t_buffer* buffer = serializarCatchPokemon(mensaje);

	return enviarMensaje(socket_cliente,CATCH_POKEMON,buffer);
}
int enviarCaughtPokemon(int socket_cliente, t_caught_pokemon mensaje){
	t_buffer* buffer = serializarCaughtPokemon(mensaje);

	return enviarMensaje(socket_cliente,CAUGHT_POKEMON,buffer);
}

t_new_pokemon* recibirNewPokemon(int socket_cliente){
	uint32_t size_buffer, largo_nombre, pos_x, pos_y, cantidad;
	uint32_t id_mensaje = 0;
	t_new_pokemon* new_pokemon;
	int bytes_recibidos = 0;

	recv(socket_cliente, &size_buffer, sizeof(uint32_t), MSG_WAITALL);
	bytes_recibidos += sizeof(uint32_t);
	recv(socket_cliente, &largo_nombre, sizeof(uint32_t), MSG_WAITALL);
	bytes_recibidos += sizeof(uint32_t);
	char* nombre_pokemon = malloc(largo_nombre);
	recv(socket_cliente, nombre_pokemon, largo_nombre, MSG_WAITALL);
	bytes_recibidos += largo_nombre;
	recv(socket_cliente, &pos_x, sizeof(uint32_t), MSG_WAITALL);
	bytes_recibidos += sizeof(uint32_t);
	recv(socket_cliente, &pos_y, sizeof(uint32_t), MSG_WAITALL);
	bytes_recibidos += sizeof(uint32_t);
	recv(socket_cliente, &cantidad, sizeof(uint32_t), MSG_WAITALL);
	bytes_recibidos += sizeof(uint32_t);

	//Si el buffer es más largo, me falta recibir el id_mensaje
	if(size_buffer > bytes_recibidos){
		recv(socket_cliente, &id_mensaje, sizeof(uint32_t), MSG_WAITALL);
		bytes_recibidos += sizeof(uint32_t);
	}
	printf("buffer size %d\n", size_buffer);
	printf("largo %d\n", largo_nombre);
	printf("nombre %s\n", nombre_pokemon);
	printf("pos x %d\n", pos_x);
	printf("pos y %d\n", pos_y);
	printf("cantidad %d\n", cantidad);
	printf("id msg %d\n", id_mensaje);

	new_pokemon->nombre_pokemon = nombre_pokemon;
	new_pokemon->pos_x = pos_x;
	new_pokemon->pos_y = pos_y;
	new_pokemon->cantidad = cantidad;
	new_pokemon->id_mensaje = id_mensaje;

	return new_pokemon;
}

t_appeared_pokemon* recibirAppearedPokemon(int socket_cliente){

	uint32_t size_buffer, largo_nombre, pos_x, pos_y,id_mensaje_correlativo;
	int bytes_recibidos = 0;

	recv(socket_cliente, &size_buffer, sizeof(uint32_t), MSG_WAITALL);
	bytes_recibidos += sizeof(uint32_t);
	recv(socket_cliente, &largo_nombre, sizeof(uint32_t), MSG_WAITALL);
	bytes_recibidos += sizeof(uint32_t);
	char* nombre_pokemon = malloc(largo_nombre);
	recv(socket_cliente, nombre_pokemon, largo_nombre, MSG_WAITALL);
	bytes_recibidos += largo_nombre;
	recv(socket_cliente, &pos_x, sizeof(uint32_t), MSG_WAITALL);
	bytes_recibidos += sizeof(uint32_t);
	recv(socket_cliente, &pos_y, sizeof(uint32_t), MSG_WAITALL);
	bytes_recibidos += sizeof(uint32_t);

	if(size_buffer > bytes_recibidos){
		recv(socket_cliente, &id_mensaje_correlativo, sizeof(uint32_t), MSG_WAITALL);
		bytes_recibidos += sizeof(uint32_t);
	}

	t_appeared_pokemon* appeared_pokemon;
	appeared_pokemon->nombre_pokemon = nombre_pokemon;
	appeared_pokemon->pos_x = pos_x;
	appeared_pokemon->pos_y = pos_y;
	appeared_pokemon->id_mensaje_correlativo = id_mensaje_correlativo;

	printf("El pokemon es %s\n", appeared_pokemon->nombre_pokemon);

	return appeared_pokemon;
}
t_get_pokemon* recibirGetPokemon(int socket_cliente){

	uint32_t size_buffer, largo_nombre;
	uint32_t id_mensaje = 0;
	int bytes_recibidos = 0;

	recv(socket_cliente, &size_buffer, sizeof(uint32_t), MSG_WAITALL);
	bytes_recibidos += sizeof(uint32_t);

	recv(socket_cliente, &largo_nombre, sizeof(uint32_t), MSG_WAITALL);
	bytes_recibidos += sizeof(uint32_t);

	char* nombre_pokemon = malloc(largo_nombre);
	recv(socket_cliente, nombre_pokemon, largo_nombre, MSG_WAITALL);
	bytes_recibidos += largo_nombre;

	//Si me queda buffer por recibir, es el id_mensaje
	if(size_buffer > bytes_recibidos){
		recv(socket_cliente, &id_mensaje, sizeof(uint32_t), MSG_WAITALL);
		bytes_recibidos += sizeof(uint32_t);
	}

	t_get_pokemon* get_pokemon;
	get_pokemon->nombre_pokemon = nombre_pokemon;
	get_pokemon->id_mensaje = id_mensaje;

	printf("El pokemon es %s\n", get_pokemon->nombre_pokemon);

	return get_pokemon;
}
t_localized_pokemon* recibirLocalizedPokemon(int socket_cliente){
	uint32_t size_buffer, largo_nombre,cant_pos;
	uint32_t id_mensaje_correlativo = 0;
	uint32_t pos_x = 0;
	uint32_t pos_y = 0;
	int bytes_recibidos = 0;
	char* posicionesString = "";

	recv(socket_cliente, &size_buffer, sizeof(uint32_t), MSG_WAITALL);
	bytes_recibidos += sizeof(uint32_t);

	recv(socket_cliente, &largo_nombre, sizeof(uint32_t), MSG_WAITALL);
	bytes_recibidos += sizeof(uint32_t);

	char* nombre_pokemon = malloc(largo_nombre);
	recv(socket_cliente, nombre_pokemon, largo_nombre, MSG_WAITALL);
	bytes_recibidos += largo_nombre;

	recv(socket_cliente, &cant_pos, sizeof(uint32_t), MSG_WAITALL);
	bytes_recibidos += sizeof(uint32_t);

	strcat(posicionesString,"[");
	//Por cada cant_pos, recibir un par de enteros (armo el formato "[1|2,2|2]")
	for(int i = 1; i<=cant_pos; i++){
		recv(socket_cliente, &pos_x, sizeof(uint32_t), MSG_WAITALL);
		bytes_recibidos += sizeof(uint32_t);

		strcat(posicionesString,string_itoa(pos_x));
		strcat(posicionesString,"|");

		recv(socket_cliente, &pos_y, sizeof(uint32_t), MSG_WAITALL);
		bytes_recibidos += sizeof(uint32_t);

		strcat(posicionesString,string_itoa(pos_y));

		if(i < cant_pos) strcat(posicionesString,",");
	}
	strcat(posicionesString,"]");

	//Si me queda buffer por recibir, es el id_mensaje_correlativo
	if(size_buffer > bytes_recibidos){
		recv(socket_cliente, &id_mensaje_correlativo, sizeof(uint32_t), MSG_WAITALL);
		bytes_recibidos += sizeof(uint32_t);
	}

	t_localized_pokemon* localized_pokemon;
	localized_pokemon->nombre_pokemon = nombre_pokemon;
	localized_pokemon->cant_pos = cant_pos;
	localized_pokemon->posiciones = posicionesString;
	localized_pokemon->id_mensaje_correlativo = id_mensaje_correlativo;

	printf("El pokemon es %s\n", localized_pokemon->nombre_pokemon);

	return localized_pokemon;
}
t_catch_pokemon* recibirCatchPokemon(int socket_cliente){

	uint32_t size_buffer, largo_nombre, pos_x, pos_y;
	uint32_t id_mensaje = 0;
	int bytes_recibidos = 0;

	recv(socket_cliente, &size_buffer, sizeof(uint32_t), MSG_WAITALL);
	bytes_recibidos += sizeof(uint32_t);

	recv(socket_cliente, &largo_nombre, sizeof(uint32_t), MSG_WAITALL);
	bytes_recibidos += sizeof(uint32_t);

	char* nombre_pokemon = malloc(largo_nombre);
	recv(socket_cliente, nombre_pokemon, largo_nombre, MSG_WAITALL);
	bytes_recibidos += largo_nombre;

	recv(socket_cliente, &pos_x, sizeof(uint32_t), MSG_WAITALL);
	bytes_recibidos += sizeof(uint32_t);

	recv(socket_cliente, &pos_y, sizeof(uint32_t), MSG_WAITALL);
	bytes_recibidos += sizeof(uint32_t);

	//Si me queda buffer por recibir, es el id_mensaje
	if(size_buffer > bytes_recibidos){
		recv(socket_cliente, &id_mensaje, sizeof(uint32_t), MSG_WAITALL);
		bytes_recibidos += sizeof(uint32_t);
	}

	t_catch_pokemon* catch_pokemon;
	catch_pokemon->nombre_pokemon = nombre_pokemon;
	catch_pokemon->pos_x = pos_x;
	catch_pokemon->pos_y = pos_y;
	catch_pokemon->id_mensaje = id_mensaje;

	printf("El pokemon es %s\n", catch_pokemon->nombre_pokemon);

	return catch_pokemon;
}

t_caught_pokemon* recibirCaughtPokemon(int socket_cliente){

	uint32_t size_buffer, atrapo_pokemon, id_mensaje_correlativo;

	recv(socket_cliente, &size_buffer, sizeof(uint32_t), MSG_WAITALL);

	recv(socket_cliente, &atrapo_pokemon, sizeof(uint32_t), MSG_WAITALL);

	recv(socket_cliente, &id_mensaje_correlativo, sizeof(uint32_t), MSG_WAITALL);

	t_caught_pokemon* caught_pokemon;
	caught_pokemon->atrapo_pokemon = atrapo_pokemon;
	caught_pokemon->id_mensaje_correlativo = id_mensaje_correlativo;

	printf("El resultado es %d id_correlativo %d \n", caught_pokemon->atrapo_pokemon, caught_pokemon->id_mensaje_correlativo);

	return caught_pokemon;
}
