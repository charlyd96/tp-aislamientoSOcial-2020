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
	log_info(logger, "Servidor levantado en el puerto %d", puerto);

	freeaddrinfo(servinfo);

	return socket_servidor;
}

int aceptarCliente(int socket_servidor){
	t_log* logger = log_create("conexion.log", "CONEXION", 0, LOG_LEVEL_INFO);

	struct sockaddr_in cliente;
	int size_cliente = sizeof(struct sockaddr_in);

	int socket_cliente = accept(socket_servidor, (void*) &cliente, &size_cliente);

	log_info(logger, "Se acepta la conexión de un cliente");
	log_destroy(logger);

	return socket_cliente;
}

void enviarNewPokemon(int socket_cliente, t_new_pokemon mensaje){
	t_buffer* buffer = malloc(sizeof(t_buffer));

	uint32_t largo_nombre  = strlen(mensaje.nombre_pokemon) + 1;
	printf("Largo del nombre %d\n", largo_nombre);

	buffer->size = sizeof(uint32_t) + largo_nombre + 3 * sizeof(uint32_t);
	void* stream = malloc(buffer->size);

	int offset = 0;

	memcpy(stream + offset, &(largo_nombre), sizeof(uint32_t));
	offset += sizeof(uint32_t);

	memcpy(stream + offset, mensaje.nombre_pokemon, largo_nombre);
	offset += largo_nombre;

	memcpy(stream + offset, &(mensaje.pos_x), sizeof(uint32_t));
	offset += sizeof(uint32_t);

	memcpy(stream + offset, &(mensaje.pos_y), sizeof(uint32_t));
	offset += sizeof(uint32_t);

	memcpy(stream + offset, &(mensaje.cantidad), sizeof(uint32_t));
	offset += sizeof(uint32_t);

	buffer->stream = stream;

	t_paquete* paquete = malloc(sizeof(t_paquete));

	paquete->codigo_operacion = NEW_POKEMON;
	paquete->buffer = buffer;

	int tamanio_a_enviar;

	void* mensajeSerializado = serializarPaquete(paquete, &tamanio_a_enviar);

	int enviado = send(socket_cliente, mensajeSerializado, tamanio_a_enviar, 0);

	if(enviado == -1){
		printf("No se envió el mensaje serializado");
	}else{
		printf("Lo que se envía es: %d\n", enviado);
	}

	free(mensajeSerializado);
	eliminarPaquete(paquete);
}

char* recibirNewPokemon(int socket_cliente){
	uint32_t size_buffer, largo_nombre, pos_x, pos_y, cantidad;

	op_code tipo_mensaje;

	recv(socket_cliente, &(tipo_mensaje), sizeof(uint32_t), MSG_WAITALL);
	recv(socket_cliente, &size_buffer, sizeof(uint32_t), MSG_WAITALL);
	recv(socket_cliente, &largo_nombre, sizeof(uint32_t), MSG_WAITALL);
	char* nombre_pokemon = malloc(largo_nombre);
	recv(socket_cliente, nombre_pokemon, largo_nombre, MSG_WAITALL);
	recv(socket_cliente, &pos_x, sizeof(uint32_t), MSG_WAITALL);
	recv(socket_cliente, &pos_y, sizeof(uint32_t), MSG_WAITALL);
	recv(socket_cliente, &cantidad, sizeof(uint32_t), MSG_WAITALL);

	printf("operacion %d\n", tipo_mensaje);
	printf("buffer size %d\n", size_buffer);
	printf("largo %d\n", largo_nombre);
	printf("nombre %s\n", nombre_pokemon);
	printf("pos x %d\n", pos_x);
	printf("pos y %d\n", pos_y);
	printf("cantidad %d\n", cantidad);

	t_new_pokemon new_pokemon;
	new_pokemon.nombre_pokemon = nombre_pokemon;
	new_pokemon.pos_x = pos_x;
	new_pokemon.pos_y = pos_y;
	new_pokemon.cantidad = cantidad;

	printf("El pokemon es %s\n", new_pokemon.nombre_pokemon);

	return nombre_pokemon;
}
