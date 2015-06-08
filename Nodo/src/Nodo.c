#include "Nodo.h"
#include "../../utils/socket.h"

void createNode();
//void getFileContent();
//void nodeMap(rutinaMap, int nroBloque);
//void nodeReduce(int[string nameNode, int nroBloque], rutinaReduce, char nombreDondeGuarda);
int size_of(int fd);
int fileSize;
int conectarFileSystem();
int socket_fileSystem;

//Le agregue los argumentos para que se pueda pasar el archivo de conf como parametro del main
int main(int argc, char *argv[]) {
	getBloque(5);
	//pthread_t conexionesJob;
	//pthread_t conexionesNodo;
	if(argc != 2)
		{
			printf("ERROR, la sintaxis del servidor es: ./Nodo.c archivo_configuracion \n");
			return -1;
		}
	getInfoConf(argv[1]);
	/*Creo el logger parametros -->
	Nombre del archivo de log
	nombre del programa que crea el log
	se muestra el log por pantalla?
	nivel minimo de log */

	logger = log_create("Log.txt", "Node",1, log_level_from_string("DEBUG"));
	socket_fileSystem =	conectarFileSystem();
	//todo Ver bien si es necesaria esta funcion
	createNode(); //creo que no es necesario el createNodo.

	/*[8/6/2015, 14:29] Santo: char *buffer;
			socket_recv_string(socket, &buffer);
			//Copias el buffer en el .bin haciendo 20MB*nrobloque
			[8/6/2015, 14:30] Santo: despues rellenas si tenes que rellenar con/0 o eso

			[8/6/2015, 14:31] Santo: char *buffer;
			socket_recv_string(socket, &buffer);
			setBloque(nroBloque, buffer);
			[8/6/2015, 14:31] Santo: y setBloque lo que hace en realidad es copiar el buffer en el archivo
			*/

	/*socket_recv_paquete(socket, &paquete);
			int comando = copiarPrimeros4Bytes(paquete)
			switch(comando){
 	 	 	case 1:
      	  		int numBlock= copiarSegundos4Bytes(paquete)
      	  		int ssize = copiarTerceros4Bytes(paquete)
      	  	char * buffer = copiarlossiguientes<ssize>(paquete)
      	    setBloque(numBlock, buffer);
			break;
			case 2:
     	 		//alguna otra ordenq ue te pueda pedir el fs (supongo que get bloque por ej)
			default: error mensaje desconocido;*/

	//TODO ACA DEBERIAMOS HACER EL WHILE INFINITO ESPERANDO CONEXIONES Y PETICIONES
	/*TODO QUILOMBO ....
		   HAY QUE ABRIR UN THREAD PARA ESCUCHAR JOBS Y UNO PARA ESCUCHAR NODOS
		   PORQUE?
		   PORQUE DICE QUE TIENEN QUE PUEDEN CORRER EN PARALELO :)


	//ptrhead_create(&conexionesJob,NULL,(void*)escucharJobs,NULL);
	//pthread_create(&conexionesNodo,NULL,(void*)escucharNodos,NULL);


	/*TODO TODAS LAS FUNCIONES GETBLOQUE Y ESAS VAN ADENTRO DE LOS TRHEADS */
	return EXIT_SUCCESS;
}


// Almacenar los datos del FS y hacer Map y Reduce segun lo requerido por los Jobs
 void createNode() {
	//TODO DANI NO ENTIENDO QUE ES ESTE PARAMETRO
	fileSize = size_of(archivo_bin);
	printf(fileSize);
	 /*Funcion de la biblioteca lisen. para esperar al FS
	 stat se consigue el tamaño de la rchivo
	truncate -s 1G miarchivo.bin*/

	 //VAN LOS SOCKETS
	//Primero se conecta al filesystem


	//TODO SI ME PUDE CONECTAR AL FILESYSTEM ENTONCES CREAR EL ARCHIVO DE DATOS Y EL TMP

//tamanioArchivoDatos = size_of(archivo_bin);
	//TODO ENGLOBAR LO DE ARRIBA EN UNA VARIABLE
	//TODO QUEDAR A LA ESPERA DEL FILESYSTEM, NODOS, O JOBS PARA REALIZAR DISTINTAS TAREAS
}

/*void escucharJobs(){
	fd_set read_fds;
	fd_set master;
	int fdmax;
	int i;
	int sock_escucha, new_socket;


	sock_escucha = escuchar();

	if(sock_escucha == -1 )
	{
		printf("ERROR, error iniciando sockets. \n");
		printf("ERROR, sock_escucha = %d \n", sock_escucha);
		exit(-1) ;
	}

	while(exit==1)
		{
			read_fds = master;
			if (select(fdmax + 1, &read_fds, NULL, NULL, NULL) == -1)
			{
				log_error(logger, "Error en la funcion select de escucharJobs");
				exit(-1);
			}
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
							log_error(logger,"No se pudo agregar una nueva conexion");
							continue;
						}
						//lo agrego para que lo escuchen en el select
						FD_SET(new_socket, &master);
						//si es el mayor lo re asigno
						if(fdmax < new_socket)
							fdmax = new_socket;

						continue;
					}

					if(EstaConectadaCPU(i) != 0)
					{
						//si es distinto de 0 la cpu me esta hablando
						log_debug(logger,"Se detecto actividad en un CPU");

						//escucho la cpu que me habla
						if(atender_cpu(i)==-1)
						{
							log_debug(logger,"Se cayo una CPU");
							close(i);
							FD_CLR(i, &master);
							//hago un metodo que saque la cpu de la lista de cpus
							cpu_remove(i);
							//TODO bajo el semaforo ya que hay una cpu menos
							//sem_wait(&sem_cpu_list);
							continue;
						}
						continue;
					}
					//si i no pertenece a ninguno lo saco
					//close(i);
					//FD_CLR(i, &master);
				}
			}
		}
		log_destroy(logger);
		return;
}*/

/*void escucharNodos(){}*/

/*int escuchar()
{
	//llega a hacer el listen
	int sock_escucha;
	int yes=1;
	struct sockaddr_in my_addr;

	if( (sock_escucha=socket(AF_INET,SOCK_STREAM,0))==-1)
	{
		log_error(logger, "Error en funcion socket en escuchar");
		return -1;
	}
	//este nose si va
	if(setsockopt(sock_escucha,SOL_SOCKET,SO_REUSEADDR, &yes,sizeof(int))==-1)
	{
		log_error(logger, "Error en funcion setsockopt en escuchar");
		return -1;

	}
	my_addr.sin_port=htons(puerto_nodo);
	my_addr.sin_family=AF_INET;
	my_addr.sin_addr.s_addr=ip_nodo;
	memset(&(my_addr.sin_zero),0,8);

	if (bind(sock_escucha,(struct sockaddr *)&my_addr,sizeof(struct sockaddr))==-1)
	{
		log_error(logger, "Error en funcion bind en escuchar");
		return -1;
	}

	if (listen(sock_escucha,BACKLOG)==-1)
	{
		log_error(logger, "Error en funcion listen en escuchar");
		return -1;
	}

	return sock_escucha;
}*/

/*int escuchar_puerto(int sock_escucha)
{
	socklen_t sin_size;
	struct sockaddr_in my_addr;
	struct sockaddr_in their_addr;
	t_mensaje mensaje;

	int size_mensaje = sizeof(t_mensaje);
	char* buffer;

	int new_socket;
	int numbytes;

	if((buffer = (char*) malloc (sizeof(char) * MAXDATASIZE)) == NULL)
	{
		log_error(logger,"Error al reservar memoria para el buffer de escuchar puerto");
		return -1;
	}

	my_addr.sin_port=htons(puerto_nodo);
	my_addr.sin_family=AF_INET;
	my_addr.sin_addr.s_addr=ip_nodo;
	memset(&(my_addr.sin_zero),0,8);

	sin_size=sizeof(struct sockaddr_in);

	if((new_socket=accept(sock_escucha,(struct sockaddr *)&their_addr,	&sin_size))==-1)
	{
		log_error(logger, "Error en funcion accept en escuchar puerto");
		return -1;
	}

	memset(buffer,'\0',MAXDATASIZE);

	if((numbytes=read(new_socket,buffer,size_mensaje))<=0)
	{
		log_error(logger, "Error en el read en escuchar puerto");
		close(new_socket);
		return -1;
	}
	//recibo el handshake
	memcpy(&mensaje,buffer,size_mensaje);
	if(mensaje.tipo == HANDSHAKE && mensaje.id_proceso == JOB)
	{
		log_info(logger, "conexion lograda con JOB");
		//atenderJob(new_socket);
	}
	else
	{
		log_error(logger,"no identifique quien se conecto");
		return -1;
	}
	return new_socket;
}*/

int conectarFileSystem(){
	int descriptorFileSystem ;
	int handshakea;
	descriptorFileSystem =  socket_connect(ip_fs,puerto_fs);
	handshakea = socket_handshake_to_server(descriptorFileSystem,HANDSHAKE_NODO,HANDSHAKE_FILESYSTEM);
	printf("derror %d", handshakea);
	return descriptorFileSystem;
    /*
	fileSystem.sin_family = AF_INET;
	fileSystem.sin_port = htons(puerto_fs);
	fileSystem.sin_addr.s_addr = inet_addr(ip_fs);
	memset(&(fileSystem.sin_zero),0,8);

	if(connect(sockfd,(struct sockaddr *)&fileSystem,sizeof(struct sockaddr))==-1){
		printf("error en el connect al FileSystem");
		exit(-1);
	}

	log_info(logger,"Se conecto al FileSystem correctamente");

	if((numbytes=recv(sockfd, buffer, SIZE_MSG,0))==-1){
		printf("Error en el recv handshake del fileSystem");
		exit(-1);
	}

	//Copio el mensaje que recibi en el buffer
	memcpy(&mensaje,buffer,SIZE_MSG);

	if((mensaje.id_proceso  == FILESYSTEM)&&(mensaje.tipo=HANDSHAKE))
	{
		log_info(logger, "Conexion Lograda con el FileSystem");
	}
	else
	{
		log_error(logger, "No recibi Handshake del FileSystem");
		exit(-1);
	}

	memset(buffer,'\0',MAXDATASIZE);

	mensaje.tipo = HANDSHAKEOK;
	mensaje.id_proceso = NODO;
	memcpy(buffer,&mensaje,SIZE_MSG);

	if((numbytes=send(sockfd,buffer,SIZE_MSG,0))<=0)
	{
		printf("Error en el send handshakeok al FileSystem");
		exit(-1);
	}
	free(buffer);
	return sockfd;*/
}

char* getBloque(int nroBloque){
			int mapper;
			char* mapeo;
			int size;
			int pagesize;
			char* file_name = "./archivo_mmap.txt"; //Aca tiene que abrir el archivo que crea en el createNodo
			//Se abre el archivo para solo lectura

			mapper = fopen (file_name, O_RDONLY);
			pagesize = getpagesize();
			size = size_of(mapper);
			//size = 20;
			//Trate size bytes a partir de la posicion pagesize*(nroBloque-1)
			if( (mapeo = mmap( NULL, size, PROT_READ, MAP_SHARED, mapper, pagesize*(nroBloque-1) )) == MAP_FAILED){
				//Si no se pudo ejecutar el MMAP, imprimir el error y abortar;
				log_error(logger, "Error al ejecutar MMAP del archivo '%s' de tamaño: %d: %s\nfile_size",file_name,size);
				//fprintf(stderr, "Error al ejecutar MMAP del archivo '%s' de tamaño: %d: %s\nfile_size", file_name, size, strerror(errno));
				abort();
			}
			log_info("Tamaño del archivo: %d\nContenido:'%s'\n", size, mapeo);
			//printf ("Tamaño del archivo: %d\nContenido:'%s'\n", size, mapeo);

			//Se unmapea , y se cierrra el archivo
			munmap( mapeo, size );
			close(mapper);
			return mapeo;
		}

void setBloque(int nroBloque,char** string){
	char* lugarpaguardar;
	socket_recv_string(socket_fileSystem, &lugarpaguardar);
	//socket_recv_packet(int socket, void** packet, size_t* size);
	/*Recibe un buffer de datos,despues con el puntero que me devuelve el mmap modifico el archivo mapeado, primero busco puntero[ j ]=\0 y lo saco,
	 * relleno los espacios que falten hasta el nuevo bloque y remplazo el puntero[ j ]=datos[a] ,agrego el \0 y cierro el mmap.*/
}

/*void getFileContent(){
Devolverá   el   contenido   del   archivo   de   Espacio   Temporal solicitado.
  Se usara en el return de las funciones para devolver los archivos almencenadaso en memoria temporal
  getFileContent probablemente no sea tan "útil" como usuario, pero sí la usan los Nodos para pasarse datos para el Reduce, y, ya que está, exponérsela al FS ayuda a que,
  por ejemplo, mientras desarrollan puedan chequear de manera "fácil" que los temporales se estén generando bien. Poder inspeccionar lo que está pasando en el sistema siempre
   es bueno, y si encima viene casi gratis en cuanto a esfuerzo de desarrollo, mejor.

}*/

/*int nodeMap (rutinaMap, int nroBloque){
	 El Nodo4 guarda el contenido de map.py en un archivo en el filesystem local. Le da permisos de ejecución.
	 * El hilo mapper le solicita al Nodo4 que envie el contenido del Bloque6 por entrada estánda a map.py. El STDOUT lo almacena en un archivo temporal (ej: map.py.result.tmp)
	 * Usa la tool sort para ordenar el archivo temporal del paso anterior ya en el archivo definitivo
	# cat map.py.result.tmp | sort > librazo12347.tmp
	 * El hilo mapper se conecta al nodo, y le indica la rutina de maping, el bloque de datos donde aplicarla y tiene que almacenar
	* los resultados de manera ordenada (sort) en el FS Temporal del nodo. Debera dar una respuesta al hilo MApper
	int lugarDeAlmacenamiento;
	return lugarDeAlmacenamiento;
}*/

/*void nodeReduce (array[string nameNode, int nroBloque], rutinaReduce, char nombreDondeGuarda){
	//el reduce recibe un nodo y un nombre de archivo (el FS se encargara de rearmar ese archivo y pasarlo)
	  El hilo reduce, indica aplicar la rutina sobre varios archvos del espacio temporal, de los cuales uno debe ser siempre local al nodo
	 * El reduce le manda el nombre de los bloques y los nodos donde se encuentran, el codigo de la rutina de reduce y el nombre del
	 * archivo donde se alamcenara. Al finalizar se debe informar al JOB que termino
	return 0;
}*/

void getInfoConf(char* conf)
{
	t_config* config; //creamos la variable que va a ser el archivo de config
	config = config_create(conf); //creamos el "objeto" archivo de config
	strcpy(ip_fs,config_get_string_value(config,"IP_FS"));
	puerto_fs = config_get_int_value(config,"PUERTO_FS");
	//strcpy(ip_nodo,config_get_string_value(config, "IP_NODO"));
	puerto_nodo = config_get_int_value(config, "PUERTO_NODO");
	strcpy(archivo_bin,config_get_string_value(config,"ARCHIVO_BIN"));
	//strcpy(dir_tmp,config_get_string_value(config,"DIR_TMP"));
	//strcpy(nodo_nuevo,config_get_string_value(config,"NODO_NUEVO"));
	printf("Extraccion correcta del archivo de configuracion");

	config_destroy(config); //destruimos el "objeto" archivo de config
}
/*
 * Obtiene todos los datos del archivo de configuracion y los guarda en variables
 * para que podamos utilizarlo a lo largo del programa
 * ACLARACION:
 * 			 El archivo de configuracion se pasa por parametro cuando se realiza la
 * 			 ejecucion: ./Nodo.c "rutaArchivoConfig"
 */

int size_of(int fd){
	struct stat buf;
	fstat(fd, &buf);
	return buf.st_size;
}

