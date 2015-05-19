/*
 * servidor.c
 *
 *  Created on: 12/5/2015
 *      Author: utnso
 */

#include "servidor.h"

//AHORA LE PASAMOS COMO PARAMETRO DE LA FUNCION MAIN LA RUTA DEL ARCHIVO DE CONFIG
int main(int argc, char *argv[])
{

   int sock_escucha = 0; // --> lo vamos a mantener todo el tiempo escuchando nuevas conexiones
   int new_socket = 0;   // --> es el que va a aceptar la nueva conexion
   int exit = 1;  // --> lo vamos a utilizar para salir del sistema
   int i,fdmax;  // --> los utilizamos para recorrer todos los sockets
   //Los fd_set guardan informacion de los files descriptores (sock_escucha y new_socket
   //asi podemos tener un control de cuantos clientes hay conectados y quienes son cada uno
   fd_set read_fds;
   fd_set master;

   //Preguntamos si los argumentos son distintos a 2 quiere decir que no se paso
   //por parametro el archivo de config
   if(argc != 2)
   		{
   			printf("ERROR, la sintaxis del servidor es: ./servidor.c archivo_configuracion \n");
   			return -1;
   		}

   getInfoConf(argv[1]);

   logger = log_create("Log.txt", "Cliente",false, LOG_LEVEL_DEBUG);
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
	my_addr.sin_port=htons(puerto_servidor);
	my_addr.sin_family=AF_INET;
	my_addr.sin_addr.s_addr=inet_addr(ip_servidor);
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
	char* buffer; //Variable para intercambiar informacion entre sockets
	socklen_t sin_size;
	struct sockaddr_in my_addr;
	struct sockaddr_in their_addr;

	int new_socket;
	int numbytes;

	t_mensaje mensaje;  //Estructura para intercambiar mensajes (PROTOCOLO)

	//Creo el buffer
	if((buffer = (char*) malloc (sizeof(char) * MAXDATASIZE)) == NULL)
		{
			printf("Error al reservar memoria para el buffer en conectar_msp");
			return -1;
		}

	my_addr.sin_port=htons(puerto_servidor);
	my_addr.sin_family=AF_INET;
	my_addr.sin_addr.s_addr=inet_addr(ip_servidor);
	memset(&(my_addr.sin_zero),0,8);

	sin_size=sizeof(struct sockaddr_in);

	if((new_socket=accept(sock_escucha,(struct sockaddr *)&their_addr,	&sin_size))==-1)
	{
		printf( "Error en funcion accept en escuchar puerto");
		return -1;
	}

	printf("Se obtuvo una nueva conexion");

	mensaje.tipo = HANDSHAKE;
	mensaje.id_proceso = SERVIDOR;
	strncpy(mensaje.mensaje, "HOLA", sizeof("HOLA"));
	memcpy(buffer,&mensaje,SIZE_MSG);

	 if((numbytes=send(new_socket,buffer,SIZE_MSG,0))<=0)
			{
				printf("Error en el send()");
				close(new_socket);
				return -1;
			}

	//pongo en 0 el buffer para recibir
	 memset(buffer,'\0',MAXDATASIZE);

	 //recibo en el buffer la respuesta al handshake
	 if((numbytes=recv(new_socket,buffer,SIZE_MSG,0))<=0)
	 	 {
	 		log_error(logger, "Error en el read en el socket con el cliente");
	 		close(new_socket);
	 		return -1;
	 	}

	 	//copio la respuesta del buffer
	 	memcpy(&mensaje,buffer,SIZE_MSG);

	 	if((mensaje.id_proceso  == CLIENTE)&&(mensaje.tipo=HANDSHAKEOK))
	 	{
	 		log_info(logger,"Conexion Lograda con el cliente");
	 		log_info(logger,"Mensaje del cliente: %d", mensaje.id_proceso);
	 		log_info(logger,"El mensaje es: %s", mensaje.mensaje);
	 		free(buffer);
	 		return new_socket;
	 	}
	 	else
	 	{
	 		log_error(logger,"No recibi Handshake OK");
	 	}

	 	free(buffer);
	 	return -1;
}

//METODO PARA OBTENER LOS PARAMETROS DEL ARCHIVO DE CONFIG
void getInfoConf(char* conf)
{
	t_config* config; //creamos la variable que va a ser el archivo de config

	config = config_create(conf); //creamos el "objeto" archivo de config

	//OBTENEMOS LOS VALORES DEL ARCHIVO DE CONFIG
	//ip_servidor = config_get_int_value(config,"IP_SERVIDOR");
	strcpy(ip_servidor,config_get_string_value(config, "IP_SERVIDOR"));
	puerto_servidor = config_get_int_value(config, "PUERTO_SERVIDOR");

	printf("Extraccion correcta del archivo de configuracion \n");

	config_destroy(config); //destruimos el "objeto" archivo de config
}


