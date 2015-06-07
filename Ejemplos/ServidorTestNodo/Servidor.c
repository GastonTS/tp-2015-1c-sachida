/*
 * Servidor.c
 *
 *  Created on: 6/6/2015
 *      Author: utnso
 */

#include "Servidor.h"
#include "../utils/socket.h"

int main(){

	int my_socket;
	int socket_nodo;
	t_port puerto;
	int nodo;

	puerto = 5000;
	logger = log_create("Log.txt", "Servidor",1, log_level_from_string("DEBUG"));
	log_info(logger,"puerto: %d",puerto);
	log_info(logger,"Arranque");
	my_socket = socket_listen(puerto);
	log_info(logger,"Ando el listen");
	socket_nodo = socket_accept(my_socket);
	log_info(logger,"Anduvo el acept");
	nodo = socket_handshake_to_client(my_socket,HANDSHAKE_NODO,HANDSHAKE_FILESYSTEM);
	log_info(logger,"Nodo: %d",nodo);
	log_destroy(logger);
	return 1;
}

