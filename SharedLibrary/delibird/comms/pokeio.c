#include "pokeio.h"
#include <stdio.h>


int SendAll(int client_socket, char *stream, uint32_t *lenght)
{
    int totalBytesSent = 0;
    int bytesleft = *lenght;
    int sendResult;

    while(totalBytesSent < *lenght)
    {
    	sendResult = send(client_socket, stream + totalBytesSent, bytesleft, 0 | MSG_NOSIGNAL);
        if (sendResult == -1) { break; }
        totalBytesSent += sendResult;
        bytesleft -= sendResult;
    }

    *lenght = totalBytesSent; // return number actually sent here

    return sendResult ==-1?-1:0; // return -1 on failure, 0 on success
}


int SendPackage(op_code opCode, t_buffer* buffer, int client_socket)
{
	t_package* package = (t_package*)malloc(sizeof(t_package));
	package->operationCode = opCode;
	package->buffer = buffer;

	void* serializedPackage = SerializePackage(package);
	uint32_t packageSize = sizeof(uint32_t) + sizeof(uint32_t) + package->buffer->bufferSize;

	int result = SendAll(client_socket, serializedPackage, &packageSize);

	Free_t_package(package);
	free(serializedPackage);

	return result;
}

int SendMessageAcknowledge(int messageId, int client_socket)
{
	void* stream = malloc(sizeof(uint32_t));
	memcpy(stream, &(messageId), sizeof(uint32_t));

	t_package* package = (t_package*)malloc(sizeof(t_package));
	package->operationCode = ACKNOWLEDGE;
	package->buffer = (t_buffer*)malloc(sizeof(t_buffer));
	package->buffer->bufferSize = sizeof(uint32_t);
	package->buffer->stream = stream;

	void* serializedPackage = SerializePackage(package);
	uint32_t packageSize =  sizeof(uint32_t) + sizeof(uint32_t) + sizeof(uint32_t);

	int result = SendAll(client_socket, serializedPackage, &packageSize);

	Free_t_package(package);
	free(serializedPackage);

	return result;
}

int SendSubscriptionRequest(message_type queueType, int client_socket)
{
	void* stream = malloc(sizeof(uint32_t));
	memcpy(stream, &(queueType), sizeof(uint32_t));

	t_package* package = (t_package*)malloc(sizeof(t_package));
	package->operationCode = SUBSCRIPTION;
	package->buffer = (t_buffer*)malloc(sizeof(t_buffer));
	package->buffer->bufferSize = sizeof(uint32_t);
	package->buffer->stream = stream;

	void* serializedPackage = SerializePackage(package);
	uint32_t packageSize =  sizeof(uint32_t) + sizeof(uint32_t) + sizeof(uint32_t);

	int result = SendAll(client_socket, serializedPackage, &packageSize);

	Free_t_package(package);
	free(serializedPackage);

	return result;
}


int SendMessage(deli_message deliMessage, int client_socket)
{
	t_message* message = ConvertDeliMessageToMessage(&deliMessage);
	t_buffer* buffer = SerializeMessage(message);

	int result = SendPackage(MESSAGE, buffer, client_socket);

	Free_t_message(message);

	return result;
}

int _send_message(t_buffer* messageBuffer, message_type type, uint32_t corelationId, int client_socket)
{
	t_message* message = (t_message*)malloc(sizeof(t_message));
	message->id = 0;
	message->correlationId = corelationId;
	message->messageType = type;
	message->messageBuffer = messageBuffer;

	t_buffer* buffer = SerializeMessage(message);

	int result = SendPackage(MESSAGE, buffer, client_socket);

	Free_t_message(message);

	return result;
}


int Send_NEW(new_pokemon new, int client_socket)
{
	int result = _send_message(
		SerializeNewPokemon(&new),
		NEW_POKEMON,
		0,
		client_socket
	);
	return result;
}

int Send_LOCALIZED(localized_pokemon localized, uint32_t corelationId, int client_socket)
{
	int result = _send_message(
		SerializeLocalizedPokemon(&localized),
		LOCALIZED_POKEMON,
		corelationId,
		client_socket
	);
	return result;
}

int Send_GET(get_pokemon get, int client_socket)
{
	int result = _send_message(
		SerializeGetPokemon(&get),
		GET_POKEMON,
		0,
		client_socket
	);
	return result;
}

int Send_CATCH(catch_pokemon catch, int client_socket)
{
	int result = _send_message(
		SerializeCatchPokemon(&catch),
		CATCH_POKEMON,
		0,
		client_socket
	);
	return result;
}

int Send_CAUGHT(caught_pokemon caught, uint32_t corelationId, int client_socket)
{
	int result = _send_message(
		SerializeCaughtPokemon(&caught),
		CAUGHT_POKEMON,
		corelationId,
		client_socket
	);
	return result;
}

int Send_APPEARED(appeared_pokemon appeared, uint32_t corelationId, int client_socket)
{
	int result = _send_message(
		SerializeAppearedPokemon(&appeared),
		APPEARED_POKEMON,
		corelationId,
		client_socket
	);
	return result;
}


int GetPackage(int client_socket, t_package** recievedPackage)
{
	uint32_t op_code;
	if(recv(client_socket, &op_code, sizeof(uint32_t), MSG_WAITALL) == -1)
	{
		return -1;
	}
	else
	{
		uint32_t streamSize;
		recv(client_socket, &streamSize, sizeof(uint32_t), MSG_WAITALL);
		void* stream = malloc(streamSize);
		int result = recv(client_socket, stream, streamSize, MSG_WAITALL);

		if(result == -1) return -1;

		t_package* newPackage = (t_package*)malloc(sizeof(t_package));
		newPackage->operationCode = op_code;
		newPackage->buffer = (t_buffer*)malloc(sizeof(t_buffer));
		newPackage->buffer->bufferSize = streamSize;
		newPackage->buffer->stream = stream;
		*recievedPackage = newPackage;
		return 0;
	}
}
/*
int GetMessage(int client_socket, deli_message* recievedMessage)
{
	t_package* package = GetPackage(client_socket);
	deli_message* message;
	if(package->operationCode == MESSAGE)
	{
		free(recievedMessage);
		recievedMessage = DeserializeMessage(package->buffer->stream);
		message = (deli_message*)malloc(sizeof(recievedMessage));
		message->id = recievedMessage->id;
		message->correlationId = (uint32_t)malloc(sizeof(uint32_t));
		message->correlationId = recievedMessage->correlationId;
		message->messageType = (uint32_t)malloc(sizeof(uint32_t));
		message->messageType = recievedMessage->messageType;
		message->messageContent = malloc(sizeof(void*));
		message->messageContent = DeserializeMessageContent(recievedMessage->messageType, recievedMessage->messageBuffer->stream);
	}
	else
	{
		message = -1;
	}
	Free_t_package(package);
	return 0;
}


uint32_t GetAcknowledge(int client_socket)
{
	t_package* package = GetPackage(client_socket);
	uint32_t acknowledge;
	if(package->operationCode == ACKNOWLEDGE)
	{
		memcpy(&(acknowledge), package->buffer->stream, package->buffer->bufferSize);
	}
	else
	{
		acknowledge = 0;
	}

	Free_t_package(package);

	return acknowledge;
}

message_type GetSubscription(int client_socket)
{
	
	t_package* package = GetPackage(client_socket);
	message_type type;
	if(package->operationCode == ACKNOWLEDGE)
	{
		memcpy(&(type), package->buffer->stream, package->buffer->bufferSize);
	}
	else
	{
		type = 0;
	}

	Free_t_package(package);

	return type;
}
*/

int RecievePackage(int client_socket, op_code* operationCode, void** content)
{
	t_package* recievedPackage;
	if(GetPackage(client_socket, &recievedPackage) == 0)
	{
		//printf("%i",recievedPackage->operationCode);
		*operationCode = recievedPackage->operationCode;
		switch(recievedPackage->operationCode)
		{
			case SUBSCRIPTION:
				{
				message_type* type = (message_type*)malloc(sizeof(uint32_t));
				memcpy(type, recievedPackage->buffer->stream, recievedPackage->buffer->bufferSize);
				*content = (void*)type;
				break;
				}
			case MESSAGE:
				{
				t_message* recievedMessage = DeserializeMessage(recievedPackage->buffer->stream);
				deli_message* message = (deli_message*)malloc(sizeof(deli_message));
				message->id = recievedMessage->id;
				message->correlationId = recievedMessage->correlationId;
				message->messageType = recievedMessage->messageType;
				message->messageContent = DeserializeMessageContent(recievedMessage->messageType, recievedMessage->messageBuffer->stream);
				*content = (void*)message;
				break;
				}
			case ACKNOWLEDGE:
				{
				uint32_t* id = (void*)malloc(sizeof(uint32_t));
				memcpy(id, recievedPackage->buffer->stream, recievedPackage->buffer->bufferSize);
				*content = (void*)id;
				break;
				}
			default:
				return -1;
		}
		return 0;
	}
	return -1;
}


//TODO Implement this
/*
void Send_CATCH(catch_pokemon catch, int socket_cliente);
void Send_CAUGHT(caught_pokemon caught, int socket_cliente);
void Send_APPEARED(appeared_pokemon appeared, int socket_cliente);
void Send_LOCALIZED(localized_pokemon localized, int socket_cliente);
void Send_GET(get_pokemon get, int socket_cliente);
*/
