#include "socket.h"
#include <errno.h>
#include <netdb.h>
#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <string.h>

// Siempre que halla if (0> algo) significa "Si hay error"
// Siempre que halla if (0< algo) significa "Si NO hay error"
//**********************************************************************************//
//									SOCKETS											//
//**********************************************************************************//

typedef struct sockaddr_in t_sockaddr_in;
typedef struct sockaddr* p_sockaddr;

int socket_setoption(int fd) { //Setea el tiempo para reintentar algo por ese fd
	struct timeval timeout;
	timeout.tv_sec = 10;
	timeout.tv_usec = 0; //microsegundos
	if (0 > setsockopt(fd, SOL_SOCKET, SO_RCVTIMEO, (char *) &timeout, sizeof(timeout)))
		return SOCKET_ERROR_TIMEOUT_RECV;

	if (0 > setsockopt(fd, SOL_SOCKET, SO_SNDTIMEO, (char *) &timeout, sizeof(timeout)))
		return SOCKET_ERROR_TIMEOUT_SEND;

	return SOCKET_ERROR_NONE;
}

static p_sockaddr _getaddr(const char* ip, t_port port) { //Consigue un struct sockaddr* en base a un ip y un puerto
	t_sockaddr_in* addr = malloc(sizeof(t_sockaddr_in));
	addr->sin_family = AF_INET;
	addr->sin_port = htons(port);
	if (ip) { //Si mando una ip, consigue el ip de ese dominio con gethostbyname
		struct hostent* host = gethostbyname(ip);
		if (!host)
			return NULL;
		addr->sin_addr = *((struct in_addr *) host->h_addr);
		//free(host); free(ip); NO HACER: double free or corruption porque se usa en los demas
	} else { //Si no manda una ip, pongo la de la maquina local
		addr->sin_addr.s_addr = INADDR_ANY;
		memset(&(addr->sin_zero), '\0', 8);
	}
	return (p_sockaddr) addr;
}

static p_sockaddr _getlocaladdr(t_port port) {
	return _getaddr(NULL, port); //llama la anterior para la maquina local
}

int socket_listen(t_port port) {
	//Prepara el file descriptor
	int fd = socket(AF_INET, SOCK_STREAM, PF_UNSPEC);
	if (0 > fd)
		return SOCKET_ERROR_SOCKET;
	int yes = 1;
	if (0 > setsockopt(fd, SOL_SOCKET, SO_REUSEADDR, &yes, sizeof(int)))
		return SOCKET_ERROR_SETSOCKETOPT;
	//Crea un addr (local, con el puerto paramtro) y la bindea a ese fd
	p_sockaddr addr = _getlocaladdr(port);
	if (!addr)
		return SOCKET_ERROR_ADDRESS;
	if (0 > bind(fd, addr, sizeof(t_sockaddr_in))) {
		free(addr);
		return SOCKET_ERROR_BIND;
	}
	free(addr);
	//Hace el listen
	if (0 > listen(fd, 5))
		return SOCKET_ERROR_LISTEN;
	return fd;
}

int socket_accept(int fdListener) {
	struct sockaddr addr;
	socklen_t addrlen = sizeof(addr);
	int fd = accept(fdListener, &addr, &addrlen);
	//Usa retval para poder mandar error si hubo error en el setoption
	int retval = socket_setoption(fd);
	if (0 > retval)
		return retval;
	if (0 > fd)
		return SOCKET_ERROR_ACCEPT;
	return fd;
}

int socket_connect(const char* ip, t_port port) {
	int fd = socket(AF_INET, SOCK_STREAM, PF_UNSPEC);
	if (0 > fd)
		return SOCKET_ERROR_SOCKET;
	int retval = socket_setoption(fd);
	if (0 > retval)
		return retval;
	p_sockaddr addr = _getaddr(ip, port);
	if (!addr)
		return SOCKET_ERROR_ADDRESS;
	//Con el while reintenta conectar 3 veces, espera 3 segundos entre cada intento
	int retry = 3;
	while (0 > connect(fd, addr, sizeof(t_sockaddr_in)) && retry--) {
		switch (errno) {
		case ECONNREFUSED:
		case ETIMEDOUT:
			usleep(3000000);
			continue;
		default:
			free(addr);
			return SOCKET_ERROR_CONNECT;
		}
	}
	free(addr);
	//si salio por el retry tira error
	if (0 > retry) {
		close(fd);
		return SOCKET_ERROR_CONNECT;
	} else
		return fd;
}

e_socket_status socket_close(int socket) {
	if (0 > close(socket))
		return SOCKET_ERROR_CLOSE;
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
	if (!stream) //Si no envia bytes en el stream tira error
		return SOCKET_ERROR_SEND;
	void* data = stream; //data es el puntero que voy a usar para ir avanzando en el stream y saber desde donde empezar
	size_t count = 0, tosend = size; //tosend es el tamaño total a mandar y count es lo que mando en ese send especifico
	for (; tosend; tosend -= count, data += count) { //le resta a tosend lo que envio y mueve el puntero data para enviar desde ahi
		//otra vez el while para reintentar pero con send
		int retry = 3;
		while (0 > (count = send(socket, data, tosend, 0)) && retry--) {
			switch (errno) {
			case ECONNREFUSED:
			case ETIMEDOUT:
				usleep(3000000);
				continue;
			default:
				return SOCKET_ERROR_SEND;
			}
		}
		if (0 > count)
			return SOCKET_ERROR_SEND;
	}
	return SOCKET_ERROR_NONE;
}

//Similar a la anterior pero con recv (no sirve para multiplexado
e_socket_status socket_recv(int socket, void* stream, size_t size) {
	if (!stream)
		return SOCKET_ERROR_RECV;
	void* data = stream;
	size_t count = 0, torecv = size;
	for (; torecv; torecv -= count, data += count) {
		int retry = 3;
		while (0 > (count = recv(socket, data, torecv, 0)) && retry--) {
			switch (errno) {
			case ECONNREFUSED:
			case ETIMEDOUT:
				usleep(3000000);
				continue;
			default:
				return SOCKET_ERROR_RECV;
			}
		}
		if (0 > count)
			return SOCKET_ERROR_RECV;
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
	if (0 > socket_send_string(socket, _get_handshake_msg(hiserver)))//Primero envia el handshake al cliente
		return SOCKET_ERROR_SEND;
	int client = _check_handshake(socket, hiclient);//Se fija si el recibido esta en los permitidos (hiclient)
	if (0 > client)
		return client;
	if (0 > socket_send_string(socket, _HANDSHAKE_WELCOME))//Si esta permitido envia el wellcome
		return SOCKET_ERROR_WELCOME;
	return client;
}
