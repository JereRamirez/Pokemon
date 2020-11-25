/*
 ============================================================================
 Name        : GamerCard.c
 Author      : Mauro
 Version     :
 Copyright   : Your copyright notice
 Description : GameCard Process
 ============================================================================
 */

#include "GameCard.h"

t_GameCard* GameCard;
t_dictionary* pokeSemaphore;
pthread_t* thread;
sem_t mutexDictionaty;
sem_t mutexDirectory;
sem_t mutexBitArray;
sem_t mutexCliente;

int main(void) {

	t_log* logger;
	t_config* config;
	pokeSemaphore=dictionary_create();
	signal(SIGINT,signaltHandler);

	char* ptoMnt;
	char* retryOperation;
	char* delayTime;
	char* ip;
	char* puerto;
	int server;
	char* retryConnection;
	sem_init(&(mutexDictionaty),0,1);
	sem_init(&(mutexDirectory),0,1);
	sem_init(&(mutexBitArray),0,1);
	sem_init(&(mutexCliente),0,1);
	thread = (pthread_t*)malloc(sizeof(pthread_t));

	logger = iniciar_logger();
	log_info(logger,"PROCESO GAMECARD ONLINE");


	config = read_config();

	ptoMnt = get_config_value(config,PUNTO_MONTAJE_TALLGRASS);
    retryConnection = get_config_value(config,TIEMPO_DE_REINTENTO_CONEXION);
	retryOperation = get_config_value(config,TIEMPO_DE_REINTENTO_OPERACION);
	delayTime = get_config_value(config,TIEMPO_RETARDO_OPERACION);
	ip = get_config_value(config,IP_GAMECARD);
	puerto = get_config_value(config,PUERTO_GAMECARD);

	server = iniciar_servidor(ip, puerto);


	GameCard=GameCard_initialize(logger,ptoMnt,retryOperation, delayTime, retryConnection);
	GameCard_mountFS(config);

	initBroker(&broker);
	readConfigBrokerValues(config,logger,&broker);

	pthread_t* subs=(pthread_t*)malloc(sizeof(pthread_t)*3);

	subscribeToBroker(broker,subs);

	config_destroy(config);

	while(1){

		sem_wait(&mutexCliente);
		int cliente = esperar_cliente(server);
		sem_post(&mutexCliente);

		pthread_create(thread,NULL,(void*)GameCard_Attend_Gameboy,(void*)cliente);

		pthread_detach(*thread);

	}

	munmap(GameCard->fileMapped, (GameCard->blocks/8));

	destroy_poke_dictionary(pokeSemaphore);

	GameCard_Destroy(GameCard);

	return EXIT_SUCCESS;
}

t_GameCard* GameCard_initialize(t_log* logger, char* ptoMnt,char* retryOperation, char* delayTime, char* retryConnection){
	t_GameCard* aux = (t_GameCard*)malloc(sizeof(t_GameCard));

	aux->ptoMnt = (char*)malloc(strlen(ptoMnt)+1);
	strcpy(aux->ptoMnt,ptoMnt);
	aux->logger = logger;

	char* files = (char*)malloc(strlen(ptoMnt)+strlen("/files/") + 1);
	strcpy(files,aux->ptoMnt);
	strcat(files,"/Files/");
	aux->filePath= files;

	char* metadata = (char*)malloc(strlen(ptoMnt)+strlen("/metadata/") + 1);
	strcpy(metadata,aux->ptoMnt);
	strcat(metadata,"/Metadata/");
	aux->metadataPath = metadata;

	char* blocks = (char*)malloc(strlen(ptoMnt)+strlen("/blocks/") + 1);
	strcpy(blocks,aux->ptoMnt);
	strcat(blocks,"/Blocks/");
	aux->blocksPath = blocks;
	aux->retryOperation = atoi(retryOperation);
	aux->delayTime = atoi(delayTime);
	aux->retryConnection = atoi(retryConnection);

	sem_init(&(aux->semMap),0,1);

	return aux;
}
void GameCard_Destroy(t_GameCard* GameCard){
	free(GameCard->blocksPath);
	free(GameCard->filePath);
	free(GameCard->metadataPath);
	free(GameCard->ptoMnt);
	free(GameCard->bitArray->bitarray);
	bitarray_destroy(GameCard->bitArray);
	sem_destroy(&(GameCard->semMap));
	log_destroy(GameCard->logger);
}

int GameCard_mountFS(t_config* config){

	// CREATE FIRST DIRECTORY
	int result = create_directory(GameCard->ptoMnt);

	// CREATE METADA FOR FILES & FILES DIRECTORY
	result = create_directory(GameCard->filePath);

	t_values* values = (t_values*)malloc(sizeof(t_values));

	values->values= list_create();
	char* value1 = "Y";
	list_add(values->values,value1);

	result = create_file(METADATA_DIRECTORY,values);

	list_destroy(values->values);

	// CREATE BLOCKS DIRECTORY
	result = create_directory(GameCard->blocksPath);

	// CREATE METADATA FOR METADA DIRECTORY

	result = create_directory(GameCard->metadataPath);

	char* blocksize= get_config_value(config,BLOCK_SIZE);

	char* blocks=get_config_value(config,BLOCKS);

	char* magicNumber = get_config_value(config,MAGIC_NUMBER);

	values->values= list_create();

	list_add(values->values,blocksize);
	list_add(values->values,blocks);
	list_add(values->values,magicNumber);

	result = create_file(METADATA,values);

	GameCard->block_size = atoi(blocksize);
	GameCard->blocks = atoi(blocks);

	list_destroy(values->values);

	GameCard_Initialize_bitarray();

	result = create_file(BITMAP,values);

	free(values);

	return result;
}

void GameCard_Attend_Gameboy(int var){

	uint32_t type;
	void* content = malloc(sizeof(void*));

	int cliente = var;

	int result= RecievePackage(cliente,&type,&content);

	if (!result)
	{
		deli_message* message = (deli_message*)content;

		SendMessageAcknowledge(message->id, cliente);

		GameCard_Process_Gameboy_Message(message);
	}

	pthread_exit(NULL);
}

void GameCard_Process_Gameboy_Message(deli_message* message){
	deli_message* responseMessage;

	switch (message->messageType){
		case NEW_POKEMON: {
			responseMessage=GameCard_Process_Message_New(message);
			break;
		}

		case GET_POKEMON:{
			responseMessage= GameCard_Process_Message_Get(message);
			break;
		}

		case CATCH_POKEMON:{
			responseMessage=GameCard_Process_Message_Catch(message);
			break;
		}
	}
	free(responseMessage);
	free(message->messageContent);
	free(message);
}

void GameCard_Wait_For_Message(void* variables){

	int suscription = ((t_args*)variables)->suscription;
	uint32_t queueType = ((t_args*)variables)->queueType;

	uint32_t type;
	void* content = malloc(sizeof(void*));

	int resultado= RecievePackage(suscription,&type,&content);

	if (!resultado)
	{
		deli_message* message = (deli_message*)content;
		SendMessageAcknowledge(message->id, suscription);
		t_args_process_message* argsProcessMessage= (t_args_process_message*) malloc (sizeof (t_args_process_message));
		argsProcessMessage->message = message;

		pthread_create(thread,NULL,(void*)GameCard_Process_Message,argsProcessMessage);

		pthread_create(thread,NULL,(void*)GameCard_Wait_For_Message,variables);
		pthread_detach(*thread);

		pthread_exit(NULL);
	}else
	{
//		thread = (pthread_t*)malloc(sizeof(pthread_t));
		switch (queueType) {

			case NEW_POKEMON: {
				pthread_create(thread,NULL,(void*)subscribeToBrokerNew,NULL);
				pthread_detach(*thread);

				pthread_exit(NULL);
				break;
			}

			case GET_POKEMON:{
				pthread_create(thread,NULL,(void*)subscribeToBrokerGet,NULL);
				pthread_detach(*thread);

				pthread_exit(NULL);
				break;
			}

			case CATCH_POKEMON:{
				pthread_create(thread,NULL,(void*)subscribeToBrokerCatch,NULL);
				pthread_detach(*thread);

				pthread_exit(NULL);
				break;
			}

		}
	}

}

void GameCard_Process_Message(void* variables){

	t_args_process_message* args = (t_args_process_message*)variables;

	deli_message* message = args->message;

	void* responseMessage;


	switch (message->messageType){
		case NEW_POKEMON: {
			responseMessage=GameCard_Process_Message_New(message);
			appeared_pokemon* appearedPokemon = (appeared_pokemon*)responseMessage;

			int brokerSocket = connectBroker(broker.ip,broker.port,GameCard->logger);
			if (brokerSocket > 0){
				Send_APPEARED(*(appearedPokemon),message->id,brokerSocket);
			}else{
				log_info(GameCard->logger, "El Mensaje no pudo ser enviado al broker");
			}
			break;
		}

		case GET_POKEMON:{
			responseMessage= GameCard_Process_Message_Get(message);
			localized_pokemon* localizedPokemon = (localized_pokemon*)responseMessage;

			int brokerSocket = connectBroker(broker.ip,broker.port,GameCard->logger);
			if (brokerSocket > 0){
				Send_LOCALIZED(*(localizedPokemon),message->id,brokerSocket);
			}else{
				log_info(GameCard->logger, "El Mensaje no pudo ser enviado al broker");
			}

			break;
		}

		case CATCH_POKEMON:{
			responseMessage=GameCard_Process_Message_Catch(message);
			caught_pokemon* caughtPokemon = (caught_pokemon*) responseMessage;

			int brokerSocket = connectBroker(broker.ip,broker.port,GameCard->logger);
			if (brokerSocket > 0){
				Send_CAUGHT(*(caughtPokemon),message->id,brokerSocket);
			}else{
				log_info(GameCard->logger, "El Mensaje no pudo ser enviado al broker");
			}

			break;
		}
	}
	free(responseMessage);
	free(args->message);
}

void* GameCard_Process_Message_Catch(deli_message* message){
	int resulCatchPokemon;
	catch_pokemon* catchPokemon = (catch_pokemon*)message->messageContent;

	char * directory = (char*)malloc(strlen(GameCard->filePath) + strlen(catchPokemon->pokemonName) + 1 );
	strcpy(directory,GameCard->filePath);
	strcat(directory,catchPokemon->pokemonName);

	caught_pokemon* caughtPokemon = (caught_pokemon*)malloc(sizeof(caught_pokemon));

	int result = check_directory(directory);

	if (result !=2){
		caughtPokemon->caught = 0;
		resulCatchPokemon =1;
		log_error(GameCard->logger, "No hay ningún pokemon %s en el File System", catchPokemon->pokemonName);

	}
	else
	{
		char* file = (char*)malloc(strlen(directory) + strlen("/metadata.bin") + 1);

		strcpy(file,directory);
		strcat(file,"/metadata.bin");

		t_file_metadata* metadataFile = (t_file_metadata*)malloc(sizeof(t_file_metadata));

		read_metadata_file(metadataFile, file, catchPokemon->pokemonName);

		// Levantar el archivo leyendo todos los bloques
		char* fileContent = get_file_content(metadataFile);

		log_info(GameCard->logger,"Archivo de pokemon %s leido", catchPokemon->pokemonName);

		char* horCoordinate = string_itoa(catchPokemon->horizontalCoordinate);
		char* verCoordinate = string_itoa(catchPokemon->verticalCoordinate);

		char* coordinate=(char*)malloc(strlen(horCoordinate)
				+strlen(verCoordinate)+2);

		strcpy(coordinate,horCoordinate);
		strcat(coordinate,"-");
		strcat(coordinate,verCoordinate);

		resulCatchPokemon = catch_a_pokemon(&(fileContent),metadataFile,coordinate, catchPokemon->pokemonName);

		sleep(GameCard->delayTime);

		free(horCoordinate);
		free(verCoordinate);
		free(coordinate);
		free(fileContent);
		free(file);
		Metadata_File_Destroy(metadataFile);

	}

	if (resulCatchPokemon == 0)
		caughtPokemon->caught = 1;
	else
		caughtPokemon->caught = 0;



	free(directory);

	return (void*)caughtPokemon;

}

void* GameCard_Process_Message_Get(deli_message* message){
	get_pokemon* getPokemon = (get_pokemon*)message->messageContent;

	char * directory = (char*)malloc(strlen(GameCard->filePath) + strlen(getPokemon->pokemonName) + 1 );
	strcpy(directory,GameCard->filePath);
	strcat(directory,getPokemon->pokemonName);

	localized_pokemon* localizedPokemon = (localized_pokemon*)malloc(sizeof(localized_pokemon));
	localizedPokemon->pokemonName = (char*)malloc(strlen(getPokemon->pokemonName) + 1 );

	int result = check_directory(directory);

	if (result !=2)
	{
		strcpy(localizedPokemon->pokemonName,getPokemon->pokemonName);
		localizedPokemon->coordinates = (Vector2*)malloc(sizeof(Vector2));
		localizedPokemon->coordinates->x=0;
		localizedPokemon->coordinates->y=0;
		localizedPokemon->ammount = 0;
	}
	else
	{
		char* file = (char*)malloc(strlen(directory) + strlen("/metadata.bin") + 1);

		strcpy(file,directory);
		strcat(file,"/metadata.bin");

		t_file_metadata* metadataFile = (t_file_metadata*)malloc(sizeof(t_file_metadata));

		read_metadata_file(metadataFile, file, getPokemon->pokemonName);

		// Levantar el archivo leyendo todos los bloques
		char* fileContent = get_file_content(metadataFile);

		log_info(GameCard->logger,"Archivo de pokemon %s leido", getPokemon->pokemonName);

		create_localized_message(localizedPokemon, fileContent, getPokemon->pokemonName, metadataFile);

		sleep(GameCard->delayTime);

		sem_t* pokeSem = get_poke_semaphore(pokeSemaphore,getPokemon->pokemonName);
		sem_wait(pokeSem);
		t_config* metadataConfig= read_metadata(file);
		Metadata_File_Open_Flag(metadataFile,metadataConfig,"N");
		sem_post(pokeSem);

		free(fileContent);
		free(file);
		Metadata_File_Destroy(metadataFile);
	}


	free(directory);

	return (void*)localizedPokemon;
}

void* GameCard_Process_Message_New(deli_message* message){

	void* responseMessage;
	new_pokemon* newPokemon = (new_pokemon*)message->messageContent;


	char * directory = (char*)malloc(strlen(GameCard->filePath) + strlen(newPokemon->pokemonName) + 1 );
	strcpy(directory,GameCard->filePath);
	strcat(directory,newPokemon->pokemonName);

	sem_wait(&(mutexDirectory));
	int result = create_directory(directory);
	sem_post(&(mutexDirectory));
	t_values* values= (t_values*)malloc(sizeof(t_values));
	values->values= list_create();

	list_add(values->values,newPokemon);


	switch (result){
		case 2:
		{
			responseMessage= modify_poke_file(values, directory);
			break;
		}

		case 0:
		{
			responseMessage= create_poke_file(values);
			break;
		}

		default:
		{
			break;
		}
	}

	list_destroy(values->values);
	free(values);
	free(directory);

	return responseMessage;
}

void GameCard_Initialize_bitarray(){
	t_bitarray* aux;
	int size = (GameCard->blocks/8);

	char* bytes = (char*)malloc(sizeof(char)*size);
	aux = bitarray_create_with_mode(bytes, size, LSB_FIRST);

	GameCard->bitArray=aux;
}

int catch_a_pokemon(char** fileContent, t_file_metadata* metadataFile, char* coordinate, char* pokemonName){

	char * file = *(fileContent);
	if (string_contains(file,coordinate))
	{
		int pos= get_string_file_position(file,coordinate);
		int resultDecreaseAmount = decrease_pokemon_amount(fileContent,pos,metadataFile);
		log_info(GameCard->logger,"Pokemon %s ha sido capturado", pokemonName);
		if (resultDecreaseAmount == 0)
		{
			char * file = *(fileContent);
			rewrite_blocks(metadataFile, file);
		}
		else
		{
			char * file = *(fileContent);
			delete_block_file(metadataFile);

			Metadata_File_Initialize_Block(metadataFile);

			int auxSize = atoi(metadataFile->size);

			free(metadataFile->size);

			metadataFile->size =  string_itoa(auxSize - resultDecreaseAmount);

			log_info(GameCard->logger,"Nuevo tamaño de la metadata luego del catch %i" , atoi(metadataFile->size));
			if (atoi(metadataFile->size)!=0){

				int amountOfBlocks=get_amount_of_blocks(atoi(metadataFile->size), metadataFile);

				Metadata_File_Add_Blocks(metadataFile,amountOfBlocks);

				write_blocks(metadataFile, file);

				metadataFile->directory='N';
				metadataFile->open='S';

				t_values* values = (t_values*)malloc(sizeof(t_values));
				values->values = list_create();

				list_add(values->values,pokemonName);
				list_add(values->values,metadataFile);

				sem_t* pokeSem = get_poke_semaphore(pokeSemaphore,pokemonName);
				sem_wait(pokeSem);

				create_file(POKE_METADATA,values);

				sem_post(pokeSem);

				list_remove(values->values,1);
				list_remove(values->values,0);
				free(values);

				char* pokeMetadata = (char*)malloc(strlen(GameCard->filePath) + strlen(pokemonName) + strlen("/matadata.bin") + 1 );
				strcpy(pokeMetadata,GameCard->filePath);
				strcat(pokeMetadata,pokemonName);
				strcat(pokeMetadata,"/metadata.bin");


				sem_wait(pokeSem);
				t_config* metadataConfig = read_metadata(pokeMetadata);
				Metadata_File_Open_Flag(metadataFile,metadataConfig,"N");
				sem_post(pokeSem);

			}else{
				char* pokeMetadata = (char*)malloc(strlen(GameCard->filePath) + strlen(pokemonName) + strlen("/matadata.bin") + 1 );
				strcpy(pokeMetadata,GameCard->filePath);
				strcat(pokeMetadata,pokemonName);
				strcat(pokeMetadata,"/metadata.bin");

				log_info(GameCard->logger,"Elimina la metadata del pokemon %s" , pokeMetadata);

				remove(pokeMetadata);
				free(pokeMetadata);

				char* pokeDirectory = (char*)malloc(strlen(GameCard->filePath)+strlen(pokemonName)+1);

				strcpy(pokeDirectory,GameCard->filePath);
				strcat(pokeDirectory,pokemonName);
				log_info(GameCard->logger,"Elimina la carpeta del pokemon %s" , pokeDirectory);

				rmdir(pokeDirectory);
				free(pokeDirectory);

			}
		}
		return 0;
	}
	else
	{
		log_error(GameCard->logger, "No hay ningún pokemon en la posición %s ", coordinate);
		return 1;
	}
}

void create_localized_message(localized_pokemon* localizedPokemon, char* fileContent, char* pokemonName, t_file_metadata* metadataFile)
{
	strcpy(localizedPokemon->pokemonName,pokemonName);

	int times = 1;
	int cursor = 0;
	int tam = atoi(metadataFile->size);
	int vectorPosition=0;

	localizedPokemon->coordinates =(Vector2*)malloc(sizeof(Vector2));
	localizedPokemon->ammount = 0;


	while(cursor < tam){
		int start = cursor;

		while ((fileContent[cursor] != '\n') || (cursor > tam) ) cursor++;

		if (cursor <= tam){
			char* line = string_substring(fileContent,start,cursor+1);
			char* charAmount= get_amount_of_pokemons(line);

			localizedPokemon->ammount = localizedPokemon->ammount + atoi(charAmount);

			char* xCoor = get_x_coordinate(line);
			char* yCoor = get_y_coordinate(line);

			(localizedPokemon->coordinates[vectorPosition]).x = atoi(xCoor);
			(localizedPokemon->coordinates[vectorPosition]).y = atoi(yCoor);

			times++;
			vectorPosition++;
			localizedPokemon->coordinates = (Vector2*)realloc(localizedPokemon->coordinates, sizeof(Vector2) * times);

			free(line);
			free(charAmount);
			free(xCoor);
			free(yCoor);

		}

		cursor++;
	}

}


void* modify_poke_file(t_values* values, char* directory){;

	char* file = (char*)malloc(strlen(directory) + strlen("/metadata.bin") + 1);

	strcpy(file,directory);
	strcat(file,"/metadata.bin");

	t_file_metadata* metadataFile = (t_file_metadata*)malloc(sizeof(t_file_metadata));

	new_pokemon* newPokemon = (new_pokemon*)list_get(values->values,0);

	appeared_pokemon* appearedPokemon = (appeared_pokemon*)malloc(sizeof(appeared_pokemon));
	appearedPokemon->pokemonName = (char*)malloc(strlen(newPokemon->pokemonName) + 1);
	strcpy(appearedPokemon->pokemonName,newPokemon->pokemonName);
	appearedPokemon->horizontalCoordinate = newPokemon->horizontalCoordinate;
	appearedPokemon->verticalCoordinate = newPokemon->verticalCoordinate;

	read_metadata_file(metadataFile, file, newPokemon->pokemonName);

	char* fileContent = get_file_content(metadataFile);

	log_info(GameCard->logger,"Archivo de pokemon %s leido", newPokemon->pokemonName);

	char* horCoordinate = string_itoa(newPokemon->horizontalCoordinate);
	char* verCoordinate = string_itoa(newPokemon->verticalCoordinate);

	char* coordinate=(char*)malloc(strlen(horCoordinate)
			+strlen(verCoordinate)+2);

	strcpy(coordinate,horCoordinate);
	strcat(coordinate,"-");
	strcat(coordinate,verCoordinate);
	sem_t* pokeSem = get_poke_semaphore(pokeSemaphore,newPokemon->pokemonName);

	if (string_contains(fileContent,coordinate))
	{
		int pos= get_string_file_position(fileContent,coordinate);
		int newAmountExtraSize=increase_pokemon_amount(&(fileContent),pos,newPokemon->ammount, metadataFile);
		if (newAmountExtraSize==0){
			log_info(GameCard->logger, "Re escribe la metadata del pokemon %s", newPokemon->pokemonName);
			rewrite_blocks(metadataFile, fileContent);
		}else{
			int amountOfBlocksNeededForNeWAmount= get_amount_of_blocks(newAmountExtraSize, metadataFile);

			Metadata_File_Add_Blocks(metadataFile,amountOfBlocksNeededForNeWAmount);

			int auxSize = atoi(metadataFile->size);

			free(metadataFile->size);

			metadataFile->size =  string_itoa(auxSize + newAmountExtraSize);

			write_blocks(metadataFile, fileContent);

			metadataFile->directory='N';
			metadataFile->open='N';

			list_remove(values->values,0);

			list_add(values->values,newPokemon->pokemonName);
			list_add(values->values,metadataFile);

			sem_wait(pokeSem);

			create_file(POKE_METADATA,values);

			sem_post(pokeSem);
		}
	}
	else
	{
		int size = get_message_size(newPokemon);

		int amountOfBlocks=get_amount_of_blocks(size,metadataFile);

		int auxSize = atoi(metadataFile->size);

		free(metadataFile->size);

		metadataFile->size =  string_itoa(auxSize + size);

		Metadata_File_Add_Blocks(metadataFile,amountOfBlocks);

		char*buffer = serialize_data(size,newPokemon);

		string_append(&(fileContent),buffer);

		free(buffer);

		write_blocks(metadataFile, fileContent);

		metadataFile->directory='N';
		metadataFile->open='S';

		list_remove(values->values,0);

		list_add(values->values,newPokemon->pokemonName);
		list_add(values->values,metadataFile);

		sem_wait(pokeSem);

		create_file(POKE_METADATA,values);

		sem_post(pokeSem);
	}


	sleep(GameCard->delayTime);

	sem_wait(pokeSem);
	t_config* metadataConfig = read_metadata(file);
	Metadata_File_Open_Flag(metadataFile,metadataConfig,"N");
	sem_post(pokeSem);

	free(horCoordinate);
	free(verCoordinate);
	free(coordinate);

	free(fileContent);
	free(file);

	Metadata_File_Destroy(metadataFile);

	return (void*)appearedPokemon;
}

void* create_poke_file(t_values* values){

	new_pokemon* newPokemon = (new_pokemon*)list_get(values->values,0);

	appeared_pokemon* appearedPokemon = (appeared_pokemon*)malloc(sizeof(appeared_pokemon));
	appearedPokemon->pokemonName = (char*)malloc(strlen(newPokemon->pokemonName) + 1);
	strcpy(appearedPokemon->pokemonName,newPokemon->pokemonName);
	appearedPokemon->horizontalCoordinate = newPokemon->horizontalCoordinate;
	appearedPokemon->verticalCoordinate = newPokemon->verticalCoordinate;

	t_file_metadata* metadataFile = (t_file_metadata*)malloc(sizeof(t_file_metadata));

	metadataFile->block= list_create();

	sem_t* pokeSem = get_poke_semaphore(pokeSemaphore,newPokemon->pokemonName);

	int size = get_message_size(newPokemon);

	int amountOfBlocks=get_amount_of_blocks(size,metadataFile);

	metadataFile->size =  string_itoa(size);

	Metadata_File_Add_Blocks(metadataFile,amountOfBlocks);


	char* buffer=serialize_data(atoi(metadataFile->size),newPokemon);

	write_blocks(metadataFile, buffer);

	free(buffer);

	metadataFile->directory='N';
	metadataFile->open='N';

	list_remove(values->values,0);
	list_add(values->values, newPokemon->pokemonName);

	list_add(values->values,metadataFile);

	create_file(POKE_METADATA,values);

	list_remove(values->values,1);
	list_remove(values->values,0);

	sem_post(pokeSem);

	Metadata_File_Destroy(metadataFile);

	sleep(GameCard->delayTime);

	return (void*)appearedPokemon;
}

void destroy_poke_dictionary(t_dictionary* pokeSemaphore){

	dictionary_destroy_and_destroy_elements(pokeSemaphore, (void*)sem_destroy);

}

void create_poke_semaphore(char* pokemonName){
	sem_t* pokeSem = (sem_t*)malloc(sizeof(sem_t));

	char* file = (char*)malloc(strlen(GameCard->filePath) + strlen(pokemonName) +strlen("/metadata.bin") + 1);
	strcpy(file,GameCard->filePath);
	strcat(file,pokemonName);
	strcat(file, "/metadata.bin");
	if( access( file, F_OK ) != -1 ) {
		sem_init(pokeSem,0,1);
	}else{
		sem_init(pokeSem,0,0);
	}
	free(file);

	dictionary_put(pokeSemaphore, pokemonName,(void*)pokeSem);
}

void destroy_poke_semaphore(char* pokemonName){
	sem_t* pokeSem=(sem_t*)dictionary_remove(pokeSemaphore,pokemonName);
	sem_destroy(pokeSem);
	free(pokeSem);
}

int create_file_metadata_poke(t_values* values){

	char* pokemonName = (char*)list_get(values->values,0);
	char rc='\n';
	char sepRight = ']';
	char sepLeft = '[';
	char comma = ',';
	char equal ='=';
	char* filename = (char*)malloc(strlen(GameCard->filePath) + strlen(pokemonName) + strlen("/metadata.bin") + 1);
	strcpy(filename,GameCard->filePath);
	strcat(filename,pokemonName);
	strcat(filename,"/metadata.bin");

	FILE* f= fopen(filename,"wb");

	if (f==NULL){
		return -1;
	}
	t_file_metadata* metadataFile = (t_file_metadata*)list_get(values->values,1);

	char* line = (char*)malloc(strlen(DIRECTORY) + 1 + 2 + 1);
	strcpy(line,DIRECTORY);
	strcat(line,"=");
	fwrite(line, strlen(line),1,f);
	fwrite(&(metadataFile->directory),sizeof(char),1,f);
	fwrite(&rc,sizeof(char),1,f);

	free(line);

	line=(char*)malloc(strlen(SIZE) + strlen(metadataFile->size) + 2 + 1);
	strcpy(line,SIZE);
	strcat(line,"=");
	strcat(line,metadataFile->size);
	fwrite(line, strlen(line),1,f);
	fwrite(&rc,sizeof(char),1,f);

	fwrite(&BLOCKS,strlen(BLOCKS),1,f);
	fwrite(&equal,1,1,f);
	fwrite(&sepLeft,sizeof(char),1,f);

	int index = 0;
	while(list_get(metadataFile->block, index) != NULL){
		fwrite(list_get(metadataFile->block, index), strlen(list_get(metadataFile->block, index)),1,f);
		index++;
		if (list_get(metadataFile->block, index) != NULL)
			fwrite(&comma,sizeof(char),1,f);
	}
	fwrite(&sepRight,sizeof(char),1,f);
	fwrite(&rc,sizeof(char),1,f);

	free(line);

	line = (char*)malloc(strlen(OPEN) + 1 + 2 + 1);
	strcpy(line,OPEN);
	strcat(line,"=");
	fwrite(line, strlen(line),1,f);
	fwrite(&(metadataFile->open),sizeof(char),1,f);
	fwrite(&rc,sizeof(char),1,f);

	free(line);
	free(filename);
	fclose(f);
	return 0;
}

int create_file_bin(t_values* values){

	int bit = (int)list_get(values->values,0);

	char* blockChar =string_itoa(bit);

	char* filename = (char*)malloc(strlen(GameCard->blocksPath) + strlen(blockChar) + strlen(".bin") + 1 );
	strcpy(filename,GameCard->blocksPath);
	strcat(filename,blockChar);
	strcat(filename,".bin");

	FILE* f= fopen(filename,"wb");

	if (f==NULL){
		return -1;
	}


	int tam =(int)list_get(values->values,2);
	char* bytes = malloc(tam);
	memcpy(bytes,list_get(values->values,1),tam);
	fwrite(bytes,tam, 1, f);

	log_info(GameCard->logger, "Escribe el archivo de bloque: %s", filename);
	free(bytes);
	free(filename);
	free(blockChar);

	fclose(f);
	return 0;
}

int create_file_bitmap(){

	int exist;

	char* filename = (char*)malloc(strlen(GameCard->metadataPath) + strlen("bitmap.bin") + 1);
	strcpy(filename,GameCard->metadataPath);
	strcat(filename,"bitmap.bin");

	int f;

	if( access( filename, F_OK ) != -1 ) {
			f = open(filename, O_RDWR, (mode_t)0600);
			exist = 1;
	}
	else {
			f = open(filename, O_CREAT|O_RDWR, (mode_t)0600);
			exist = 0;
	}

	if (f==-1){
			return -1;
	}

	if (exist){
			read(f,(void*)GameCard->bitArray->bitarray,(GameCard->blocks/8));
	}
	else{
			int max = bitarray_get_max_bit(GameCard->bitArray);
			for (int i = 0; i< max; i++){
				bitarray_clean_bit(GameCard->bitArray,i);
			}
			write(f,(void*)GameCard->bitArray->bitarray,(GameCard->blocks/8));
	}

	if ((GameCard->fileMapped = mmap(0,(GameCard->blocks/8),PROT_READ|PROT_WRITE,MAP_SHARED|MAP_FILE,f,0)) ==  MAP_FAILED){
	}

	log_info(GameCard->logger, "Archivo Bit Map creado: %s", filename );

	free(filename);
	close(f);
	return 0;
}

int create_file_metadata(t_values* values){
	char salto = '\n';
	char* filename = (char*)malloc(strlen(GameCard->metadataPath) + strlen("metadata.bin") + 1 );
	strcpy(filename,GameCard->metadataPath);
	strcat(filename,"metadata.bin");

	FILE* f= fopen(filename,"wb");

	if (f==NULL){
		return -1;
	}

	char* line=(char*)malloc(strlen(BLOCK_SIZE) + strlen("=") + strlen((char*)list_get(values->values,0)) + 1 ) ;
	strcpy(line,BLOCK_SIZE);
	strcat(line, "=");
	strcat(line, (char*)list_get(values->values,0));
	fwrite(line, strlen(line), 1, f);
	fwrite(&salto, 1, 1, f);
	free(line);

	line = (char*)malloc(strlen(BLOCKS) + strlen("=") + strlen((char*)list_get(values->values,1))+1);
	strcpy(line,BLOCKS);
	strcat(line, "=");
	strcat(line, (char*)list_get(values->values,1));
	fwrite(line, strlen(line), 1, f);
	fwrite(&salto, 1, 1, f);
	free(line);

	line = (char*)malloc(strlen(MAGIC_NUMBER) + strlen("=") + strlen((char*)list_get(values->values,2))+1);
	strcpy(line,MAGIC_NUMBER);
	strcat(line, "=");
	strcat(line, (char*)list_get(values->values,2));
	fwrite(line, strlen(line), 1, f);
	fwrite(&salto, 1, 1, f);
	free(line);

	log_info(GameCard->logger, "Archivo metadata del File System: %s", filename);

	free(filename);

	fclose(f);
	return 0;

}


int create_file_metadata_directory(t_values* values){

	char salto = '\n';
	char* filename = (char*)malloc(strlen(GameCard->filePath) + strlen("metadata.bin") + 1 );
	strcpy(filename,GameCard->filePath);
	strcat(filename,"metadata.bin");

	FILE* f= fopen(filename,"wb");

	if (f==NULL){
		return -1;
	}

	char* line=(char*)malloc(strlen(DIRECTORY) + 4);
	strcpy(line,DIRECTORY);
	strcat(line, "=");
	strcat(line, (char*)list_get(values->values,0));
	fwrite(line, strlen(line), 1, f);
	fwrite(&salto,1,1,f);
	free(line);

	log_info(GameCard->logger, "Archivo de metadata de Pokemon creado: %s", filename);

	free(filename);
	fclose(f);
	return 0;
}

int create_file(fileType fileType, t_values* values){
	int result;

	switch(fileType){

		case METADATA:
		{
			result = create_file_metadata(values);
			break;
		}

		case BIN_FILE:
		{
			result = create_file_bin(values);
			break;
		}

		case BITMAP:
		{
			result = create_file_bitmap();
			break;
		}

		case METADATA_DIRECTORY:
		{
			result = create_file_metadata_directory(values);
			break;
		}

		case POKE_METADATA:
		{
			result= create_file_metadata_poke(values);
			break;
		}
	}

	return result;
}


//METADATA

void Metadata_File_Add_Blocks(t_file_metadata* metadataFile,int amountOfBlocks){
	for(int i = 0; i < amountOfBlocks; i++){
		sem_wait(&(mutexBitArray));
		int block = get_first_free_block();
		turn_bit_on(block);
		sem_post(&(mutexBitArray));
		char* charblock = string_itoa(block);
		list_add(metadataFile->block,charblock);
		log_info(GameCard->logger, "Agrega el bloque %i a la metadata del pokemon", block);
	}

}

void Metadata_File_Open_Flag(t_file_metadata* metadataFile,t_config* metadataConfig, char* value){
	config_set_value(metadataConfig, OPEN, value);
	config_save(metadataConfig);
	config_destroy(metadataConfig);

}

void Metadata_File_Destroy(t_file_metadata* metadataFile){
	free(metadataFile->size);
	list_destroy_and_destroy_elements(metadataFile->block,free);
	free(metadataFile);
}

void Metadata_File_Initialize_Block(t_file_metadata* metadataFile){
	list_destroy_and_destroy_elements(metadataFile->block, free);
	metadataFile->block = list_create();
}

void read_metadata_file(t_file_metadata* metadataFile, char* file, char* pokemonName){
	sem_t* pokeSem = get_poke_semaphore(pokeSemaphore,pokemonName);
	int fileAvailable=0;

	while(!fileAvailable){

		sem_wait(pokeSem);
		t_config* metadataConfig;
		metadataConfig = read_metadata(file);
		metadataFile->open = *(get_config_value(metadataConfig,OPEN));

		if(metadataFile->open =='N'){
			metadataFile->open = 'S';
			char* newValue ="S";
			config_set_value(metadataConfig, OPEN, newValue);
			config_save(metadataConfig);
			fileAvailable=1;
			config_destroy(metadataConfig);
			sem_post(pokeSem);
		}
		else
		{
			config_destroy(metadataConfig);
			sem_post(pokeSem);
			log_error(GameCard->logger, "Archivo abierto, re intenta en %d segundos", GameCard->retryOperation);
			sleep(GameCard->retryOperation);
		}

	}

	t_config* metadataConfig;
	metadataConfig = read_metadata(file);

	metadataFile->directory= *(get_config_value(metadataConfig,DIRECTORY));

	metadataFile->size= (char*)malloc(strlen(get_config_value(metadataConfig,SIZE))+1);
	strcpy(metadataFile->size,get_config_value(metadataConfig,SIZE));

	metadataFile->block = list_create();

	char** blocks = get_config_value_array(metadataConfig,BLOCKS);

	int i=0;
	while (blocks[i]!='\0'){
		char* aux = string_duplicate(blocks[i]);
		list_add(metadataFile->block,aux);
		i++;
	}

    string_iterate_lines(blocks, (void*) free);
    free(blocks);

	config_destroy(metadataConfig);

}

sem_t* get_poke_semaphore(t_dictionary* pokeSempahore, char* pokemonName){

	sem_t* aux;
	sem_wait(&(mutexDictionaty));
	if (dictionary_has_key(pokeSemaphore, pokemonName)){
		aux=dictionary_get(pokeSemaphore,pokemonName);
		sem_post(&(mutexDictionaty));
		return aux;
	}else
	{
		create_poke_semaphore(pokemonName);
		aux=dictionary_get(pokeSemaphore,pokemonName);
		sem_post(&(mutexDictionaty));
		return aux;
	}
}

char* get_file_content(t_file_metadata* metadataFile){

	char* aux = (char*)malloc(sizeof(char)*GameCard->block_size * list_size(metadataFile->block)  + 1 );

	int index = 0;

	int tam = atoi(metadataFile->size);

	while(list_get(metadataFile->block, index) != NULL){
		char* block =(char*)malloc(strlen(list_get(metadataFile->block, index))+1);
		strcpy(block,list_get(metadataFile->block, index));

		char* blockFile = (char*)malloc(strlen(GameCard->blocksPath) + strlen(block) + strlen(".bin") + 1 );
		strcpy(blockFile,GameCard->blocksPath);
		strcat(blockFile,block);
		strcat(blockFile,".bin");

		FILE* f= fopen(blockFile,"rb");

		if (f==NULL){
			return "1";
		}

		char* buffer=malloc(GameCard->block_size + 1);

		fread(buffer,GameCard->block_size,1,f);

		if (tam > GameCard->block_size){
			tam = tam - GameCard->block_size;
			buffer[GameCard->block_size] = '\0';
		}else{
			buffer[tam] = '\0';
		}

		if (index==0)
			strcpy(aux,(char*)buffer);
		else
			strcat(aux,(char*)buffer);

		free(buffer);
		fclose(f);
		free(block);
		free(blockFile);
		index++;

	}

	return aux;
}

char* remove_line_from_file(char* fileContent,int pos,t_file_metadata* metadataFile){

	int index1=0;

	while (fileContent[pos + index1] != '\n') index1++;

	char* line = string_substring(fileContent,pos,pos + index1+1);

	int bytes = strlen(line);
	free(line);

	int auxIntSize = atoi(metadataFile->size) - bytes;

	free(metadataFile->size);

	metadataFile->size = string_itoa(auxIntSize);

	char* aux=(char*)malloc(auxIntSize+2);

	if (pos != 0) {
		memcpy(aux,fileContent,pos-1);
		auxIntSize = auxIntSize-pos+1;
		if(auxIntSize > 0){
			memcpy(aux+pos-1,fileContent+pos+bytes-1,auxIntSize);
			aux[auxIntSize+pos-1]='\0';
		}
	}
	else
	{
		memcpy(aux,fileContent+bytes,auxIntSize);
		aux[auxIntSize]='\0';
	}

	return aux;
}

int increase_pokemon_amount(char** fileContent, int pos, int amount, t_file_metadata* metadataFile){

	int index1=0;
	int index2=0;

	char * file = *(fileContent);

	while (file[pos + index1] != '=') index1++;

	while (file[pos + index2] != '\n') index2++;

	char* line = string_substring(file,pos,pos + index2+1);

	char* stringAux= get_amount_of_pokemons(line);

	int chars = strlen(stringAux);

	int auxAmount = atoi(stringAux) + amount;

	char* newAmount = string_itoa(auxAmount);

	if (chars < strlen(newAmount)){
		//TODO:Ver 1262 si tengo que agregar un byte
		char* auxBuffer=(char*)malloc(atoi(metadataFile->size) + (strlen(newAmount) - chars));
		memcpy(auxBuffer,file,(pos+index1+1));
		memcpy(auxBuffer+(pos+index1)+1,newAmount,strlen(newAmount));
		memcpy(auxBuffer+(pos+index1)+strlen(newAmount)+1,file + pos +index1 + chars+1,atoi(metadataFile->size)-pos -index1);
		free(*(fileContent));
		*fileContent = auxBuffer;

		int auxNewAmount = strlen(newAmount);
		free(newAmount);
		free(line);
		free(stringAux);
		return (auxNewAmount - chars);
	}

	//TODO: Ver este warning, con un memcpy se arregla
	for(int i=0; i<strlen(newAmount); i++)
	{
		file[pos + index1 + 1 + i] = newAmount[i];
	}

	free(line);
	free(stringAux);
	free(newAmount);
	return 0;
}

int decrease_pokemon_amount(char** fileContent,int pos, t_file_metadata* metadataFile){

	int index1=0;
	int index2=0;

	char * file = *(fileContent);

	while (file[pos + index1 ] != '=') index1++;

	while (file[pos + index2] != '\n') index2++;

	// sumarle 1
	char* line = (char*)malloc(index2+1);
	memcpy(line,file+pos,index2+1);

	int bytes = index2+1;

	int auxIntSize = atoi(metadataFile->size) - bytes;

	char* stringAux= get_amount_of_pokemons(line);

	int auxAmount = atoi(stringAux) - 1;

	if (auxAmount == 0 ){
		char* auxBuffer=(char*)malloc(atoi(metadataFile->size) - index2+1 + 1);
		if (pos != 0) {
			memcpy(auxBuffer,file,pos);
			auxBuffer[pos]='\0';

			auxIntSize = auxIntSize-pos+1;
			if(auxIntSize > 0){
				//TODO; probar el +1
				memcpy(auxBuffer+pos-1,file+pos+bytes-1,auxIntSize);
				auxBuffer[pos+auxIntSize-1]='\0';
			}
		}
		else
		{
			memcpy(auxBuffer,file+bytes,auxIntSize);
			auxBuffer[auxIntSize]='\0';

		}

		free(*(fileContent));
		*fileContent = auxBuffer;

		free(line);
		free(stringAux);
		return bytes;
	}
	else
	{
		//TODO: ver que hacer si los dígitos del valor final son menos que el anterior
		char* newAmount = string_itoa(auxAmount);

		if (strlen(newAmount) < strlen(stringAux)){
			log_debug(GameCard->logger, "Sobre un caracter luego de restar la cantidad de pokemones");
		}

		for(int i=0; i<strlen(newAmount); i++)
		{
			file[pos + index1 + 1 + i] = newAmount[i];
		}

		free(newAmount);
	}

	free(line);
	free(stringAux);

	return 0;
}

void delete_block_file(t_file_metadata* metadataFile){

	int index = 0;

	while(list_get(metadataFile->block, index) != NULL){

		char* auxblock= (char*)list_get(metadataFile->block, index);

		char* block =(char*)malloc(strlen(auxblock)+1);
		strcpy(block,list_get(metadataFile->block, index));

		char* blockFile = (char*)malloc(strlen(GameCard->blocksPath) + strlen(block) + strlen(".bin") + 1 );
		strcpy(blockFile,GameCard->blocksPath);
		strcat(blockFile,block);
		strcat(blockFile,".bin");

		remove(blockFile);

		turn_bit_off(atoi(auxblock));

		free(block);
		free(blockFile);
		index++;

	}

}

char* get_amount_of_pokemons(char* line){

	int index1=0;
	int index2=0;
	while (line[index1] != '=') index1++;

	while (line[index1 + index2] != '\n') index2++;

	return string_substring(line,index1+1,index2-1);
}

char* get_x_coordinate(char* line){
	int index1=0;
	while (line[index1] != '-') index1++;
	return string_substring(line,0,index1);
}

char* get_y_coordinate(char* line){
	int index1=0;
	int index2=0;
	while (line[index1] != '-' ) index1++;
	while (line[index2+index1] != '=') index2++;
	return string_substring(line,index1+1,index2-1);

}

//BITARRAY LOGIC

int get_first_free_block(){
	int max = bitarray_get_max_bit(GameCard->bitArray);
	int i = 0;
	while((bitarray_test_bit(GameCard->bitArray,i)) && (i<=max))
	{
		i++;
	}
	return i;
}

void turn_bit_on(int block){
	bitarray_set_bit(GameCard->bitArray, block);
	sem_wait(&(GameCard->semMap));
	memcpy(GameCard->fileMapped, GameCard->bitArray->bitarray, (GameCard->blocks/8));
	msync(GameCard->fileMapped, (GameCard->blocks/8) , MS_SYNC);
	sem_post(&(GameCard->semMap));
	log_info(GameCard->logger, "Se solicitó el bloque %i ", block);
}

void turn_bit_off(int block){
	bitarray_clean_bit(GameCard->bitArray, block);
	sem_wait(&(GameCard->semMap));
	memcpy(GameCard->fileMapped, GameCard->bitArray->bitarray, (GameCard->blocks/8));
	msync(GameCard->fileMapped, (GameCard->blocks/8) , MS_SYNC);
	sem_post(&(GameCard->semMap));
	log_info(GameCard->logger, "Se libero el bloque %i", block);
}


// UTILS

int get_message_size(new_pokemon* newPokemon){

	int size=0;

	char* horCoordinate = string_itoa(newPokemon->horizontalCoordinate);
	char* verCoordinate = string_itoa(newPokemon->verticalCoordinate);
	char* amount = string_itoa(newPokemon->ammount);

	//Sumo 1 por el \n
	size = (strlen(horCoordinate)) + 1 + (strlen(verCoordinate)) + 1 + (strlen(amount)) + 1;
	free(horCoordinate);
	free(verCoordinate);
	free(amount);

	return size;
}

int get_amount_of_blocks(int size, t_file_metadata* metadataFile){

	int auxSize = 0;
	int availableSpaceInblock =0;

	int amountOfBlocks = list_size(metadataFile->block);

	// Logica para usar el espacio restante del último bloque
	if(amountOfBlocks != 0)
		availableSpaceInblock = (GameCard->block_size)*amountOfBlocks - atoi(metadataFile->size);

	if (availableSpaceInblock > 0)
		auxSize = size - availableSpaceInblock;
	else
		auxSize = size;

	if (auxSize <= 0) return 0;

	if (auxSize % GameCard->block_size == 0)
	    return auxSize/GameCard->block_size;
	else
	    return (auxSize/GameCard->block_size) + 1;

}

char* serialize_data(int size, new_pokemon* newPokemon ){
	int tam;

	char* buffer=(char*)malloc(size + 1);

	char* horCoordinate = string_itoa(newPokemon->horizontalCoordinate);
	char* verCoordinate = string_itoa(newPokemon->verticalCoordinate);
	char* amount = string_itoa(newPokemon->ammount);

	strcpy(buffer,horCoordinate);
	strcat(buffer,"-");
	strcat(buffer,verCoordinate);
	strcat(buffer,"=");
	strcat(buffer,amount);;

	tam = strlen(buffer);
	buffer[strlen(buffer)] ='\n';
	buffer[tam + 1]='\0';

	free(horCoordinate);
	free(verCoordinate);
	free(amount);

	return buffer;
}

void write_blocks(t_file_metadata* metadataFile, char* buffer ){

	int cantBytes=0;
	int index=0;
	int tam = strlen(buffer);


	char* bufferAux=(char*)malloc(GameCard->block_size);

	t_values* values = (t_values*)malloc(sizeof(t_values));
	values->values =list_create();
	int i=0;

	while(list_get(metadataFile->block, i) != NULL)
	{

		char* charblock = string_duplicate((char*)list_get(metadataFile->block,i));

		int bitBlock = atoi(charblock);
		list_add(values->values,(void*)bitBlock);

		index = 0;
		while((GameCard->block_size > index) && (index < tam)){
			bufferAux[index]=buffer[cantBytes];
			cantBytes++;
			index++;
		}
		tam=tam-GameCard->block_size;

		list_add(values->values,bufferAux);
		list_add(values->values,(void*)index);

		create_file(BIN_FILE,values);

		list_remove(values->values,2);
		list_remove(values->values,1);
		list_remove(values->values,0);


		free(bufferAux);
		free(charblock);
		bufferAux=(char*)malloc(GameCard->block_size);
		i++;
	}

	free(bufferAux);
	list_destroy(values->values);
	free(values);
}

void rewrite_blocks(t_file_metadata* metadataFile, char* fileContent ){

	int index = 0;
	int currentBlock = 0;

	while(list_get(metadataFile->block, index) != NULL){

		char* auxblock= (char*)list_get(metadataFile->block, index);

		char* block =(char*)malloc(strlen(auxblock)+1);
		strcpy(block,list_get(metadataFile->block, index));

		char* blockFile = (char*)malloc(strlen(GameCard->blocksPath) + strlen(block) + strlen(".bin") + 1 );
		strcpy(blockFile,GameCard->blocksPath);
		strcat(blockFile,block);
		strcat(blockFile,".bin");

		FILE* f= fopen(blockFile,"wb");

		if (f==NULL){
			return ;
		}

		char* bufferAux = string_substring(fileContent,currentBlock,GameCard->block_size);

		fwrite(bufferAux,GameCard->block_size,1,f);

		currentBlock = currentBlock + GameCard->block_size;

		free(bufferAux);
		fclose(f);
		free(block);
		free(blockFile);
		index++;

	}

}

int get_string_file_position(char* fileContent, char* coordinates){


	int line = 0;
	int match = 0;

	while(!match)
	{
		int pos=0;

		while(fileContent[pos + line] != '\n') pos++;

		char* stringAux = string_substring(fileContent,line,pos+1);

		if (string_contains(stringAux,coordinates))
			match=1;
		else
			line = line+pos+1;

		free(stringAux);
	}

	return line;
}

int check_directory(char* directory){

	struct stat st = {0};

	if (stat(directory, &st) == -1)
	{
		return 0;
	}
	else
	{
		return 2;
	}
}

int create_directory(char* directory){
	int result;

	result=check_directory(directory);

	if (result == 0) {
		result=mkdir(directory, 0700);
	    if (result!=0){
	    	return errno;
	    }
	    else{
		    return 0;
	    }
	}
	else
	{
		return result;
	}
}


//BROKER LOGIC
void readConfigBrokerValues(t_config *config,t_log *logger,struct Broker *broker){
	if (config_has_property(config,broker->ipKey)){
		char* aux = config_get_string_value(config,broker->ipKey);
		broker->ip=(char*)malloc(strlen(aux)+1);
		strcpy(broker->ip,aux);
	}else{
		exit(-3);
	}

	if (config_has_property(config,broker->portKey)){
		char* aux = config_get_string_value(config,broker->portKey);
		broker->port=(char*)malloc(strlen(aux)+1);
		strcpy(broker->port,aux);
	}else{
		exit(-3);
	}
}

void subscribeToBroker(struct Broker broker,pthread_t* subs){

	pthread_create(&(subs[0]),NULL,subscribeToBrokerNew,NULL);
	sleep(2);
	pthread_create(&(subs[1]),NULL,subscribeToBrokerCatch,NULL);
	sleep(2);
	pthread_create(&(subs[2]),NULL,subscribeToBrokerGet,NULL);
	sleep(10);
}

void* subscribeToBrokerNew(){
	int socketNew;

	int connectionSuccess = 0;
	while (!connectionSuccess){
		socketNew = connectBroker(broker.ip,broker.port,GameCard->logger);
		if (socketNew > 0){
			if (-1==SendSubscriptionRequest(NEW_POKEMON,socketNew)){
				sleep(GameCard->retryConnection);
			}else{
				connectionSuccess=1;
			}
		}else{
			log_info(GameCard->logger, "Reintento de conexión a la queue New en %d segundos", GameCard->retryConnection);
			sleep(GameCard->retryConnection);
		}
	}

	t_args* args= (t_args*) malloc (sizeof (t_args));

	args->suscription = socketNew;
	args->queueType = NEW_POKEMON;

	pthread_create(thread,NULL,(void*)GameCard_Wait_For_Message,args);
	pthread_detach(*thread);

	pthread_exit(NULL);
}

void* subscribeToBrokerCatch(){
	int socketCatch;
	int connectionSuccess = 0;

	while (!connectionSuccess){
		socketCatch = connectBroker(broker.ip,broker.port,GameCard->logger);
		if (socketCatch > 0){
			if(-1==SendSubscriptionRequest(CATCH_POKEMON,socketCatch)){
				sleep(GameCard->retryConnection);
			}else{
				connectionSuccess=1;
			}
		}else{
			log_info(GameCard->logger, "Reintento de conexión a la queue Catch en %d segundos", GameCard->retryConnection);
			sleep(GameCard->retryConnection);
		}
	}
	t_args* args= (t_args*) malloc (sizeof (t_args));

	args->suscription = socketCatch;
	args->queueType = CATCH_POKEMON;

	pthread_create(thread,NULL,(void*)GameCard_Wait_For_Message,args);
	pthread_detach(*thread);

	pthread_exit(NULL);
}

void* subscribeToBrokerGet(){
	int socketGet;
	int connectionSuccess = 0;

	while (!connectionSuccess){
		socketGet = connectBroker(broker.ip,broker.port,GameCard->logger);
		if (socketGet > 0){
			if(-1==SendSubscriptionRequest(GET_POKEMON,socketGet)){
			}else{
				connectionSuccess = 1;
			}
		}
		else
		{
			log_info(GameCard->logger, "Reintento de conexión a la queue Get en %d segundos", GameCard->retryConnection);
			sleep(GameCard->retryConnection);
		}
	}

	t_args* args= (t_args*) malloc (sizeof (t_args));

	args->suscription = socketGet;
	args->queueType = GET_POKEMON;

	pthread_create(thread,NULL,(void*)GameCard_Wait_For_Message,args);
	pthread_detach(*thread);

	pthread_exit(NULL);
}

int connectBroker(char* ip, char* puerto,t_log* logger)
{
	int teamSocket;

    struct addrinfo hints, *servinfo, *p;

    memset(&hints, 0, sizeof(hints));
    hints.ai_family = AF_UNSPEC;
    hints.ai_socktype = SOCK_STREAM;
    hints.ai_flags = 0;
    hints.ai_protocol = 0;

    getaddrinfo(ip, puerto, &hints, &servinfo);
    for (p = servinfo; p != NULL; p = p->ai_next) {
    	teamSocket = socket(p->ai_family, p->ai_socktype, p->ai_protocol);
		if (teamSocket == -1){
			return -2;
		}
		if (connect(teamSocket, p->ai_addr, p->ai_addrlen)==0) {
			freeaddrinfo(servinfo);
			return teamSocket;
		}else{
			close(teamSocket);
			return -1;
		}
	}
    return -1;
}

// Manage an interupt or segmentation fault
void signaltHandler(int sig_num){
	log_debug(GameCard->logger, "SE INTERRUMPIO EL PROCESO");

	munmap(GameCard->fileMapped, (GameCard->blocks/8));

	destroy_poke_dictionary(pokeSemaphore);

	GameCard_Destroy(GameCard);
	exit(-5);
}

