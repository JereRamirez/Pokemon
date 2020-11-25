/*
 * GameCard.h
 *
 *  Created on: 14 jun. 2020
 *      Author: utnso
 */

#ifndef GAMECARD_H_
#define GAMECARD_H_
#define TIEMPO_DE_REINTENTO_CONEXION "TIEMPO_DE_REINTENTO_CONEXION"
#define TIEMPO_DE_REINTENTO_OPERACION "TIEMPO_DE_REINTENTO_OPERACION"
#define TIEMPO_RETARDO_OPERACION "TIEMPO_RETARDO_OPERACION"
#define PUNTO_MONTAJE_TALLGRASS "PUNTO_MONTAJE_TALLGRASS"
#define IP_BROKER "IP_BROKER"
#define PUERTO_BROKER "PUERTO_BROKER"
#define BLOCK_SIZE "BLOCK_SIZE"
#define	BLOCKS "BLOCKS"
#define	MAGIC_NUMBER "MAGIC_NUMBER"
#define DIRECTORY "DIRECTORY"
#define	SIZE "SIZE"
#define	OPEN "OPEN"
#define IP_GAMECARD "IP_GAMECARD"
#define PUERTO_GAMECARD "PUERTO_GAMECARD"

#include<stdio.h>
#include<stdlib.h>
#include<stdint.h>
#include<unistd.h>
#include<string.h>
#include "utils.h"
#include<commons/log.h>
#include<commons/bitarray.h>
#include<commons/string.h>
#include<commons/collections/list.h>
#include<commons/collections/dictionary.h>
#include <semaphore.h>
#include <fcntl.h>
#include <sys/stat.h>
#include <sys/mman.h>
#include "delibird/comms/messages.h"
#include "delibird/comms/serialization.h"
#include "delibird/comms/pokeio.h"
#include <signal.h>
#include <errno.h>
#include <sys/types.h>
#include <sys/stat.h>

typedef struct {
	t_log* logger;
	char* ptoMnt;
	char* filePath;
	char* metadataPath;
	int blocks;
	int block_size;
	char* blocksPath;
	t_bitarray* bitArray;
	char* fileMapped;
	int retryOperation;
	int retryConnection;
	int delayTime;
	sem_t semMap;
} t_GameCard;

struct Broker
{
	char* ipKey;
	char* ip;
	char* portKey;
	char* port;
} broker;

typedef enum{
	METADATA,
	METADATA_DIRECTORY,
	BIN_FILE,
	BITMAP,
	POKE_METADATA
} fileType;

typedef struct{
	char directory;
	char* size;
	t_list* block;
	char open;

}t_file_metadata;

typedef struct {
	t_list* values;
}t_values;

typedef struct {
	int suscription;
	uint32_t queueType;
} t_args;

typedef struct {
	deli_message* message;
} t_args_process_message;


t_GameCard* GameCard_initialize(t_log*, char*,char*,char*, char*);
int GameCard_mountFS(t_config*);
void GameCard_Wait_For_Message(void*);
void GameCard_Process_Message(void*);
void* GameCard_Process_Message_New(deli_message*);
void* GameCard_Process_Message_Get(deli_message*);
void* GameCard_Process_Message_Catch(deli_message*);
void GameCard_Attend_Gameboy(int);
void GameCard_Process_Gameboy_Message(deli_message*);
void GameCard_Initialize_bitarray();
void GameCard_Destroy(t_GameCard*);

int create_directory(char*);
int create_file(fileType, t_values*);
int create_file_bitmap();
int create_file_bin(t_values*);
int create_file_metadata(t_values* );
int create_file_metadata_directory(t_values* );
int create_file_metadata_poke(t_values*);

void* create_poke_file(t_values*);
void* modify_poke_file(t_values*, char*);
sem_t* get_poke_semaphore(t_dictionary*, char*);
void create_poke_semaphore(char*);
void destroy_poke_semaphore(char*);
void destroy_poke_dictionary(t_dictionary*);
int catch_a_pokemon(char**,t_file_metadata*, char*, char*);
void create_localized_message(localized_pokemon*, char*,char*, t_file_metadata*);

int check_directory(char*);

void read_metadata_file(t_file_metadata*, char*, char*);
void Metadata_File_Open_Flag(t_file_metadata*,t_config*,char*);
void Metadata_File_Destroy(t_file_metadata*);
void Metadata_File_Initialize_Block(t_file_metadata*);
void Metadata_File_Add_Blocks(t_file_metadata*,int);
char* get_file_content(t_file_metadata*);
char* remove_line_from_file(char*,int, t_file_metadata*);

int get_amount_of_blocks(int, t_file_metadata*);
int get_message_size(new_pokemon* );
char* serialize_data(int, new_pokemon*);


void turn_bit_on(int);
void turn_bit_off(int);
int get_first_free_block();

int get_string_file_position(char*,char*);
int delete_file_line(char* , int , t_file_metadata* );
int increase_pokemon_amount(char** , int, int, t_file_metadata*);
int decrease_pokemon_amount(char** ,int, t_file_metadata*);
char* get_amount_of_pokemons(char*);
void delete_block_file(t_file_metadata*);

char* get_x_coordinate(char*);
char* get_y_coordinate(char*);

void write_blocks(t_file_metadata*,char*);
void rewrite_blocks(t_file_metadata* , char*);

void readConfigBrokerValues(t_config*,t_log* ,struct Broker*);
void subscribeToBroker(struct Broker broker,pthread_t* subs);
void* subscribeToBrokerNew();
void* subscribeToBrokerCatch();
void* subscribeToBrokerGet();

void initBroker(struct Broker *broker){
	broker->ipKey=IP_BROKER;
	broker->portKey=PUERTO_BROKER;

}

int connectBroker(char* , char* ,t_log* );

void signaltHandler(int);

#endif /* GAMECARD_H_ */
