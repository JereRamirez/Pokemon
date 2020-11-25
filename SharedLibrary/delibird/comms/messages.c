#include "messages.h"

void Free_t_message(t_message* message)
{
	free(message->messageBuffer->stream);
	free(message->messageBuffer);
	free(message);
}

void Free_t_package(t_package* package)
{
	free(package->buffer->stream);
	free(package->buffer);
	free(package);
}

void Free_deli_message(deli_message* message)
{
	free(message);
}

void Free_deli_message_withContent(deli_message* message)
{
	switch (message->messageType) {

		case NEW_POKEMON:
			Free_new_pokemon((new_pokemon*) message->messageContent);
			break;

		case LOCALIZED_POKEMON:
			Free_localized_pokemon((localized_pokemon*) message->messageContent);
			break;

		case GET_POKEMON:
			Free_get_pokemon((get_pokemon*) message->messageContent);
			break;

		case APPEARED_POKEMON:
			Free_appeared_pokemon((appeared_pokemon*) message->messageContent);
			break;

		case CATCH_POKEMON:
			Free_catch_pokemon((catch_pokemon*) message->messageContent);
			break;

		case CAUGHT_POKEMON:
			Free_caught_pokemon((caught_pokemon*) message->messageContent);
			break;
	}
	free(message);
}

void Free_new_pokemon(new_pokemon* new)
{
	free(new->pokemonName);
	free(new);
}

void Free_localized_pokemon(localized_pokemon* localized)
{
	free(localized->coordinates);
	free(localized->pokemonName);
	free(localized);
}

void Free_get_pokemon(get_pokemon* get)
{
	free(get->pokemonName);
	free(get);
}

void Free_appeared_pokemon(appeared_pokemon* appeared)
{
	free(appeared->pokemonName);
	free(appeared);
}

void Free_catch_pokemon(catch_pokemon* catch)
{
	free(catch->pokemonName);
	free(catch);
}


void Free_caught_pokemon(caught_pokemon* caught)
{
	free(caught);
}

char* GetStringFromMessageType(message_type type)
{
	char* result;
	switch(type)
	{
		case NEW_POKEMON:
			result = "NEW_POKEMON";
			break;
		case LOCALIZED_POKEMON:
			result = "LOCALIZED_POKEMON";
			break;
		case GET_POKEMON:
			result = "GET_POKEMON";
			break;
		case APPEARED_POKEMON:
			result = "APPEARED_POKEMON";
			break;
		case CATCH_POKEMON:
			result = "CATCH_POKEMON";
			break;
		case CAUGHT_POKEMON:
			result = "CAUGHT_POKEMON";
			break;
		default:
			result = "UNKNOWN";
			break;
	}
	return result;
}






