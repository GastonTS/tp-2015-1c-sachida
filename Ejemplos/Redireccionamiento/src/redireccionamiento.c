/*
 ============================================================================
 Name        : Redireccionamiento.c
 Author      : Ian
 Version     :
 Copyright   : 
 Description :
 ============================================================================
 */

#include <stdio.h>
#include <stdlib.h>
#include <string.h>

//-------------------------------------------------------------------------------------------------
int redireccionamiento(char *STDIN,char *STDOUT)
{
	//CREO EL TIPO QUE VA A DEVOLVER EL POPEN
	FILE *entradaARedirigir = NULL;

	//RESERVO LA MEMORIA DEL TAMAÑO DEL MENSAJE
	char *mensaje=malloc(5+strlen(STDIN)+11+strlen(STDOUT));

	//ARMO EL MENSAJE PARA REDIRECCIONAR
	sprintf(mensaje,"more %s | sort > %s",STDIN,STDOUT);

	//EJECUTO EL COMANDO
	entradaARedirigir = popen (mensaje,"w");

	//VERIFICO EL VALOR DEL RESULTADO
	if (entradaARedirigir != NULL)
	{
		printf ("La redirecion ha sido exitosa");

		pclose (entradaARedirigir);

		free(mensaje);
	}
	else
	{
		printf("La redirecion ha sido fallida");
		return -1;
	}


	return 0;
}

//-------------------------------------------------------------------------------------------------
int main(void) {
	char *entrada[50], *salida[50];
	printf ("Ingrese la ruta de la entrada\n");
	scanf ("%s",entrada);
	printf ("Ingrese la ruta de la salida\n");
	scanf ("%s",salida);

	//LLAMO A LA FUNCION PARA HACER EL REDIRECCIONAMIENTO PASANDOLE POR PARAMETRO EL STDIN Y STDOUT
	redireccionamiento(entrada,salida);

	return 0;
}
