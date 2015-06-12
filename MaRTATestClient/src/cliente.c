#include <stdlib.h>
#include <unistd.h>
#include <stdio.h>
#include "../../utils/socket.h"

int fd;
int fd2;

int main() {
	fd = socket_connect("127.0.0.1", 30000);
	int a = socket_handshake_to_server(fd, HANDSHAKE_MARTA, HANDSHAKE_FILESYSTEM);
	printf("\n\n%i\n\n", a);
	fflush(stdout);

	fd2 = socket_connect("127.0.0.1", 30000);
	a = socket_handshake_to_server(fd2, HANDSHAKE_MARTA, HANDSHAKE_JOB);
	printf("\n\n%i\n\n", a);
	fflush(stdout);
	return EXIT_SUCCESS;
}
