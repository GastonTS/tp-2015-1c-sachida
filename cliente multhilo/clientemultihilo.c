/*
 * clientemultihilo.c
 *
 *  Created on: 26/4/2015
 *      Author: utnso
 */

#include <stdio.h>
#include <sys/types.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <netdb.h> 		//ES NECESARIO PARA HOSTENT

#define PUERTO 5000
#define MAXDATASIZE 100  //NUMERO MAXIMO DE BYTES

int main()
{
	int sockfd, numbytes; //descriptores

	//Para enviar y recibir datos creamos un buffer o paquete donde se almacenan
	char buf[MAXDATASIZE];

	struct hostent *he;

	struct sockaddr_in servidor;


	if((sockfd = socket(AF_INET,SOCK_STREAM,0))==-1){
		printf("Error en crear el socket");
		exit(-1);
	}

	servidor.sin_family = AF_INET;
	servidor.sin_port = htons(PUERTO);
	servidor.sin_addr.s_addr = INADDR_ANY;
	memset(&(servidor.sin_zero),0,8);

	if(connect(sockfd,(struct sockaddr *)&servidor,sizeof(struct sockaddr))==-1){
		printf("error en el connect");
		exit(-1);
	}

	if((numbytes=recv(sockfd, buf, MAXDATASIZE,0))==-1){
		printf("Error en el recv");
		exit(-1);
	}

	buf[numbytes]='\0';

	printf("%s\n",buf);
	printf("Recibi mensaje del servidor");

	while(1){
		sleep(50);
	}
	close(sockfd);
}



