/*
 * sock_servidor.c
 *
 *  Created on: 26/4/2015
 *      Author: utnso
 */
#include <stdio.h>
#include <sys/types.h>
#include <sys/socket.h>
#include <netinet/in.h>

#define PORT 5000 /* El puerto que será abierto para atender a los sock_clientees */
#define BACKLOG 2 /* El número de conexiones permitidas */
#define MAXDATASIZE 100 //Cantidad maxima de datos que puedo mandar de un socket a otro

main()
{

   int sockfd, sockfd2; // son los ficheros descriptores, lo vamos a ver mas adelante

   struct sockaddr_in sock_servidor; //estos guardan la info del servidor IP , PUERTO, ETC

   struct sockaddr_in sock_cliente;  //idem sock_clientee

   int sin_size;

   int error;

   //Llamamos a la funcion socket para obtener el nuevo socket (nos devuelve un ID que se guarda en sockfd)
   //AF_INET es la familia de direcciones , es como obtiene el sistema y como maneja las ip y esas cosas
   //SOCK_STREAM --> hay dos tipos de socket los STREAM y los DATAGRAM , pero usamos STREAM

   if ((sockfd=socket(AF_INET, SOCK_STREAM, 0)) == -1 ) {
      printf("Error en la funcion socket()\n");
      exit(-1);
   }

   sock_servidor.sin_family = AF_INET;  //sin_family--> se refiere a como se obtiene el IP (por default AF_INET)

   sock_servidor.sin_port = htons(PORT); //se utiliza la funcion htons para poder convertir el PUERTO

   sock_servidor.sin_addr.s_addr = INADDR_ANY; // INADDR_ANY pone nuestra dirección IP automáticamente

   memset(&(sock_servidor.sin_zero),0,8); // escribimos ceros en el reto de la estructura */


   //Bindeamos el socket a la IP y el PUERTO definidos anteriormente
   if(bind(sockfd,(struct sockaddr*)&sock_servidor, sizeof(struct sockaddr))==-1) {
      printf("Error en la funcion bind() \n");
      exit(-1);
   }

   //Hacemos el listen y nos ponemos a escuchar conexiones nuevas
   //BACKLOG --> era la cantidad de conexiones permitidas que lleguen a la vez
   if(listen(sockfd,BACKLOG) == -1) {  /* llamada a listen() */
      printf("Error en la funcion listen()\n");
      exit(-1);
   }

   //HACEMOS UN WHILE TRUE Y ACEPTAMOS LAS CONEXIONES QUE LLEGUEN
   while(1) {
      sin_size=sizeof(struct sockaddr_in);

      //Aca aceptamos la conexion y la "guardamos" en el sockfd2
      if ((sockfd2 = accept(sockfd,(struct sockaddr *)&sock_cliente, &sin_size))==-1) {
         printf("error en accept()\n");
         exit(-1);
      }

      //INET_NTOA --> es una funcion que convierte direcciones, hay varias de estas
      //			  despues se las paso pero generalmente se usa esta (estan en la Beej igual)

      printf("Se obtuvo una conexión desde %s\n", inet_ntoa(sock_cliente.sin_addr) );

      char mensaje[MAXDATASIZE] = "Bienvenido";

      if((error=send(sockfd2,mensaje,sizeof(mensaje),0))<=0)
		{
			printf("Error en el send()");
			close(sockfd2);
			return -1;
		}

      close(sockfd2); /* cierra sockfd2 */
   }
}


