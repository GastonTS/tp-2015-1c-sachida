/*
 * Job.c
 *
 *  Created on: 4/6/2015
 *      Author: utnso
 */
/*

LISTO 1--> Iniciar y extraer datos del archivo de configuracion
LISTO 2--> Conectarse a MARTA
Casi 3--> Mandarle a MARTA el/los archivo/s sobre el/los cual/es se desea trabajar.
LISTO 4--> Quedar a la espera de ips y puertos de los nodos al cual me tengo que conectar para aplicar dichas rutinas
	NOTA:
		Cada rutina es un hilo nuevo, si es Map entonces es hilo mapper si es Reduce entonces hilo reduce
Casi--> Conectarme al nodo X que me paso antes MARTA y mandarle la rutina que deseo correr "map.py" , el bloque
	y el archivo temporal donde debe ser almacenado
Casi--> Esperar la finalizacion de la rutina y la confirmacion por parte del Nodo X
Casi--> Notificar a MARTA de la ejecucion



*/


/* TODO
	VER COMO EXTRAER DEL ARCHIVO DE CONFIG LA LISTA DE ARCHIVOS
*/

#include "Job.h"

int main(int argc, char *argv[]){

	logger = log_create("Log.txt","JOB",false,LOG_LEVEL_DEBUG);

	list_mappers = list_create();
	list_reducers = list_create();

	if(argc != 2){
		printf("ERROR -> La sintaxis es:  ./Job.c \"Ruta_archivo_config\" \n");
		return(-1);
	}

	leerArchivoConfig(argv[1]);

	if((sock_marta = conectarMarta()) == -1){
		log_error(logger,"Error al conectarme con MaRTA. Aborta la ejecucion");
		return(-1);
	};

	atenderMarta(sock_marta);



	return 1;
}

void leerArchivoConfig(char* conf){
	t_config* config;

	config = config_create(conf);

	strcpy(IP_MARTA,config_get_string_value(config,"IP_MARTA"));
	PUERTO_MARTA = config_get_int_value(config,"PUERTO_MARTA");

	//char* MAPPER = malloc(LISTA_ARCHIVOS);
	//strcpy(MAPPER,config_get_string_value(config,"MAPPER"));
	MAPPER = config_get_string_value(config,"MAPPER");
	convertirListaArch(MAPPER);

	REDUCER = config_get_string_value(config,"REDUCER");
	ARCH_RESULTADO = config_get_string_value(config,"RESULTADO");
	strcpy(COMBINER,config_get_string_value(config,"COMBINER"));

	log_info(logger,"Extraccion correcta del archivo de configuracion");
	config_destroy(config);
}


void convertirListaArch(char* cadena,t_list* list_archivos){
	char* caracter;
	int indice,longCadenaNueva,longCadena,count;
	char* cadenaNueva;

	while((caracter = strchr(cadena, ' '))){
		indice = (int)(caracter - cadena);
		longCadena = strlen(cadena);

		longCadenaNueva = longCadena-indice;
		cadenaNueva = malloc(indice+1);
		memset(cadenaNueva,'\0',indice+1);
		memcpy(cadenaNueva,cadena,indice);
		list_add(list_archivos,cadenaNueva);
		//log_info(logger,"CadenaNueva: %s", cadenaNueva);
		//realloc(cadena,i+1);
		//memset(cadena,'\0',indice+1);
		memcpy(cadena,cadena+indice+1,longCadenaNueva+1);
		//log_info(logger,"nueva longitud: %d",longCadenaNueva);
		//memset(cadenaNueva,'\0',i+1);
		//memcpy(cadenaNueva,cadena+indice,i);
		//log_info(logger,"String : %s",cadena);
	}

	list_add(list_archivos,cadena);

	free(cadenaNueva);
	free(cadena);
	return;
	/*
	cadenaNueva = malloc(indice+1);
	memset(cadenaNueva,'\0',indice+1);
	memcpy(cadenaNueva,cadena, indice);

	list_add(list_archivos,cadenaNueva);
	for(i=0;i<list_size(list_archivos);i++){
		cadenita = list_get(list_archivos,i);
		log_info(logger,"lista posicion %d es %s",i,cadenita);
	}
	log_info(logger, "cadena Nueva: %s",cadenaNueva);
	*/
}


int conectarMarta(){
	int sockfd, numbytes;
	struct sockaddr_in marta;
	char* buffer;
	t_mensaje mensaje;

	if((buffer = (char*) malloc (sizeof(char) * MAXDATASIZE)) == NULL)
	{
		printf("Error al reservar memoria para el buffer en conectar con MaRTA \n");
		return(-1);
	}

	if((sockfd = socket(AF_INET,SOCK_STREAM,0))==-1){
		printf("Error en crear el socket del MaRTA \n");
		close(sockfd);
		return(-1);
	}

	marta.sin_family = AF_INET;
	marta.sin_port = htons(PUERTO_MARTA);
	marta.sin_addr.s_addr = inet_addr(IP_MARTA);
	memset(&(marta.sin_zero),0,8);

	if(connect(sockfd,(struct sockaddr *)&marta,sizeof(struct sockaddr))==-1){
		printf("Error en el connect con MaRTA \n");
		close(sockfd);
		return(-1);
	}

	memset(buffer,'\0',MAXDATASIZE);

	mensaje.tipo = HANDSHAKE;
	mensaje.id_proceso = JOB;
	memcpy(buffer,&mensaje,SIZE_MSG);

	if((numbytes=send(sockfd,buffer,SIZE_MSG,0))<=0)
	{
		printf("Error en el send handshake a MaRTA \n");
		return(-1);
	}

	memset(buffer,'\0',MAXDATASIZE);

	if((numbytes=recv(sockfd, buffer, SIZE_MSG,0))==-1){
		printf("Error en el recv handshakeok de MaRTA \n");
		return(-1);
	}

	//Copio el mensaje que recibi en el buffer
	memcpy(&mensaje,buffer,SIZE_MSG);

	if((mensaje.id_proceso  == MARTA)&&(mensaje.tipo=HANDSHAKEOK))
	{
		log_info(logger, "Conexion Lograda con MaRTA ");
	}
	else
	{
		printf("No recibi Handshake de MaRTA, no se pudo conectar \n");
		return(-1);
	}

	memset(buffer,'\0',MAXDATASIZE);

	mensaje.tipo = ARCHIVOS;
	mensaje.id_proceso = JOB;
	//TODO CUANDO ARREGLE LO DE LA LISTA DE ARCHIVOS HAY QUE MANDARLA ACA
	//ADEMAS HAY QUE MANDARLE SI ES COMBINER O NO
	memcpy(buffer,&mensaje,SIZE_MSG);

	if((numbytes=send(sockfd,buffer,SIZE_MSG,0))<=0)
	{
		printf("Error en el send archivos a MaRTA \n");
		free(buffer);
		return(-1);
	}

	free(buffer);
	return sockfd;
}


void atenderMarta(int socketMarta){
	int numbytes;
	char* buffer;
	t_mensaje mensaje;
	pthread_t hilo_mapper;
	pthread_t hilo_reduce;

	if((buffer = (char*) malloc (sizeof(char) * MAXDATASIZE)) == NULL)
	{
		printf("Error al reservar memoria para el buffer en atender MaRTA \n");
		exit(-1);
	}

	while(1){

		memset(buffer,'\0',MAXDATASIZE);
		if((numbytes=recv(socketMarta, buffer, SIZE_MSG,0))==-1){
			printf("Error en el recv MapReduce de MaRTA \n");
			exit(-1);
		}
		memcpy(&mensaje,buffer,SIZE_MSG);

		if((mensaje.id_proceso == MARTA)&&(mensaje.tipo == MAP))
		{
			//todo lo del mapper
			pthread_create(&hilo_mapper,NULL,(void*)atenderMapper,NULL);
			//list_add(list_mappers,);

		} else if((mensaje.id_proceso == MARTA)&&(mensaje.tipo == REDUCE))
		{
			//todo lo del reduce
			pthread_create(&hilo_reduce,NULL,(void*)atenderReducer,NULL);
			//list_add(list_reducers,);

		} else
		{
			printf("Error en el recv MapReduce de MaRTA --> Me esta mandando otra cosa \n");
			free(buffer);
			close(socketMarta);
			exit(-1);
		}
	}

}

void atenderMapper(){

	log_info(logger,"Cree hilo mapper");

	//CONECTARME AL NODO Y MANDARLE EL BLOQUE Y LOS DATOS DE MAP.PY
	int sockfd, numbytes;
	char* buffer;
	struct sockaddr_in nodo;
	int retorno = 0;
	t_mensaje mensaje;

	if((buffer = (char*) malloc (sizeof(char) * MAXDATASIZE)) == NULL)
	{
		printf("Error al reservar memoria para el buffer en conectar con NODO mapper\n");
		pthread_exit(&retorno);
	}

	if((sockfd = socket(AF_INET,SOCK_STREAM,0))==-1){
		printf("Error en crear el socket del NODO mapper\n");
		close(sockfd);
		pthread_exit(&retorno);
	}

	nodo.sin_family = AF_INET;
	//nodo.sin_port = htons(PUERTO_NODO);
	//nodo.sin_addr.s_addr = inet_addr(IP_NODO);
	memset(&(nodo.sin_zero),0,8);

	if(connect(sockfd,(struct sockaddr *)&nodo,sizeof(struct sockaddr))==-1){
		printf("Error en el connect con NODO mapper \n");
		close(sockfd);
		pthread_exit(&retorno);
	}

	memset(buffer,'\0',MAXDATASIZE);

	mensaje.tipo = RUTINAMAP;
	mensaje.id_proceso = JOB;
	memcpy(buffer,&mensaje,SIZE_MSG);

	if((numbytes=send(sockfd,buffer,SIZE_MSG,0))<=0)
	{
		printf("Error en el send RUTINAMAP a NODO mapper \n");
		pthread_exit(&retorno);
	}

	memset(buffer,'\0',MAXDATASIZE);

	mensaje.tipo = BLOQUE;
	mensaje.id_proceso = JOB;
	//mensaje.datosNumericos = NUMERO_BLOQUE;
	memcpy(buffer,&mensaje,SIZE_MSG);

	if((numbytes=send(sockfd,buffer,SIZE_MSG,0))<=0)
	{
		printf("Error en el send BLOQUE a NODO mapper \n");
		pthread_exit(&retorno);
	}

	memset(buffer,'\0',MAXDATASIZE);

	if((numbytes=recv(sockfd, buffer, SIZE_MSG,0))==-1){
		printf("Error en el recv confirmacion map de NODO \n");
		pthread_exit(&retorno);
	}

	//Copio el mensaje que recibi en el buffer
	memcpy(&mensaje,buffer,SIZE_MSG);

	if((mensaje.id_proceso  == NODO)&&(mensaje.tipo==MAPOK))
	{
		log_info(logger, "Map terminado con exito ");
		confirmarMap();
	}
	else if((mensaje.id_proceso == NODO) && (mensaje.tipo == MAPERROR))
	{
		log_error(logger,"Map finalizado erroneamente");
		pthread_exit(&retorno);
	}else{
		printf("No recibi confirmacion del Nodo \n");
	}

}

void confirmarMap(){
	int numbytes;
	char* buffer;
	t_mensaje mensaje;
	int retorno = 0;

	if((buffer = (char*) malloc (sizeof(char) * MAXDATASIZE)) == NULL)
	{
		printf("Error al reservar memoria para el buffer en confirmarMap a MaRTA\n");
		pthread_exit(&retorno);
	}

	memset(buffer,'\0',MAXDATASIZE);

	mensaje.tipo = MAPOK;
	mensaje.id_proceso = JOB;

	memcpy(buffer,&mensaje,SIZE_MSG);

	if((numbytes=send(sock_marta,buffer,SIZE_MSG,0))<=0)
	{
		printf("Error en el send CONFIRMARMAP a MaRTA \n");
		free(buffer);
		pthread_exit(&retorno);
	}

	free(buffer);
	pthread_exit(&retorno);
}

void atenderReducer(){
	log_info(logger,"Cree hilo reducer");

	//CONECTARME AL NODO Y MANDARLE EL BLOQUE Y LOS DATOS DE REDUCE.PY
	int sockfd, numbytes;
	char* buffer;
	int retorno = 0;
	struct sockaddr_in nodo;
	t_mensaje mensaje;

	if((buffer = (char*) malloc (sizeof(char) * MAXDATASIZE)) == NULL)
	{
		printf("Error al reservar memoria para el buffer en conectar con NODO reduce \n");
		pthread_exit(&retorno);
	}

	if((sockfd = socket(AF_INET,SOCK_STREAM,0))==-1){
		printf("Error en crear el socket del NODO reduce\n");
		close(sockfd);
		pthread_exit(&retorno);
	}

	nodo.sin_family = AF_INET;
	//nodo.sin_port = htons(PUERTO_NODO);
	//nodo.sin_addr.s_addr = inet_addr(IP_NODO);
	memset(&(nodo.sin_zero),0,8);

	if(connect(sockfd,(struct sockaddr *)&nodo,sizeof(struct sockaddr))==-1){
		printf("Error en el connect con NODO reduce \n");
		close(sockfd);
		pthread_exit(&retorno);
	}

	memset(buffer,'\0',MAXDATASIZE);

	mensaje.tipo = RUTINAREDUCE;
	mensaje.id_proceso = JOB;
	memcpy(buffer,&mensaje,SIZE_MSG);

	if((numbytes=send(sockfd,buffer,SIZE_MSG,0))<=0)
	{
		printf("Error en el send RUTINAREDUCE a NODO \n");
		pthread_exit(&retorno);
	}

	memset(buffer,'\0',MAXDATASIZE);

	if((numbytes=recv(sockfd, buffer, SIZE_MSG,0))==-1){
		printf("Error en el recv confirmacion reduce de NODO \n");
		pthread_exit(&retorno);
	}

	//Copio el mensaje que recibi en el buffer
	memcpy(&mensaje,buffer,SIZE_MSG);

	if((mensaje.id_proceso  == NODO)&&(mensaje.tipo==REDUCEOK))
	{
		log_info(logger, "Reduce terminado con exito ");
		confirmarReduce();
	}
	else if((mensaje.id_proceso == NODO) && (mensaje.tipo == REDUCEERROR))
	{
		log_error(logger,"Reduce finalizado erroneamente");
		pthread_exit(&retorno);
	}else{
		printf("No recibi confirmacion del Nodo \n");
	}
}

void confirmarReduce(){
	int numbytes;
	char* buffer;
	t_mensaje mensaje;
	int retorno = 0;

	if((buffer = (char*) malloc (sizeof(char) * MAXDATASIZE)) == NULL)
	{
		printf("Error al reservar memoria para el buffer en confirmarReduce a MaRTA\n");
		pthread_exit(&retorno);
	}

	memset(buffer,'\0',MAXDATASIZE);

	mensaje.tipo = REDUCEOK;
	mensaje.id_proceso = JOB;

	memcpy(buffer,&mensaje,SIZE_MSG);

	if((numbytes=send(sock_marta,buffer,SIZE_MSG,0))<=0)
	{
		printf("Error en el send CONFIRMARREDUCE a MaRTA \n");
		free(buffer);
		pthread_exit(&retorno);
	}

	free(buffer);
	pthread_exit(&retorno);
}

