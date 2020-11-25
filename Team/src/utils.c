/*
 * utils.c
 *
 *  Created on: 19 abr. 2020
 *      Author: utnso
 */

#include"utils.h"
/*
int iniciar_cliente(char* ip, char* puerto,t_log* logger)
{
	int teamSocket;

    struct addrinfo hints, *servinfo, *p;

    memset(&hints, 0, sizeof(hints));
    hints.ai_family = AF_UNSPEC;
    hints.ai_socktype = SOCK_STREAM;
    //hints.ai_flags = AI_PASSIVE;
    hints.ai_flags = 0;
    hints.ai_protocol = 0;

    getaddrinfo(ip, puerto, &hints, &servinfo);
    log_debug(logger,"IP y puerto configurado");
    for (p = servinfo; p != NULL; p = p->ai_next) {
    	teamSocket = socket(p->ai_family, p->ai_socktype, p->ai_protocol);
		log_debug(logger,"Socket configurado");
		if (teamSocket == -1){
			log_debug(logger,"El socket se configuró incorrectamente");
			continue;
		}
		log_debug(logger,"Se intentará conectar con Broker");
		if (connect(teamSocket, p->ai_addr, p->ai_addrlen)==0) {
			log_debug(logger,"La conexión fue realizada");
			freeaddrinfo(servinfo);
			return teamSocket;
		}else{
			log_debug(logger,"La conexión falló");
			return -1;
		}
		close(teamSocket);
	}
    return -1;
}



 void* serializar_paquete(t_paquete* paquete, int bytes)
{
	void * magic = malloc(bytes);
	int desplazamiento = 0;

	memcpy(magic + desplazamiento, &(paquete->codigo_operacion), sizeof(int));
	desplazamiento+= sizeof(int);
	memcpy(magic + desplazamiento, &(paquete->buffer->size), sizeof(int));
	desplazamiento+= sizeof(int);
	memcpy(magic + desplazamiento, paquete->buffer->stream, paquete->buffer->size);
	desplazamiento+= paquete->buffer->size;

	return magic;
}


void enviar_mensaje(void* payload, int size, int broker)
{
	t_paquete* paquete = malloc(sizeof(t_paquete));

	paquete->codigo_operacion = MENSAJE;
	paquete->buffer = malloc(sizeof(t_buffer));
	paquete->buffer->size = size;
	paquete->buffer->stream = malloc(paquete->buffer->size);
	memcpy(paquete->buffer->stream, payload, paquete->buffer->size);

	int bytes = paquete->buffer->size + 2*sizeof(int);

	void* a_enviar = serializar_paquete(paquete, bytes);

//	printf("Tamaño del stream %d \n",paquete->buffer->size);
//	printf("Tamaño del mensaje a enviar %d \n", bytes);
	send(broker, a_enviar, bytes, 0);

	free(a_enviar);
	free(paquete->buffer->stream);
	free(paquete->buffer);
	free(paquete);
}

void* recibir_mensaje(int socket_cliente, int* size,t_log* logger)
{
	void * buffer;

	recv(socket_cliente, size, sizeof(int), MSG_WAITALL);
	buffer = malloc(*size);
	recv(socket_cliente, buffer, *size, MSG_WAITALL);

	return buffer;
}
void serve_client(int* socket, t_log* logger)
{
	int cod_op;
	if(recv(*socket, &cod_op, sizeof(int), MSG_WAITALL) == -1)
		cod_op = -1;
	log_debug(logger,"Se analizó el codigo de operación, se obtuvo el siguiente: %i",cod_op);
	process_request(cod_op, *socket,logger);
}

void process_request(int cod_op, int cliente_fd,t_log* logger) {
	int size;
	void* msg;
		switch (cod_op) {
		case MENSAJE:;
			msg = recibir_mensaje(cliente_fd, &size,logger);
			log_debug(logger,"Se recibió un mensaje");
			log_debug(logger,msg);
			free(msg);
			break;

		default:
			break;
		}
}


*/
