/*
 * servidormulthilo.c
 *
 *  Created on: 26/4/2015
 *      Author: utnso
 */

#include <stdio.h>
#include <sys/types.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <stdlib.h>
#include <arpa/inet.h>
#include <unistd.h> //es para los fd_set

#define PUERTO 5000 /* El puerto que será abierto para atender a los sock_clientees */
#define BACKLOG 2 /* El número de conexiones permitidas */
#define MAXDATASIZE 100 //Cantidad maxima de datos que puedo mandar de un socket a otro

main()
{

   int sock_escucha = 0; // --> lo vamos a mantener todo el tiempo escuchando nuevas conexiones
   int new_socket = 0;   // --> es el que va a aceptar la nueva conexion
   int exit = 1;  // --> lo vamos a utilizar para salir del sistema
   int i,fdmax;  // --> los utilizamos para recorrer todos los sockets

   //Los fd_set guardan informacion de los files descriptores (sock_escucha y new_socket
   //asi podemos tener un control de cuantos clientes hay conectados y quienes son cada uno
   fd_set read_fds;
   fd_set master;

   //ESCUCHAR--> Esta definida abajo, basicamente lo dividi solo hace el Bind y Listen para
   //			 el sock escucha
   sock_escucha=escuchar();

   if(sock_escucha == -1)
   	{
   		printf("ERROR, error iniciando sockets. \n");
   		printf("ERROR, sock_escucha = %d \n", sock_escucha);
   		return -1;
   	}

   //Estas funciones estan mejor explicadas en la Guia Beej, pero basicamente
   //FD_ZERO --> limpia toda la "lista" de las conexiones que le pases los fd_set
   //FD_SET --> añade un file descriptor al fd_set que le pases
   //fdmax --> es una variable que la usamos para recorrer todos los sockets y saber cual es el max
   FD_ZERO (&master);
   FD_ZERO (&read_fds);
   FD_SET (sock_escucha, &master);
   fdmax=sock_escucha;

   //Bucle infitio para seguir aceptando conexiones
   while(exit==1)
   	{
	   //READ_FDS --> lee un file descriptor de la lista de fd_set
   		read_fds = master;
   		if (select(fdmax + 1, &read_fds, NULL, NULL, NULL) == -1)
   		{
   			printf("Error en funcion select");
   			return -1;
   		}
   		//Bucle por todos los sockets
   		for (i = 0; i <= fdmax; i++)
   		{
   			//pregunto si en algun socket hubo algun cambio
   			if (FD_ISSET(i, &read_fds))
   			{
   				if (i == sock_escucha)
   				{
   					//si es la consola es porque el listen recibio una nueva conexion
   					new_socket = escuchar_puerto(sock_escucha);
   					if(new_socket == -1)
   					{
   						//si es -1 fallo asi que continuo descartando esta
   						printf("No se pudo agregar una nueva conexion");
   						continue;
   					}
   					//lo agrego para que lo escuchen en el select
   					FD_SET(new_socket, &master);
   					//si es el mayor lo re asigno
   					if(fdmax < new_socket)
   						fdmax = new_socket;

   					continue;
   				}

   			}
   		}
   	}
}


int escuchar()
{
	//llega a hacer el listen
	int sock_escucha;
	int yes=1;
	struct sockaddr_in my_addr;

	if( (sock_escucha=socket(AF_INET,SOCK_STREAM,0))==-1)
	{
		printf( "Error en funcion socket en escuchar");
		return -1;
	}
	//este nose si va
	if(setsockopt(sock_escucha,SOL_SOCKET,SO_REUSEADDR, &yes,sizeof(int))==-1)
	{
		printf( "Error en funcion setsockopt en escuchar");
		return -1;

	}
	my_addr.sin_port=htons(PUERTO);
	my_addr.sin_family=AF_INET;
	my_addr.sin_addr.s_addr=INADDR_ANY;
	memset(&(my_addr.sin_zero),0,8);

	if (bind(sock_escucha,(struct sockaddr *)&my_addr,sizeof(struct sockaddr))==-1)
	{
		printf( "Error en funcion bind en escuchar");
		return -1;
	}

	if (listen(sock_escucha,BACKLOG)==-1)
	{
		printf( "Error en funcion listen en escuchar");
		return -1;
	}

	return sock_escucha;
}

int escuchar_puerto(int sock_escucha)
{
	socklen_t sin_size;
	struct sockaddr_in my_addr;
	struct sockaddr_in their_addr;

	int new_socket;
	int numbytes;

	my_addr.sin_port=htons(PUERTO);
	my_addr.sin_family=AF_INET;
	my_addr.sin_addr.s_addr=INADDR_ANY;
	memset(&(my_addr.sin_zero),0,8);

	sin_size=sizeof(struct sockaddr_in);

	if((new_socket=accept(sock_escucha,(struct sockaddr *)&their_addr,	&sin_size))==-1)
	{
		printf( "Error en funcion accept en escuchar puerto");
		return -1;
	}

	printf("Se obtuvo una nueva conexion");

	char mensaje[MAXDATASIZE] = "Bienvenido";
	 if((numbytes=send(new_socket,mensaje,sizeof(mensaje),0))<=0)
			{
				printf("Error en el send()");
				close(new_socket);
				return -1;
			}

	return new_socket;
}


