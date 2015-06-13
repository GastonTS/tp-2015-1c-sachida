#include <stdlib.h>
#include <unistd.h>
#include <string.h>
#include <stdint.h>
#include <stdio.h>
#include <stdbool.h>
#include <commons/collections/list.h>
#include <arpa/inet.h>
#include "../../utils/socket.h"

void serializeJobToMaRTA(int fd, bool combiner, t_list *files);
int fd;

int main() {
	t_list *files = list_create();
	list_add(files, "sarasa");
	list_add(files, "sarasa");
	list_add(files, "sarasa");
	list_add(files, "sarasa");
	list_add(files, "sarasa");

	fd = socket_connect("127.0.0.1", 30000);
	int hand = socket_handshake_to_server(fd, HANDSHAKE_MARTA, HANDSHAKE_JOB);
	if (!hand) {
		printf("Error al conectar\n");
		return EXIT_FAILURE;
	}
	bool combiner = false;
	serializeJobToMaRTA(fd, combiner, files);
	list_destroy(files);
	return EXIT_SUCCESS;
}

size_t lengthStringList(t_list *stringList) {
	size_t length = 0;
	void totalLength(char *string) {
		length += (strlen(string) + 1);
	}
	list_iterate(stringList, (void *) totalLength);
	return sizeof(char) * length;
}

void serializeJobToMaRTA(int fd, bool combiner, t_list *files) {
	size_t filesLength = lengthStringList(files);
	char *stringFiles = malloc(filesLength);
	strcpy(stringFiles, "");
	void listToString(char *file) {
		strcat(stringFiles, file);
		strcat(stringFiles, " ");
	}
	list_iterate(files, (void *) listToString);
	stringFiles[filesLength - 1] = '\0';

	size_t scombiner = sizeof(combiner);
	size_t sbuffer = scombiner + filesLength;
	void *buffer = malloc(sbuffer);
	combiner = htonl(combiner);
	memcpy(buffer, &combiner, scombiner);
	memcpy((buffer + scombiner), stringFiles, filesLength);

	socket_send_packet(fd, buffer, sbuffer);
	free(stringFiles);
	free(buffer);
}
