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
void desserializeMapOrder(void *buffer);
void desserializeReduceOrder(void *buffer, size_t sbuffer);
void serializeMapResult(int fd, bool result, uint16_t idMap);
void serializeReduceResult(int fd, bool result, uint16_t idResult, char failedTemp[60]);
void recvOrder(int fd);
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
	recvOrder(fd);
	serializeMapResult(fd, true, 1);
	recvOrder(fd);
	char failedTemp[60];
	memset(failedTemp, '\0', sizeof(char) * 60);
	strcpy(failedTemp, "TemporalFallido");
	serializeReduceResult(fd, false, 0, failedTemp);
	recvOrder(fd);
	return EXIT_SUCCESS;
}

//**********************************************************************************//
//									MaRTA											//
//**********************************************************************************//
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
	buffer = memset(buffer, '\0', sbuffer);
	combiner = htons(combiner);
	memcpy(buffer, &combiner, scombiner);
	memcpy((buffer + scombiner), stringFiles, filesLength);

	socket_send_packet(fd, buffer, sbuffer);
	free(stringFiles);
	free(buffer);
}

void recvOrder(int fd) {
	void *buffer;
	size_t sbuffer = 0;
	socket_recv_packet(fd, &buffer, &sbuffer);
	char order = '\0';
	size_t sOrder = sizeof(char);
	memcpy(&order, buffer, sOrder);
	printf("\nRECVORDER: %c\n", order);
	if (order == 'm')
		desserializeMapOrder(buffer + sOrder);
	else if (order == 'r')
		desserializeReduceOrder(buffer + sOrder, sbuffer - sOrder);
	else if (order == 'd') {
		printf("\nDIE JOB\n");
	}
	free(buffer);
}

//***********************************MAP********************************************//
void desserializeMapOrder(void *buffer) {
	size_t sIdMap = sizeof(uint16_t);
	size_t snumblock = sIdMap;
	size_t snodePort = sizeof(uint16_t);
	size_t stempName = sizeof(char) * 60;
	size_t snodeIP;

	uint16_t idMap;
	char* nodeIP;
	uint16_t nodePort;
	uint16_t numBlock;
	char tempResultName[60];

	memcpy(&idMap, buffer, sIdMap);
	memcpy(&snodeIP, buffer + sIdMap, sizeof(size_t));
	nodeIP = malloc(snodeIP);
	memcpy(nodeIP, buffer + sIdMap + sizeof(size_t), snodeIP);
	memcpy(&nodePort, buffer + sIdMap + sizeof(size_t) + snodeIP, snodePort);
	memcpy(&numBlock, buffer + sIdMap + sizeof(size_t) + snodeIP + snodePort, snumblock);
	memcpy(tempResultName, buffer + sIdMap + sizeof(size_t) + snodeIP + snodePort + snumblock, stempName);

	idMap = ntohs(idMap);
	nodePort = ntohs(nodePort);
	numBlock = ntohs(numBlock);

	//Test TODO: Adaptar a las estructuras de Job
	printf("\n%d\n", idMap);
	printf("%s\n", nodeIP);
	printf("%d\n", nodePort);
	printf("%d\n", numBlock);
	printf("%s\n", tempResultName);
	fflush(stdout);
	//End
}

void serializeMapResult(int fd, bool result, uint16_t idMap) {
	char resultFrom = 'm';
	size_t sresultFrom = sizeof(char);
	size_t sresult = sizeof(result);
	size_t sidMap = sizeof(uint16_t);
	size_t sbuffer = sresultFrom + sresult + sidMap;

	idMap = htons(idMap);

	void *buffer = malloc(sbuffer);
	buffer = memset(buffer, '0', sbuffer);
	memcpy(buffer, &resultFrom, sresultFrom);
	memcpy(buffer + sresultFrom, &result, sresult);
	memcpy(buffer + sresultFrom + sresult, &idMap, sidMap);
	socket_send_packet(fd, buffer, sbuffer);
}
//*********************************REDUCE*******************************************//
typedef struct {
	uint16_t originMap;
	char *nodeIP;
	uint16_t nodePort;
	char tempName[60];
} t_temp;

void desserializeTempToList(t_list *temporals, void *buffer, size_t *sbuffer) {
	t_temp *temporal = malloc(sizeof(t_temp));
	size_t snodeIP;

	memcpy(&temporal->originMap, buffer + *sbuffer, sizeof(uint16_t));
	temporal->originMap = ntohs(temporal->originMap);
	memcpy(&snodeIP, buffer + *sbuffer + sizeof(uint16_t), sizeof(snodeIP));
	temporal->nodeIP = malloc(snodeIP);
	memcpy(temporal->nodeIP, buffer + *sbuffer + sizeof(uint16_t) + sizeof(snodeIP), snodeIP);
	memcpy(&temporal->nodePort, buffer + *sbuffer + sizeof(uint16_t) + sizeof(snodeIP) + snodeIP, sizeof(uint16_t));
	temporal->nodePort = ntohs(temporal->nodePort);
	memcpy(temporal->tempName, buffer + *sbuffer + sizeof(uint16_t) + sizeof(snodeIP) + snodeIP + sizeof(uint16_t), sizeof(char) * 60);

	list_add(temporals, temporal);
	*sbuffer += sizeof(uint16_t) + sizeof(snodeIP) + snodeIP + sizeof(uint16_t) + sizeof(char) * 60;
}

void desserializeReduceOrder(void *buffer, size_t sbuffer) {
	size_t snodePort = sizeof(uint16_t);
	size_t stempName = sizeof(char) * 60;
	size_t snodeIP;

	char* nodeIP;
	uint16_t nodePort;
	char tempResultName[60];
	uint16_t countTemps = 0;

	memcpy(&snodeIP, buffer, sizeof(size_t));
	nodeIP = malloc(snodeIP);
	memcpy(nodeIP, buffer + sizeof(snodeIP), snodeIP);
	memcpy(&nodePort, buffer + sizeof(snodeIP) + snodeIP, snodePort);
	nodePort = ntohs(nodePort);
	memcpy(tempResultName, buffer + sizeof(snodeIP) + snodeIP + snodePort, stempName);
	memcpy(&countTemps, buffer + sizeof(snodeIP) + snodeIP + snodePort + stempName, sizeof(uint16_t));
	countTemps = ntohs(countTemps);
	void *tempsBuffer = malloc(sbuffer - sizeof(snodeIP) - snodeIP - snodePort - stempName - sizeof(uint16_t));
	tempsBuffer = buffer + sizeof(snodeIP) + snodeIP + snodePort + stempName + sizeof(uint16_t);
	size_t stempsBuffer = 0;
	t_list *temps = list_create();
	for (; countTemps; countTemps--) {
		desserializeTempToList(temps, tempsBuffer, &stempsBuffer);
	}

//Test TODO: Adaptar a las estructuras de Job
	printf("\n%s\n", nodeIP);
	printf("%d\n", nodePort);
	printf("%s\n", tempResultName);
	printf("Count Temps:%d\n", list_size(temps));
	void showTemp(t_temp *temp) {
		printf("-Temp-\n");
		printf("\t%d\n", temp->originMap);
		printf("\t%s\n", temp->nodeIP);
		printf("\t%d\n", temp->nodePort);
		printf("\t%s\n", temp->tempName);
	}
	list_iterate(temps, (void *) showTemp);
//End
}

void serializeReduceResult(int fd, bool result, uint16_t idResult, char failedTemp[60]) {
	char resultFrom = 'r';
	size_t sresultFrom = sizeof(char);
	size_t sresult = sizeof(result);
	size_t sidResult = sizeof(uint16_t);
	//Mando el nombre del temporal que fallo para poder identificar del otro lado de donde vino el error
	size_t sbuffer = sresultFrom + sresult + sidResult + sizeof(char) * 60;

	idResult = htons(idResult);

	void *buffer = malloc(sbuffer);
	buffer = memset(buffer, '0', sbuffer);
	memcpy(buffer, &resultFrom, sresultFrom);
	memcpy(buffer + sresultFrom, &result, sresult);
	memcpy(buffer + sresultFrom + sresult, &idResult, sidResult);
	memcpy(buffer + sresultFrom + sresult + sidResult, failedTemp, sizeof(char) * 60);
	socket_send_packet(fd, buffer, sbuffer);
}
