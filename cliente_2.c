#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <termios.h>
#include <sys/types.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <netdb.h> 

#define TAM 256
#define CANT_USERS 10


void error_envio(ssize_t enviados,char* buffer);
void enviar_dato(char* buffer, int32_t sockfd);
void recibir_dato(char* buffer, int32_t sockfd);

int main(int argc,char *argv[])
{
	/*Definicion variables*/
	int32_t sockfd, puerto,n;
	struct sockaddr_in serv_addr;
	struct hostent *server;
	int32_t loggeo = 0;

	char* buffer = calloc(TAM, sizeof(char));

	/*Mensaje de error por falta de parametros*/
	if ( argc < 3 ) {
		fprintf( stderr, "Ingrese <localhost> <Puerto>\n");
		exit( 0 );
	}

	/*Se crea puerto y se configura con puerto y direccion IP*/
	puerto = atoi( argv[2] );						//Pasamo el numero puerto a entero.
	sockfd = socket( AF_INET, SOCK_STREAM, 0 );		//Creamos el puerto

	server = gethostbyname( argv[1] );				//Obtiene informacion respecto al cliente, en sintesis, la direccion IPv4

	memset( (char *) &serv_addr, '0', sizeof(serv_addr) );
	serv_addr.sin_family = AF_INET;
	bcopy( (char *)server->h_addr, (char *)&serv_addr.sin_addr.s_addr, server->h_length );	//Asigna la direccion IPv4 a la estructura serv_addr
	serv_addr.sin_port = htons( puerto ); //Asigna el puerto a la estructura serv_addr

	/*Establece la conexion con el servidor*/
	if ( connect( sockfd, (struct sockaddr *)&serv_addr, sizeof(serv_addr ) ) < 0 ) {	
		perror( "conexion" );
		exit( 1 );
	}


	while(1) {
		recibir_dato(buffer, sockfd);
		//printf("hola\n");
		if(loggeo!=1){
			//printf("entra aca\n");
			printf( "%s", buffer );
			buffer[strlen(buffer)-1] = '\0';
			//strcpy(buffer, strtok(buffer,""));
			//VER LO DE CTRL + C para salir
			if((!strcmp( "Ingrese el nombre de usuario:", buffer )) || (!strcmp( "Ingrese la contraseña:", buffer))) {
				//printf("comparo bien\n");
				enviar_dato(buffer,sockfd);
			}else if ( !strcmp( "Usuario Logeado", buffer )){
				loggeo = 1;
			}
		}

		//MODO CONSOLA
		//enviar_dato(buffer,sockfd);
		//exit(1);
	}
	free(buffer);
	close(sockfd);
}

void enviar_dato(char* buffer, int32_t sockfd){
	//printf( "Ingrese el mensaje a transmitir: " );
	int32_t n;
	memset( buffer, '\0', TAM);
	fgets( buffer, TAM-1, stdin );

	/*ssize_t enviados=send(sockfd,(void *)buffer, strlen(buffer), 0);
	error_envio(enviados,buffer);
		
	if( !strcmp( "fin", buffer ) ) {
		printf( "Finalizando ejecución\n" );
		exit(1);
	}
	*/
	printf("%s",buffer);
	//printf("se envia otro vacio \n");
	n = write( sockfd, buffer, strlen(buffer));
	if ( n < 0 ){
		perror( "escritura en socket" );
		exit( 1 );
	}
}

void error_envio(ssize_t enviados,char* buffer){
	if((enviados != strlen(buffer)) || enviados < 0){ 
		perror("send() envio un diferente numero de bytes que los esperados");
		exit(1);
	}
}

void recibir_dato(char* buffer, int32_t sockfd){

	//int32_t totalBytesRcvd=0;
	int32_t n;

	memset(buffer, '\0', TAM);
	/*
	while (totalBytesRcvd < strlen(buffer)) {
	      // Receive up to the buffer size (minus 1 to leave space for
	      // a null terminator) bytes from the sender
	    n = recv(sockfd,(void *) buffer, TAM-1, 0);
	    if (n < 0) {
	        printf("recv() failed");
	    }
	    else if (n == 0) {
	        printf("recv() connection closed prematurely");
	    }       
	    totalBytesRcvd += n; 	 		// Keep tally of total bytes
  	}

  	printf("Recibio: %s",buffer);      // Print the buffer*/
  	//memset( buffer, '\0', TAM );
	n = read( sockfd, buffer, TAM );
	if ( n < 0 ) {
		perror( "lectura de socket" );
		exit( 1 );
	}
	//printf( "Respuesta: %s\n", buffer );
}
