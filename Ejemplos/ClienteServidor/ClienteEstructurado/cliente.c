/*
 * cliente.c
 *
 *  Created on: 12/5/2015
 *      Author: utnso
 */


#include "cliente.h"


int main(int argc, char *argv[])
{
	int sockfd, numbytes; //descriptores
	char* buffer;
	//Para enviar y recibir datos creamos un buffer o paquete donde se almacenan
	char buf[MAXDATASIZE];

	struct hostent *he;

	struct sockaddr_in servidor;

	t_mensaje mensaje;  //Estructura para intercambiar mensajes (PROTOCOLO)

	if(argc != 2)
	   		{
	   			printf("ERROR, la sintaxis del servidor es: ./servidor.c archivo_configuracion \n");
	   			return -1;
	   		}
	//Creo el logger parametros -->
	//Nombre del archivo de log
	//nombre del programa que crea el log
	//se muestra el log por pantalla?
	//nivel minimo de log
	logger = log_create("Log.txt", "Cliente",false, LOG_LEVEL_DEBUG);

	//Llamo a la funcion que esta abajo de todo que saca los datos del archivo de config
	getInfoConf(argv[1]);

	//Creo el buffer
	if((buffer = (char*) malloc (sizeof(char) * MAXDATASIZE)) == NULL)
		{
			log_error(logger, "Error al reservar memoria para el buffer en conectar_msp");
			return -1;
		}

	if((sockfd = socket(AF_INET,SOCK_STREAM,0))==-1){
		printf("Error en crear el socket");
		exit(-1);
	}

	servidor.sin_family = AF_INET;
	servidor.sin_port = htons(puerto_servidor);
	servidor.sin_addr.s_addr = inet_addr(ip_servidor);
	memset(&(servidor.sin_zero),0,8);

	if(connect(sockfd,(struct sockaddr *)&servidor,sizeof(struct sockaddr))==-1){
		printf("error en el connect");
		exit(-1);
	}

	//Log_info crea un registro en el log con la leyenda "INFO"
	//Log_error crea un registro en el log con la leyenda "ERROR"
	log_info(logger,"Se conecto al servidor correctamente");

	if((numbytes=recv(sockfd, buffer, SIZE_MSG,0))==-1){
		printf("Error en el recv");
		exit(-1);
	}

	//copio la respuesta del buffer
	memcpy(&mensaje,buffer,SIZE_MSG);

	if((mensaje.id_proceso  == SERVIDOR)&&(mensaje.tipo=HANDSHAKE))
	{
		log_info(logger, "Conexion Lograda con el servidor");
	}
	else
	{
		log_error(logger, "No recibi Handshake");
		exit(-1);
	}


	memset(buffer,'\0',MAXDATASIZE);

	log_info(logger, "Recibi mensaje del servidor");
	log_info(logger, "El emisor es: %d" , mensaje.id_proceso);
	log_info(logger, "El mensaje es: %s", mensaje.mensaje);

	mensaje.tipo = HANDSHAKEOK;
	mensaje.id_proceso = CLIENTE;
	memcpy(buffer,&mensaje,SIZE_MSG);

	 if((numbytes=send(sockfd,buffer,SIZE_MSG,0))<=0)
			{
				printf("Error en el send()");
				return -1;
			}

	log_info(logger, "Envie el mensaje de confirmacion");

	while(1){
		sleep(50);
	}
	close(sockfd);
}

void getInfoConf(char* conf)
{
	t_config* config; //creamos la variable que va a ser el archivo de config

	config = config_create(conf); //creamos el "objeto" archivo de config

	//OBTENEMOS LOS VALORES DEL ARCHIVO DE CONFIG
	//ip_servidor = config_get_int_value(config,"IP_SERVIDOR");
	//TUVE QUE CAMBIAR LA FUNCION DE ARRIBA
	//ip_servidor = config_get.... porque en realidad la direccion tiene 14 digitos cuando se convierte o algo asi
	//entonces la meti en un string[14] y con la funcion strcpy lo meto dentro del string
	strcpy(ip_servidor,config_get_string_value(config, "IP_SERVIDOR"));
	puerto_servidor = config_get_int_value(config, "PUERTO_SERVIDOR");

	puts("Extraccion correcta del archivo de configuracion");

	config_destroy(config); //destruimos el "objeto" archivo de config
}

