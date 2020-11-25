#include "serialization.h"
#include <stdio.h>
// ---- Serialization ----------------

void* SerializePackage(t_package* package)
{
	//Size of operation_code + stream_size + size of the stream
	void* serializedPackage = malloc(sizeof(uint32_t) + sizeof(uint32_t) + package->buffer->bufferSize);

	int offset = 0;
	memcpy(serializedPackage + offset, &(package->operationCode), sizeof(uint32_t));
	offset += sizeof(uint32_t);
	memcpy(serializedPackage + offset, &(package->buffer->bufferSize), sizeof(uint32_t));
	offset += sizeof(uint32_t);
	memcpy(serializedPackage + offset, package->buffer->stream, package->buffer->bufferSize);

	return serializedPackage;
}


t_buffer* SerializeMessage(t_message* message)
{
	//Size of id, corelation id, message type, message size, and size of message stream
	t_buffer* newBuffer = (t_buffer*)malloc(sizeof(t_buffer));
	newBuffer->bufferSize = sizeof(uint32_t) * 4 + message->messageBuffer->bufferSize;

	void* serializedMessage = malloc(newBuffer->bufferSize);

	int offset = 0;
	memcpy(serializedMessage + offset, &(message->messageType), sizeof(uint32_t));
	offset += sizeof(uint32_t);
	memcpy(serializedMessage + offset, &(message->correlationId), sizeof(uint32_t));
	offset += sizeof(uint32_t);
	memcpy(serializedMessage + offset, &(message->id), sizeof(uint32_t));
	offset += sizeof(uint32_t);
	memcpy(serializedMessage + offset, &(message->messageBuffer->bufferSize), sizeof(uint32_t));
	offset += sizeof(uint32_t);
	memcpy(serializedMessage + offset, message->messageBuffer->stream, message->messageBuffer->bufferSize);

	newBuffer->stream = serializedMessage;

	return newBuffer;
}


t_buffer* SerializeMessageContent(message_type type, void* content)
{
	t_buffer* serializedContent;
	switch(type)
	{
		case NEW_POKEMON:
			serializedContent = SerializeNewPokemon((new_pokemon*)content);
			break;
		case LOCALIZED_POKEMON:
			serializedContent = SerializeLocalizedPokemon((localized_pokemon*)content);
			break;
		case GET_POKEMON:
			serializedContent = SerializeGetPokemon((get_pokemon*)content);
			break;
		case CATCH_POKEMON:
			serializedContent = SerializeCatchPokemon((catch_pokemon*)content);
			break;
		case CAUGHT_POKEMON:
			serializedContent = SerializeCaughtPokemon((caught_pokemon*)content);
			break;
		case APPEARED_POKEMON:
			serializedContent = SerializeAppearedPokemon((appeared_pokemon*)content);
			break;
		default:
			return -1;
	}
	return serializedContent;
}


t_message* ConvertDeliMessageToMessage(deli_message* deliMessage)
{
	t_message* message = (t_message*)malloc(sizeof(t_message));
	message->id = deliMessage->id;
	message->correlationId = deliMessage->correlationId;
	message->messageType = deliMessage->messageType;
	message->messageBuffer = SerializeMessageContent(deliMessage->messageType, deliMessage->messageContent);
	return message;

}

t_buffer* SerializeNewPokemon(new_pokemon* newPokemon)
{
	//size of name, name, cordinate in x, cordinate in y, ammoun of pokemon
	uint32_t sizeOfPokemonName = strlen(newPokemon->pokemonName);

	t_buffer* newBuffer = (t_buffer*)malloc(sizeof(t_buffer));
	newBuffer->bufferSize = sizeof(uint32_t) * 4 + sizeOfPokemonName;

	void* newPokemonSerialized = malloc(newBuffer->bufferSize);

	int offset = 0;
	memcpy(newPokemonSerialized + offset, &(sizeOfPokemonName), sizeof(uint32_t));
	offset += sizeof(uint32_t);
	memcpy(newPokemonSerialized + offset, newPokemon->pokemonName, sizeOfPokemonName);
	offset += sizeOfPokemonName;
	memcpy(newPokemonSerialized + offset, &(newPokemon->horizontalCoordinate), sizeof(uint32_t));
	offset += sizeof(uint32_t);
	memcpy(newPokemonSerialized + offset, &(newPokemon->verticalCoordinate), sizeof(uint32_t));
	offset += sizeof(uint32_t);
	memcpy(newPokemonSerialized + offset, &(newPokemon->ammount),  sizeof(uint32_t));

	newBuffer->stream = newPokemonSerialized;

	return newBuffer;
}

t_buffer* SerializeLocalizedPokemon(localized_pokemon* localizedPokemon)
{
	//size of name, name, ammount in x, cordinates
	uint32_t sizeOfPokemonName = strlen(localizedPokemon->pokemonName);

	t_buffer* newBuffer = (t_buffer*)malloc(sizeof(t_buffer));
	newBuffer->bufferSize = sizeof(uint32_t) * 2 + sizeOfPokemonName + sizeof(uint32_t) * localizedPokemon->ammount * 2;

	void* localizedPokemonSerialized = malloc(newBuffer->bufferSize);

	uint32_t offset = 0;
	memcpy(localizedPokemonSerialized + offset, &(sizeOfPokemonName), sizeof(uint32_t));
	offset += sizeof(uint32_t);
	memcpy(localizedPokemonSerialized + offset, localizedPokemon->pokemonName, sizeOfPokemonName);
	offset += sizeOfPokemonName;
	memcpy(localizedPokemonSerialized + offset, &(localizedPokemon->ammount),  sizeof(uint32_t));
	offset += sizeof(uint32_t);

	for(int i = 0; i < (localizedPokemon->ammount); i++)
	{
		memcpy(localizedPokemonSerialized + offset, &(localizedPokemon->coordinates[i].x), sizeof(uint32_t));
		offset += sizeof(uint32_t);
		memcpy(localizedPokemonSerialized + offset, &(localizedPokemon->coordinates[i].y), sizeof(uint32_t));
		offset += sizeof(uint32_t);
	}

	newBuffer->stream = localizedPokemonSerialized;

	return newBuffer;
}

t_buffer* SerializeGetPokemon(get_pokemon* getPokemon)
{
	//size of name, name, ammount in x, cordinates
	uint32_t sizeOfPokemonName = strlen(getPokemon->pokemonName);

	t_buffer* newBuffer = (t_buffer*)malloc(sizeof(t_buffer));
	newBuffer->bufferSize = sizeof(uint32_t) + sizeOfPokemonName;

	void* getPokemonSerialized = malloc(newBuffer->bufferSize);

	uint32_t offset = 0;
	memcpy(getPokemonSerialized + offset, &(sizeOfPokemonName), sizeof(uint32_t));
	offset += sizeof(uint32_t);
	memcpy(getPokemonSerialized + offset, getPokemon->pokemonName, sizeOfPokemonName);

	newBuffer->stream = getPokemonSerialized;

	return newBuffer;
}

t_buffer* SerializeCatchPokemon(catch_pokemon* catchPokemon)
{
	//size of name, name, cordinate in x, cordinate in y
	uint32_t sizeOfPokemonName = strlen(catchPokemon->pokemonName);

	t_buffer* newBuffer = (t_buffer*)malloc(sizeof(t_buffer));
	newBuffer->bufferSize = sizeof(uint32_t) * 3 + sizeOfPokemonName;

	void* catchPokemonSerialized = malloc(newBuffer->bufferSize);

	int offset = 0;
	memcpy(catchPokemonSerialized + offset, &(sizeOfPokemonName), sizeof(uint32_t));
	offset += sizeof(uint32_t);
	memcpy(catchPokemonSerialized + offset, catchPokemon->pokemonName, sizeOfPokemonName);
	offset += sizeOfPokemonName;
	memcpy(catchPokemonSerialized + offset, &(catchPokemon->horizontalCoordinate), sizeof(uint32_t));
	offset += sizeof(uint32_t);
	memcpy(catchPokemonSerialized + offset, &(catchPokemon->verticalCoordinate), sizeof(uint32_t));

	newBuffer->stream = catchPokemonSerialized;

	return newBuffer;
}

t_buffer* SerializeCaughtPokemon(caught_pokemon* caughtPokemon)
{
	//int representing boolean awnser
	t_buffer* newBuffer = (t_buffer*)malloc(sizeof(t_buffer));
	newBuffer->bufferSize = sizeof(uint32_t);

	void* caughtPokemonSerialized = malloc(newBuffer->bufferSize);

	memcpy(caughtPokemonSerialized, &(caughtPokemon->caught), sizeof(uint32_t));

	newBuffer->stream = caughtPokemonSerialized;

	return newBuffer;
}

t_buffer* SerializeAppearedPokemon(appeared_pokemon* appearedPokemon)
{
	//size of name, name, cordinate in x, cordinate in y
	uint32_t sizeOfPokemonName = strlen(appearedPokemon->pokemonName);

	t_buffer* newBuffer = (t_buffer*)malloc(sizeof(t_buffer));
	newBuffer->bufferSize = sizeof(uint32_t) * 3 + sizeOfPokemonName;

	void* appearedPokemonSerialized = malloc(newBuffer->bufferSize);

	int offset = 0;
	memcpy(appearedPokemonSerialized + offset, &(sizeOfPokemonName), sizeof(uint32_t));
	offset += sizeof(uint32_t);
	memcpy(appearedPokemonSerialized + offset, appearedPokemon->pokemonName, sizeOfPokemonName);
	offset += sizeOfPokemonName;
	memcpy(appearedPokemonSerialized + offset, &(appearedPokemon->horizontalCoordinate), sizeof(uint32_t));
	offset += sizeof(uint32_t);
	memcpy(appearedPokemonSerialized + offset, &(appearedPokemon->verticalCoordinate), sizeof(uint32_t));

	newBuffer->stream = appearedPokemonSerialized;

	return newBuffer;
}



// ---- De-serialization ----------------

t_package* DeserializePackage(void* serializedPackage)
{
	t_package* package = (t_package*)malloc(sizeof(t_package));
	package->buffer = (t_buffer*)malloc(sizeof(t_buffer));

	memcpy(&(package->operationCode), serializedPackage, sizeof(uint32_t));
	serializedPackage += sizeof(uint32_t);
	memcpy(&(package->buffer->bufferSize), serializedPackage, sizeof(uint32_t));
	serializedPackage += sizeof(uint32_t);
	package->buffer->stream = malloc(package->buffer->bufferSize);
	memcpy(&(package->buffer->stream), serializedPackage, package->buffer->bufferSize);

	return package;
}

t_message* DeserializeMessage(void* serializedMessage)
{
	//Size of id, correlation id, message type, message size, and size of message stream
	t_message* message = (t_message*)malloc(sizeof(t_message));
	message->messageBuffer = (t_buffer*)malloc(sizeof(t_buffer));

	memcpy(&(message->messageType), serializedMessage, sizeof(uint32_t));
	serializedMessage += sizeof(uint32_t);
	memcpy(&(message->correlationId), serializedMessage, sizeof(uint32_t));
	serializedMessage += sizeof(uint32_t);
	memcpy(&(message->id), serializedMessage, sizeof(uint32_t));
	serializedMessage += sizeof(uint32_t);
	memcpy(&(message->messageBuffer->bufferSize), serializedMessage, sizeof(uint32_t));
	serializedMessage += sizeof(uint32_t);
	message->messageBuffer->stream = malloc(message->messageBuffer->bufferSize);
	memcpy(message->messageBuffer->stream, serializedMessage, message->messageBuffer->bufferSize);

	return message;
}


void* DeserializeMessageContent(message_type type, void* serializedContent)
{
	void* content;
	switch(type)
	{
		case NEW_POKEMON:
			content = (void*)DeserializeNewPokemon(serializedContent);
			break;
		case LOCALIZED_POKEMON:
			content = (void*)DeserializeLocalizedPokemon(serializedContent);
			break;
		case GET_POKEMON:
			content = (void*)DeserializeGetPokemon(serializedContent);
			break;
		case CATCH_POKEMON:
			content = (void*)DeserializeCatchPokemon(serializedContent);
			break;
		case CAUGHT_POKEMON:
			content = (void*)DeserializeCaughtPokemon(serializedContent);
			break;
		case APPEARED_POKEMON:
			content = (void*)DeserializeAppearedPokemon(serializedContent);
			break;
			//TODO Handle error
		default:
			return -1;
	}
	return content;
}


new_pokemon* DeserializeNewPokemon(void* serializedNewPokemon)
{
	//size of name, name, cordinate in x, cordinate in y, ammoun of pokemon
	//void* newPokemonSerialized = malloc(sizeof(uint32_t) * 4 + sizeOfPokemonName);
	new_pokemon* newPokemon = (new_pokemon*)malloc(sizeof(new_pokemon));

	uint32_t sizeOfPokemonName;
	memcpy(&(sizeOfPokemonName),serializedNewPokemon, sizeof(uint32_t));
	serializedNewPokemon += sizeof(uint32_t);
	newPokemon->pokemonName = (char*)malloc(sizeOfPokemonName + 1);
	memcpy(newPokemon->pokemonName, serializedNewPokemon, sizeOfPokemonName);
	newPokemon->pokemonName[sizeOfPokemonName] = '\0';
	serializedNewPokemon += sizeOfPokemonName;
	memcpy(&(newPokemon->horizontalCoordinate), serializedNewPokemon, sizeof(uint32_t));
	serializedNewPokemon += sizeof(uint32_t);
	memcpy(&(newPokemon->verticalCoordinate), serializedNewPokemon, sizeof(uint32_t));
	serializedNewPokemon += sizeof(uint32_t);
	memcpy(&(newPokemon->ammount), serializedNewPokemon,  sizeof(uint32_t));

	return newPokemon;
}

localized_pokemon* DeserializeLocalizedPokemon(void* serializedLocalizedPokemon)
{
	//size of name, name, cordinate in x, cordinate in y, ammoun of pokemon
	localized_pokemon* localizedPokemon = (localized_pokemon*)malloc(sizeof(localized_pokemon));

	uint32_t sizeOfPokemonName;
	memcpy(&(sizeOfPokemonName),serializedLocalizedPokemon, sizeof(uint32_t));
	serializedLocalizedPokemon += sizeof(uint32_t);
	localizedPokemon->pokemonName = (char*)malloc(sizeOfPokemonName + 1);
	memcpy(localizedPokemon->pokemonName, serializedLocalizedPokemon, sizeOfPokemonName);
	localizedPokemon->pokemonName[sizeOfPokemonName] = '\0';
	serializedLocalizedPokemon += sizeOfPokemonName;
	memcpy(&(localizedPokemon->ammount), serializedLocalizedPokemon,  sizeof(uint32_t));
	serializedLocalizedPokemon += sizeof(uint32_t);

	localizedPokemon->coordinates = (Vector2*)malloc(sizeof(Vector2)*localizedPokemon->ammount);

	for(int i = 0; i < (localizedPokemon->ammount); i++)
	{
		memcpy( &(localizedPokemon->coordinates[i].x), serializedLocalizedPokemon, sizeof(uint32_t));
		serializedLocalizedPokemon += sizeof(uint32_t);
		memcpy( &(localizedPokemon->coordinates[i].y), serializedLocalizedPokemon, sizeof(uint32_t));
		serializedLocalizedPokemon += sizeof(uint32_t);
	}
	
	return localizedPokemon;
}

get_pokemon* DeserializeGetPokemon(void* serializedGetPokemon)
{
	//size of name, name, cordinate in x, cordinate in y, ammoun of pokemon
	get_pokemon* getPokemon = (get_pokemon*)malloc(sizeof(get_pokemon));

	uint32_t sizeOfPokemonName;
	memcpy(&(sizeOfPokemonName),serializedGetPokemon, sizeof(uint32_t));
	serializedGetPokemon += sizeof(uint32_t);
	getPokemon->pokemonName = (char*)malloc(sizeOfPokemonName + 1);
	memcpy(getPokemon->pokemonName, serializedGetPokemon, sizeOfPokemonName);
	getPokemon->pokemonName[sizeOfPokemonName] = '\0';

	return getPokemon;
}

catch_pokemon* DeserializeCatchPokemon(void* serializedCatchPokemon)
{
	//size of name, name, cordinate in x, cordinate in y, ammoun of pokemon
	catch_pokemon* catchPokemon = (catch_pokemon*)malloc(sizeof(catch_pokemon));

	uint32_t sizeOfPokemonName;
	memcpy(&(sizeOfPokemonName),serializedCatchPokemon, sizeof(uint32_t));
	//catchPokemon->pokemonName = malloc(sizeOfPokemonName);
	serializedCatchPokemon += sizeof(uint32_t);
	catchPokemon->pokemonName = (char*)malloc(sizeOfPokemonName + 1);
	memcpy(catchPokemon->pokemonName, serializedCatchPokemon, sizeOfPokemonName);
	catchPokemon->pokemonName[sizeOfPokemonName] = '\0';
	serializedCatchPokemon += sizeOfPokemonName;
	memcpy(&(catchPokemon->horizontalCoordinate), serializedCatchPokemon, sizeof(uint32_t));
	serializedCatchPokemon += sizeof(uint32_t);
	memcpy(&(catchPokemon->verticalCoordinate), serializedCatchPokemon, sizeof(uint32_t));

	return catchPokemon;
}

caught_pokemon* DeserializeCaughtPokemon(void* serializedCaughtPokemon)
{
	//int representing boolean awnser
	caught_pokemon* caughtPokemon = (caught_pokemon*)malloc(sizeof(caught_pokemon));

	memcpy(&(caughtPokemon->caught), serializedCaughtPokemon, sizeof(uint32_t));

	return caughtPokemon;
}

appeared_pokemon* DeserializeAppearedPokemon(void* serializedAppearedPokemon)
{
	//size of name, name, cordinate in x, cordinate in y, ammoun of pokemon
	appeared_pokemon* appearedPokemon = (appeared_pokemon*)malloc(sizeof(appeared_pokemon));

	uint32_t sizeOfPokemonName;
	memcpy(&(sizeOfPokemonName),serializedAppearedPokemon, sizeof(uint32_t));
	serializedAppearedPokemon += sizeof(uint32_t);
	appearedPokemon->pokemonName = (char*)malloc(sizeOfPokemonName+1);
	memcpy(appearedPokemon->pokemonName, serializedAppearedPokemon, sizeOfPokemonName);
	appearedPokemon->pokemonName[sizeOfPokemonName] = '\0';
	serializedAppearedPokemon += sizeOfPokemonName;
	memcpy(&(appearedPokemon->horizontalCoordinate), serializedAppearedPokemon, sizeof(uint32_t));
	serializedAppearedPokemon += sizeof(uint32_t);
	memcpy(&(appearedPokemon->verticalCoordinate), serializedAppearedPokemon, sizeof(uint32_t));

	return appearedPokemon;
}


