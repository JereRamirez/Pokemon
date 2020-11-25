#ifndef DELIBIRD_MESSAGES_H_
#define DELIBIRD_MESSAGES_H_

#include <stdint.h>

///////// STRUCTS

//enum for type of message
typedef enum
{
	NEW_POKEMON = 1,
	LOCALIZED_POKEMON = 2,
	GET_POKEMON = 3,
	APPEARED_POKEMON = 4,
	CATCH_POKEMON = 5,
	CAUGHT_POKEMON = 6
} message_type;

typedef enum
{
	SUBSCRIPTION = 1,
	MESSAGE = 2,
	ACKNOWLEDGE = 3,
} op_code;

//a subsbscription will contain just a message_type
//an acknowledge will contain just the id of the message to acknowledge

//structs used only for communication, user should not have to touch this
typedef struct
{
	uint32_t bufferSize;
	void* stream;
} t_buffer;

typedef struct
{
	uint32_t operationCode;
	t_buffer* buffer;
} t_package;

typedef struct
{
	uint32_t id;
	uint32_t correlationId;
	uint32_t messageType;
	t_buffer* messageBuffer;
} t_message;


//structs used by the delibird protocol, used by the user to send and recieve messages

typedef struct
{
	uint32_t x;
	uint32_t y;
} Vector2;

typedef struct
{
	uint32_t id;
	uint32_t correlationId;
	uint32_t messageType;
	void* messageContent;
} deli_message;

// struct for new_pokemon
typedef struct
{
	char* pokemonName;
	uint32_t horizontalCoordinate;
	uint32_t verticalCoordinate;
	uint32_t ammount;
} new_pokemon;

// struct for localized_pokemon
typedef struct
{
	char* pokemonName;
	uint32_t ammount;
	Vector2* coordinates;
} localized_pokemon;

// struct for get_pokemon
typedef struct
{
	char* pokemonName;
} get_pokemon;

// struct for appeared_pokemon
typedef struct
{
	char* pokemonName;
	uint32_t horizontalCoordinate;
	uint32_t verticalCoordinate;
} appeared_pokemon;

// struct for catch_pokemon
typedef struct
{
	char* pokemonName;
	uint32_t horizontalCoordinate;
	uint32_t verticalCoordinate;
} catch_pokemon;

// struct for caught_pokemon
typedef struct
{
	uint32_t caught;
} caught_pokemon;

//////// METHODS

void Free_t_message(t_message* message);

void Free_t_package(t_package* package);

void Free_deli_message(deli_message* message);

void Free_new_pokemon(new_pokemon* new);

void Free_localized_pokemon(localized_pokemon* localized);

void Free_get_pokemon(get_pokemon* get);

void Free_appeared_pokemon(appeared_pokemon* appeared);

void Free_catch_pokemon(catch_pokemon* catch);

void Free_caught_pokemon(caught_pokemon* caught);

char* GetStringFromMessageType(message_type type);

void Free_deli_message_withContent(deli_message* message);


#endif
