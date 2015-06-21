#ifndef SRC_SOCKET_H_
#define SRC_SOCKET_H_

#include <stddef.h>
#include <stdint.h>

//**********************************************************************************//
//									SOCKETS											//
//**********************************************************************************//

typedef enum {
	SOCKET_ERROR_NONE			= 1,
	SOCKET_ERROR_ADDRESS		= -1,
	SOCKET_ERROR_SOCKET			= -2,
	SOCKET_ERROR_SETSOCKETOPT	= -3,
	SOCKET_ERROR_BIND			= -4,
	SOCKET_ERROR_LISTEN			= -5,
	SOCKET_ERROR_ACCEPT			= -6,
	SOCKET_ERROR_CONNECT		= -7,
	SOCKET_ERROR_SEND			= -8,
	SOCKET_ERROR_RECV			= -9,
	SOCKET_ERROR_SHUTDOWN		= -10,
	SOCKET_ERROR_HANDSHAKE		= -11,
	SOCKET_ERROR_WELCOME		= -12,
	SOCKET_ERROR_CLOSE			= -13,
	SOCKET_ERROR_TIMEOUT_RECV	= -14,
	SOCKET_ERROR_TIMEOUT_SEND	= -15
} e_socket_status;

typedef uint16_t t_port;

int socket_listen(t_port port);
int socket_accept(int listener);
int socket_accept_and_get_ip(int fdListener, char **ipAddress);
int socket_connect(const char* ip, t_port port);
e_socket_status socket_close(int socket);

//**********************************************************************************//
//									INTEGER											//
//**********************************************************************************//

int isBigEndian();
int isLittleEndian();
void* hton(void* value, size_t size);
e_socket_status socket_send_integer(int socket, void* integer, size_t size);
e_socket_status socket_recv_integer(int socket, void* integer, size_t size);

//**********************************************************************************//
//									STREAM											//
//**********************************************************************************//

e_socket_status socket_send(int socket, void* stream, size_t size);
e_socket_status socket_recv(int socket, void* stream, size_t size);

//**********************************************************************************//
//									PACKET											//
//**********************************************************************************//

e_socket_status socket_send_packet(int socket, void* packet, size_t size);
e_socket_status socket_recv_packet(int socket, void** packet, size_t* size);

//**********************************************************************************//
//									STRING											//
//**********************************************************************************//

e_socket_status socket_send_string(int socket, char* string);
e_socket_status socket_recv_string(int socket, char** string);

//**********************************************************************************//
//									HANDSHAKE										//
//**********************************************************************************//

#define HANDSHAKE_MARTA 0x02
#define HANDSHAKE_FILESYSTEM 0x04
#define HANDSHAKE_JOB 0x08
#define HANDSHAKE_NODO 0x10

int socket_handshake_to_server(int socket, int hiserver, int hiclient);
int socket_handshake_to_client(int socket, int hiserver, int hiclient);

#endif
