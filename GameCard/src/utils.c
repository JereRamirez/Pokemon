#include"utils.h"

int iniciar_servidor(char* ip, char* puerto)
{
	int socket_servidor;

    struct addrinfo hints, *servinfo, *p;

    memset(&hints, 0, sizeof(hints));
    hints.ai_family = AF_UNSPEC;
    hints.ai_socktype = SOCK_STREAM;
    hints.ai_flags = AI_PASSIVE;

    getaddrinfo(ip, puerto, &hints, &servinfo);

    for (p=servinfo; p != NULL; p = p->ai_next)
    {
        if ((socket_servidor = socket(p->ai_family, p->ai_socktype, p->ai_protocol)) == -1)
            continue;

        uint32_t flag=1;
        setsockopt(socket_servidor,SOL_SOCKET,SO_REUSEPORT,&(flag),sizeof(flag));

        if (bind(socket_servidor, p->ai_addr, p->ai_addrlen) == -1) {
            close(socket_servidor);
            continue;
        }
        break;
    }

	listen(socket_servidor, SOMAXCONN);

    freeaddrinfo(servinfo);

    return socket_servidor;
}

int esperar_cliente(int socket_servidor)
{
	struct sockaddr_in dir_cliente;

	int tam_direccion = sizeof(struct sockaddr_in);

	return accept(socket_servidor, (void*) &dir_cliente, &tam_direccion);

}


t_log* iniciar_logger(void)
{
	return log_create("GameCard.log","GameCard",1,LOG_LEVEL_DEBUG);
}

t_config* read_config(void)
{
	return config_create("/home/utnso/workspace/tp-2020-1c-MATE-OS/GameCard/GameCard.config");

}

t_config* read_metadata(char* file)
{
	return config_create(file);
}

char* get_config_value(t_config* config, char* propiedad){

	if (config_has_property(config,propiedad)){
//		log_info(logger,config_get_string_value(config,propiedad));
		return config_get_string_value(config, propiedad);
	} else {
//		log_debug(logger,"Error al obtener el atributo");
		return NULL;
	}
}

char ** get_config_value_array(t_config* config, char* propiedad){

	if (config_has_property(config,propiedad)){
//		log_info(logger,"Obtiene el array de datos");
		return config_get_array_value(config, propiedad);
	} else {
//		log_debug(logger,"Error al obtener el atributo");
		return NULL;
	}
}

