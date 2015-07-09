#include "socket.h"
#include <errno.h>
#include <netdb.h>
#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <string.h>
#include <signal.h>

// Siempre que halla if (0> algo) significa "Si hay error"
// Siempre que halla if (0< algo) significa "Si NO hay error"
//**********************************************************************************//
//									SOCKETS											//
//**********************************************************************************//

void socket_set_port_string(t_port port, char *portStr) {
	sprintf(portStr, "%d", port);
}

int socket_listen(t_port port) {
	signal(SIGPIPE, SIG_IGN);

	struct addrinfo hints;
	struct addrinfo *serverInfo;

	memset(&hints, 0, sizeof(hints));
	hints.ai_family = AF_UNSPEC;		// No importa si uso IPv4 o IPv6
	hints.ai_flags = AI_PASSIVE;		// Asigna el address del localhost: 127.0.0.1
	hints.ai_socktype = SOCK_STREAM;	// Indica que usaremos el protocolo TCP

	char portStr[10];
	socket_set_port_string(port, portStr);
	getaddrinfo(NULL, portStr, &hints, &serverInfo);

	int fd = socket(serverInfo->ai_family, serverInfo->ai_socktype, serverInfo->ai_protocol);

	if (0 > fd) {
		freeaddrinfo(serverInfo);
		return SOCKET_ERROR_SOCKET;
	}

	if (0 > bind(fd, serverInfo->ai_addr, serverInfo->ai_addrlen)) {
		freeaddrinfo(serverInfo);
		return SOCKET_ERROR_BIND;
	}

	int yes = 1;
	if (setsockopt(fd, SOL_SOCKET, SO_REUSEADDR, &yes, sizeof(int)) == -1) {
		return SOCKET_ERROR_SETSOCKETOPT;
	}

	freeaddrinfo(serverInfo);

	if (0 > listen(fd, 5)) {
		return SOCKET_ERROR_LISTEN;
	}

	return fd;
}

// Accept without getting the client's IP
int socket_accept(int fdListener) {
	char *IP = NULL;
	int fd = socket_accept_and_get_ip(fdListener, &IP);
	if (IP) {
		free(IP);
	}
	return fd;
}

// Accept getting the client's IP
int socket_accept_and_get_ip(int fdListener, char **ipAddress) {
	struct sockaddr_in addr;
	socklen_t addrlen = sizeof(addr);
	int fd = accept(fdListener, (struct sockaddr *) &addr, &addrlen);

	if (0 > fd) {
		return SOCKET_ERROR_ACCEPT;
	}

	// set the ip.
	*ipAddress = malloc(INET_ADDRSTRLEN);
	inet_ntop(AF_INET, &addr.sin_addr.s_addr, *ipAddress, INET_ADDRSTRLEN);

	return fd;
}

int socket_connect(const char* ip, t_port port) {
	signal(SIGPIPE, SIG_IGN);

	struct addrinfo hints;
	struct addrinfo *serverInfo;

	memset(&hints, 0, sizeof(hints));
	hints.ai_family = AF_UNSPEC;		// Permite que la maquina se encargue de verificar si usamos IPv4 o IPv6
	hints.ai_socktype = SOCK_STREAM;	// Indica que usaremos el protocolo TCP

	char portStr[10];
	socket_set_port_string(port, portStr);
	getaddrinfo(ip, portStr, &hints, &serverInfo);

	if (!serverInfo) {
		return SOCKET_ERROR_ADDRESS;
	}

	int fd = socket(serverInfo->ai_family, serverInfo->ai_socktype, serverInfo->ai_protocol);

	if (0 > fd) {
		freeaddrinfo(serverInfo);
		return SOCKET_ERROR_SOCKET;
	}

	if (0 > connect(fd, serverInfo->ai_addr, serverInfo->ai_addrlen)) {
		close(fd);
		freeaddrinfo(serverInfo);
		return SOCKET_ERROR_CONNECT;
	}
	freeaddrinfo(serverInfo);

	return fd;
}

e_socket_status socket_close(int socket) {
	//if (0 > close(socket)) {
	if (0 > shutdown(socket, SHUT_RDWR)) {
		return SOCKET_ERROR_CLOSE;
	}
	return SOCKET_ERROR_NONE;
}

//**********************************************************************************//
//									INTEGER											//
//**********************************************************************************//

//chequea el endianess
int isBigEndian() {
	return !isLittleEndian();
}

int isLittleEndian() {
	static const int THE_ANSWER = 42;
	return (*((const char*) &THE_ANSWER) == THE_ANSWER);
}

//hton un poco mas generico, tenes que mandarle el sizeoff del tipo de dato que mandes
void* hton(void* value, size_t size) {
	if (isLittleEndian()) {
		void* l = value;
		void* a = malloc(1);
		void* r = value + size - 1;
		for (; l < r; l++, r--) {
			memcpy(a, l, 1);
			memcpy(l, r, 1);
			memcpy(r, a, 1);
		}
		free(a);
	}
	return value;
}

//send y recv especificos para integer (con el hton generico pasando el tamaño por param)
e_socket_status socket_send_integer(int socket, void* integer, size_t size) {
	e_socket_status status = socket_send(socket, hton(integer, size), size);
	hton(integer, size);
	return status;
}

e_socket_status socket_recv_integer(int socket, void* integer, size_t size) {
	e_socket_status status = socket_recv(socket, integer, size);
	hton(integer, size);
	return status;
}

//**********************************************************************************//
//									STREAM											//
//**********************************************************************************//

e_socket_status socket_send(int socket, void* stream, size_t size) {
	if (!stream) {
		return SOCKET_ERROR_SEND;
	}

	void* data = stream; //data es el puntero que voy a usar para ir avanzando en el stream y saber desde donde empezar
	size_t tosend = size; //tosend es el tamaño total a mandar y count es lo que mando en ese send especifico
	int count = 0;
	while (tosend) {

		int retry = 3;
		while (0 >= (count = send(socket, data, tosend, 0)) && retry--) {
			switch (errno) {
			case ECONNREFUSED:
			case EAGAIN:
			case ETIMEDOUT:
				usleep(1000000);
				break;
			case EPIPE:
			default:
				return SOCKET_ERROR_SEND;
			}
		}

		if (!count || 0 > count) {
			return SOCKET_ERROR_SEND;
		}

		tosend -= count;
		data += count;
	}

	return SOCKET_ERROR_NONE;
}

//Similar a la anterior pero con recv (no sirve para multiplexado
e_socket_status socket_recv(int socket, void* stream, size_t size) {
	if (!stream) {
		return SOCKET_ERROR_RECV;
	}

	void *data = stream; // Saves a pointer to keep moving until all size is read.
	size_t left = size; // Saves the left size to read.s
	int count;
	while (left) {
		int retry = 3;
		while (0 >= (count = recv(socket, data, left, 0)) && retry--) {
			switch (errno) {
			case ECONNREFUSED:
			case EAGAIN:
			case ETIMEDOUT:
				usleep(1000000);
				break;
			case EPIPE:
			default:
				return SOCKET_ERROR_RECV;
			}
		}

		if (!count || 0 > count) {
			return SOCKET_ERROR_RECV;
		}

		left -= count;
		data += count; // Moves pointer forward
	}

	return SOCKET_ERROR_NONE;
}

//**********************************************************************************//
//									PACKET											//
//**********************************************************************************//
//Los paquetes son cuando no se el tamaño total que voy a mandar (no es un tamaño fijo)

e_socket_status socket_send_packet(int socket, void* packet, size_t size) {
	if (!packet)
		return SOCKET_ERROR_SEND;
	size_t ssize = sizeof(size_t),  //El tamaño del tamaño de lo que va a mandar
			spacket = size, 	//el tamaño de lo que realmente va a mandar
			sbufer = ssize + spacket; //El tamaño de header+contenido
	void* buffer = malloc(sbufer);
	hton(memcpy(buffer, &size, ssize), ssize); //copia el tamaño de lo que va mandar
	memcpy(buffer + ssize, packet, spacket); //copia el contenido del paquete en el buffer
	e_socket_status status = socket_send(socket, buffer, sbufer); //manda el buffer
	free(buffer);
	return status;
}

e_socket_status socket_recv_packet(int socket, void** packet, size_t* size) {
	e_socket_status status = socket_recv_integer(socket, size, sizeof(size_t));
	if (0 > status)
		return status;
	*packet = malloc(*size); //hace un malloc del tamaño que tiene que recibir (parametro)
	status = socket_recv(socket, *packet, *size); //recibe ese tamaño en el buffer (packet) para desserializarlo en implementacion particular
	if (0 > status)
		free(*packet);
	return status;
}

//**********************************************************************************//
//									STRING											//
//**********************************************************************************//
//estas son implementaciones particulares de como mandar un paquete, si por ejemplo es un char* (string)

e_socket_status socket_send_string(int socket, char* string) {
	if (!string)
		return SOCKET_ERROR_SEND;
	return socket_send_packet(socket, string, sizeof(char) * (strlen(string) + 1));
}

e_socket_status socket_recv_string(int socket, char** string) {
	size_t size;
	e_socket_status status = socket_recv_packet(socket, (void**) string, &size);
	if (0 > status)
		return status;
	if (size != sizeof(char) * (1 + strlen(*string))) {
		free(*string);
		return SOCKET_ERROR_SEND;
	}
	return status;
}

//**********************************************************************************//
//									HANDSHAKE										//
//**********************************************************************************//
//Es un paso inicial antes de empezar la comunicacion para saber con quien estamos hablando
//Uso define para que se lea bien, y se entienda relativamente facil

#define HANDSHAKE_WELCOME 0x01
#define _HANDSHAKE_WELCOME "WELCOME"
#define _HANDSHAKE_MARTA "IMMARTA"
#define _HANDSHAKE_FILESYSTEM "IMFILESYSTEM"
#define _HANDSHAKE_NODO "IMNODO"
#define _HANDSHAKE_JOB "IMJOB"

//recibe un string y devuelve el hexa
static int _get_handshake_code(char* handshake) {
	if (!strcmp(handshake, _HANDSHAKE_WELCOME))
		return HANDSHAKE_WELCOME;
	if (!strcmp(handshake, _HANDSHAKE_MARTA))
		return HANDSHAKE_MARTA;
	if (!strcmp(handshake, _HANDSHAKE_FILESYSTEM))
		return HANDSHAKE_FILESYSTEM;
	if (!strcmp(handshake, _HANDSHAKE_NODO))
		return HANDSHAKE_NODO;
	if (!strcmp(handshake, _HANDSHAKE_JOB))
		return HANDSHAKE_JOB;
	return 0x00;
}

//recibe el hexa y devuelve un string
static char* _get_handshake_msg(int handshake) {
	switch (handshake) {
	case HANDSHAKE_MARTA:
		return _HANDSHAKE_MARTA;
	case HANDSHAKE_FILESYSTEM:
		return _HANDSHAKE_FILESYSTEM;
	case HANDSHAKE_NODO:
		return _HANDSHAKE_NODO;
	case HANDSHAKE_JOB:
		return _HANDSHAKE_JOB;
	}
	return NULL;
}

//Recibe un handshake y devuelve el hexa
static int _check_handshake(int socket, int handshake) { //int handshake son los permitidos (ver to client)
	char* hi;
	int recive = socket_recv_string(socket, &hi); //HACE EL RECV
	if (0 > recive)
		return recive;
	int hscode = _get_handshake_code(hi); //consigue el hexa
	free(hi);
	return hscode & handshake ? hscode : SOCKET_ERROR_HANDSHAKE; //Devuelve el hexa si es de los permitidos a travez de una multiplicacion binaria (&)
}

//El cliente le envia el handshake al servidor, se usa despues del connect
int socket_handshake_to_server(int socket, int hiserver, int hiclient) {
	int server = _check_handshake(socket, hiserver); //Primero recibe el handshake del servidor
	if (0 > server)
		return server;
	if (0 > socket_send_string(socket, _get_handshake_msg(hiclient))) //Envia su handshake al servidor
		return SOCKET_ERROR_SEND;
	int wellcome = _check_handshake(socket, HANDSHAKE_WELCOME); //Espera a que el servidor le de el ok (WELLCOME)
	if (wellcome == SOCKET_ERROR_HANDSHAKE)
		wellcome = SOCKET_ERROR_WELCOME;
	return 0 > wellcome ? wellcome : server; //devuelve wellcome (error, o 1 si esta ok)
}

//El servidor le envia el handshake al cliente, se usa despues del accept
//hiclient se debe expresar como una suma binaria (or | ) si es mas de uno asi se permite pasar a todos esos, una posible llamada seria
//switch(socket_handshake_to_client(fd, HANDSHAKE_MARTA, HNADSHAKE_JOB | HANDSHAKE_FILESYSTEM)) dependiendo del resultado
//se que accion tomar, porque me devuelve cual handshake recibi.
int socket_handshake_to_client(int socket, int hiserver, int hiclient) {
	if (0 > socket_send_string(socket, _get_handshake_msg(hiserver))) //Primero envia el handshake al cliente
		return SOCKET_ERROR_SEND;
	int client = _check_handshake(socket, hiclient); //Se fija si el recibido esta en los permitidos (hiclient)
	if (0 > client)
		return client;
	if (0 > socket_send_string(socket, _HANDSHAKE_WELCOME)) //Si esta permitido envia el wellcome
		return SOCKET_ERROR_WELCOME;
	return client;
}
