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
		freeaddrinfo(servinfo);
		return intentar_conexion;
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
			
			/*Hackeada rara para que los puertos no se "rompan" al cerrar mal los sockets*/
		int activado=1;
		setsockopt (socket_servidor, SOL_SOCKET, SO_REUSEADDR, &activado, sizeof (activado));


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
	uint32_t size_cliente = sizeof(struct sockaddr_in);

	int socket_cliente = accept(socket_servidor, (void*) &cliente, &size_cliente);

	if(socket_cliente != -1){
		t_log* logger = log_create("conexion.log", "CONEXION", 0, LOG_LEVEL_INFO);
		log_info(logger, "Se acepta la conexión de un Cliente.");
		log_destroy(logger);
	}

	return socket_cliente;
}

op_code recibirOperacion(int socket_cliente){
	op_code cod_op;
	int recibido = recv(socket_cliente, &cod_op, sizeof(uint32_t), MSG_WAITALL);
	if(recibido == 0){
		return OP_UNKNOWN;
	}
	return cod_op;
}

int enviarACK(int socket_destino){
	t_log* logger = log_create("conexion.log", "CONEXION", 0, LOG_LEVEL_ERROR);

	uint32_t ack = 1;
	int ack_enviado = send(socket_destino, &ack, sizeof(uint32_t), 0);
	if(ack_enviado == -1){
		log_error(logger, "No se pudo enviar el ACK.");
	}
	return ack_enviado;
}

int recibirACK(int socket_origen){
	t_log* logger = log_create("conexion.log", "CONEXION", 0, LOG_LEVEL_ERROR);

	uint32_t ack_recibido;
	int ack = recv(socket_origen, &ack_recibido, sizeof(uint32_t), 0);
	if(ack == -1){
		log_error(logger, "No se pudo enviar el ACK.");
	}
	return ack_recibido;
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

	free(mensajeSerializado);
	eliminarPaquete(paquete);
	return enviado;
}

int enviarNewPokemon(int socket_cliente, t_new_pokemon mensaje){
	t_buffer* buffer = serializarNewPokemon(mensaje);

	return enviarMensaje(socket_cliente,NEW_POKEMON,buffer);
}
int enviarSuscripcion(int nro_socket,t_suscribe suscripcion){

	uint32_t bytes_enviar = sizeof(op_code) + sizeof(op_code) + sizeof(uint32_t);
	if(suscripcion.tipo_suscripcion == SUSCRIBE_GAMEBOY) bytes_enviar += sizeof(uint32_t);
	uint32_t offset = 0;
	void* stream = malloc(bytes_enviar);

	memcpy(stream + offset, &(suscripcion.tipo_suscripcion), sizeof(op_code));
	offset += sizeof(op_code);
	memcpy(stream + offset, &(suscripcion.cola_suscribir), sizeof(op_code));
	offset += sizeof(op_code);
	memcpy(stream + offset, &(suscripcion.id_proceso), sizeof(uint32_t));
	offset += sizeof(uint32_t);

	if(suscripcion.tipo_suscripcion == SUSCRIBE_GAMEBOY){
		memcpy(stream + offset, &(suscripcion.timeout), sizeof(uint32_t));
		offset += sizeof(uint32_t);
	}

	int enviado = send(nro_socket, stream, bytes_enviar, 0);

	free(stream);
	return enviado;
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
	uint32_t bytes_recibidos = 0;

	recv(socket_cliente, &size_buffer, sizeof(uint32_t), MSG_WAITALL);

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

	t_new_pokemon* new_pokemon = malloc(sizeof(t_new_pokemon));
	new_pokemon->nombre_pokemon = nombre_pokemon;
	new_pokemon->pos_x = pos_x;
	new_pokemon->pos_y = pos_y;
	new_pokemon->cantidad = cantidad;
	new_pokemon->id_mensaje = id_mensaje;

	return new_pokemon;
}

t_appeared_pokemon* recibirAppearedPokemon(int socket_cliente){

	uint32_t size_buffer, largo_nombre, pos_x, pos_y,id_mensaje_correlativo;
	uint32_t bytes_recibidos = 0;

	recv(socket_cliente, &size_buffer, sizeof(uint32_t), MSG_WAITALL);
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

	t_appeared_pokemon* appeared_pokemon = malloc(sizeof(t_appeared_pokemon));
	appeared_pokemon->nombre_pokemon = nombre_pokemon;
	appeared_pokemon->pos_x = pos_x;
	appeared_pokemon->pos_y = pos_y;
	appeared_pokemon->id_mensaje_correlativo = id_mensaje_correlativo;

	return appeared_pokemon;
}
t_get_pokemon* recibirGetPokemon(int socket_cliente){

	uint32_t size_buffer, largo_nombre;
	uint32_t id_mensaje = 0;
	uint32_t bytes_recibidos = 0;

	recv(socket_cliente, &size_buffer, sizeof(uint32_t), MSG_WAITALL);

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

	t_get_pokemon* get_pokemon = malloc(sizeof(t_get_pokemon));
	get_pokemon->nombre_pokemon = nombre_pokemon;
	get_pokemon->id_mensaje = id_mensaje;

	return get_pokemon;
}
t_localized_pokemon* recibirLocalizedPokemon(int socket_cliente){
	uint32_t size_buffer, largo_nombre,cant_pos;
	uint32_t id_mensaje_correlativo = 0;
	uint32_t pos_x = 0;
	uint32_t pos_y = 0;
	uint32_t bytes_recibidos = 0;
	char* posicionesString = "";

	recv(socket_cliente, &size_buffer, sizeof(uint32_t), MSG_WAITALL);

	recv(socket_cliente, &largo_nombre, sizeof(uint32_t), MSG_WAITALL);
	bytes_recibidos += sizeof(uint32_t);

	char* nombre_pokemon = malloc(largo_nombre);
	recv(socket_cliente, nombre_pokemon, largo_nombre, MSG_WAITALL);
	bytes_recibidos += largo_nombre;

	recv(socket_cliente, &cant_pos, sizeof(uint32_t), MSG_WAITALL);
	bytes_recibidos += sizeof(uint32_t);

	strcat(posicionesString,"[");
	//Por cada cant_pos, recibir un par de enteros (armo el formato "[1|2,2|2]")
	for(uint32_t i = 1; i<=cant_pos; i++){
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

	t_localized_pokemon* localized_pokemon = malloc(sizeof(t_localized_pokemon));
	localized_pokemon->nombre_pokemon = nombre_pokemon;
	localized_pokemon->cant_pos = cant_pos;
	localized_pokemon->posiciones = posicionesString;
	localized_pokemon->id_mensaje_correlativo = id_mensaje_correlativo;

	return localized_pokemon;
}
t_catch_pokemon* recibirCatchPokemon(int socket_cliente){

	uint32_t size_buffer, largo_nombre, pos_x, pos_y;
	uint32_t id_mensaje = 0;
	uint32_t bytes_recibidos = 0;

	recv(socket_cliente, &size_buffer, sizeof(uint32_t), MSG_WAITALL);

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

	t_catch_pokemon* catch_pokemon = malloc(sizeof(t_catch_pokemon));
	catch_pokemon->nombre_pokemon = nombre_pokemon;
	catch_pokemon->pos_x = pos_x;
	catch_pokemon->pos_y = pos_y;
	catch_pokemon->id_mensaje = id_mensaje;

	return catch_pokemon;
}

t_caught_pokemon* recibirCaughtPokemon(int socket_cliente){

	uint32_t size_buffer, atrapo_pokemon, id_mensaje_correlativo;

	recv(socket_cliente, &size_buffer, sizeof(uint32_t), MSG_WAITALL);

	recv(socket_cliente, &atrapo_pokemon, sizeof(uint32_t), MSG_WAITALL);

	recv(socket_cliente, &id_mensaje_correlativo, sizeof(uint32_t), MSG_WAITALL);

	t_caught_pokemon* caught_pokemon = malloc(sizeof(t_caught_pokemon));
	caught_pokemon->atrapo_pokemon = atrapo_pokemon;
	caught_pokemon->id_mensaje_correlativo = id_mensaje_correlativo;

	return caught_pokemon;
}
t_suscribe* recibirSuscripcion(op_code tipo_suscripcion,int socket_cliente){
	op_code cola_suscribir;
	uint32_t timeout = 0;
	uint32_t id_proceso = 0;

	recv(socket_cliente, &cola_suscribir, sizeof(op_code), MSG_WAITALL);
	if(tipo_suscripcion == SUSCRIBE_GAMEBOY){
		recv(socket_cliente, &timeout, sizeof(uint32_t), MSG_WAITALL);
	}
	recv(socket_cliente, &id_proceso, sizeof(uint32_t), MSG_WAITALL);
	
	t_suscribe* suscripcion = malloc(sizeof(t_suscribe));
	suscripcion->tipo_suscripcion = tipo_suscripcion;
	suscripcion->cola_suscribir = cola_suscribir;
	suscripcion->timeout = timeout;
	suscripcion->id_proceso = id_proceso;

	return suscripcion;
}

char* colaParaLogs(op_code cola){
	char* colaLog;

	switch(cola){
		case NEW_POKEMON:{
			colaLog = "NEW_POKEMON";
			break;
		}
		case APPEARED_POKEMON:{
			colaLog = "APPEARED_POKEMON";
			break;
		}
		case CATCH_POKEMON:{
			colaLog = "CATCH_POKEMON";
			break;
		}
		case CAUGHT_POKEMON:{
			colaLog = "CAUGHT_POKEMON";
			break;
		}
		case GET_POKEMON:{
			colaLog = "GET_POKEMON";
			break;
		}
		case LOCALIZED_POKEMON:{
			colaLog = "LOCALIZED_POKEMON";
			break;
		}
		default:{
			colaLog = "NO ASIGNADA";
			break;
		}
	}

	return colaLog;
}
