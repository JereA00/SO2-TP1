#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/types.h> 
#include <sys/socket.h>
#include <netinet/in.h>
#include <sys/msg.h>

#define TAM 256

typedef struct 
{
	long Id_Mensaje;
	int32_t Dato_Numerico;
	//char* name;
	//char* pass;
	char name[30];
	char pass[30];
}mi_tipo_mensaje;


void inicio_sesion(char* buffer,int32_t Id_Cola_Mensajes, int32_t newsockfd, int32_t intentos);
void chequeo_terminacion(char* buffer, int32_t Id_Cola_Mensajes, int32_t newsockfd);
void recibir_usuario(char* buffer, int32_t newsockfd);
void escribir_usuario(int32_t newsockfd, char* msg, char* buffer);

int main( int argc, char *argv[] ){

	/*Definicion de variables*/
	int32_t sockfd, newsockfd, puerto, clilen;
	struct sockaddr_in serv_addr, cli_addr;
	key_t Clave1;
	int32_t Id_Cola_Mensajes;
	int32_t intentos = 3;


	/*Se comprueba la cantidad de argumentos*/
	if ( argc < 2 ) {
        fprintf( stderr, "Uso: %s <puerto>\n", argv[0] );
		exit( 1 );
	}

	/*Se crea y configura el socket del SV*/
	sockfd = socket( AF_INET, SOCK_STREAM, 0);

	memset( (char *) &serv_addr, 0, sizeof(serv_addr) );
	puerto = atoi( argv[1] );
	serv_addr.sin_family = AF_INET;				//Familia de direcciones
	serv_addr.sin_addr.s_addr = INADDR_ANY;		//Escucha todas las Direccion IPv4 que tiene conectadas
	serv_addr.sin_port = htons( puerto );		//Nº de puerto

	/*Enlaza el servidor a un puerto y direccion IPv4*/
	if ( bind(sockfd, ( struct sockaddr *) &serv_addr, sizeof( serv_addr ) ) < 0 ) {
		perror( "ligadura" );
		exit( 1 );
	}

    //Establece la cantidad maxima de clientes que puede atender el servidor
	listen( sockfd, 1 );
	clilen = sizeof( cli_addr );

	/*CREACION DE COLA DE MENSAJES*/

	/*Para conseguir una clave, de tipo key_t, que sea común para todos los programas que quieran compartir la cola de mensajes*/
	Clave1 = ftok ("/bin/ls", 40);	

	if (Clave1 == (key_t)-1)
	{
		printf("Error al obtener clave para cola mensajes\n");
		exit(-1);
	}
	
	//	Se crea la cola de mensajes y se obtiene un identificador para ella.
	// - El IPC_CREAT indica que cree la cola de mensajes si no lo está ya.
	// - El 0600 son permisos de lectura y escritura para el usuario que lance los procesos.
	Id_Cola_Mensajes = msgget (Clave1, 0600 | IPC_CREAT);

	printf("id de cola de mensajes; %d\n",Id_Cola_Mensajes);

	if (Id_Cola_Mensajes == -1)
	{
		printf ("Error al obtener identificador para cola mensajes\n");
		exit (-1);
	}

	char* buffer = (char *) calloc(TAM,sizeof(char)); 
	while( 1 ) {
		//Acepta la comunicacion con el cliente
		newsockfd = accept( sockfd, (struct sockaddr *) &cli_addr, &clilen );	
		
		inicio_sesion(buffer,Id_Cola_Mensajes,newsockfd,intentos);
		    //ENVIAR PROMPT
		escribir_usuario(newsockfd, "promp: ",buffer);
		recibir_usuario(buffer, newsockfd);

	}
	return 0; 
} 

void inicio_sesion(char* buffer,int32_t Id_Cola_Mensajes, int32_t newsockfd, int32_t intentos){
	
	//char* buffer_aux = (char *) calloc(TAM,sizeof(char*));

	mi_tipo_mensaje msg_inicio_sesion;
	//msg_inicio_sesion.name = (char *) calloc(30,sizeof(char));
	//msg_inicio_sesion.pass = (char *) calloc(30,sizeof(char));
	
	mi_tipo_mensaje msg_respuesta;
	//msg_respuesta.name = (char *) calloc(30,sizeof(char));
	//msg_respuesta.pass = (char *) calloc(30,sizeof(char));

	/*Escriba_usuario -> Mensaje al cliente*/
	escribir_usuario(newsockfd, "Ingrese el nombre de usuario: ",buffer);

	/*User_name -> Respuesta del cliente*/
	recibir_usuario(buffer, newsockfd);
	/*Chequeo si hay que terminar*/
	//chequeo_terminacion(buffer,Id_Cola_Mensajes);

	/*	Se rellenan los campos del mensaje que se quiere enviar.
		El Id_Mensaje es un identificador del tipo de mensaje. Luego se podrá
		recoger aquellos mensajes de tipo 1, de tipo 2, etc.
		Dato_Numerico es un dato que se quiera pasar al otro proceso. Se pone,por ejemplo 29.
		Mensaje es un texto que se quiera pasar al otro proceso. */

	msg_inicio_sesion.Id_Mensaje = 1;
	msg_inicio_sesion.Dato_Numerico = 0;
	strcpy (msg_inicio_sesion.name, buffer);
	printf("name mensaje: %s\n",msg_inicio_sesion.name);

	/*Escriba_usuario -> Mensaje al cliente*/
	escribir_usuario(newsockfd, "Ingrese la contraseña: ",buffer);

	/*User_name -> Respuesta del cliente*/
	recibir_usuario(buffer, newsockfd);

	strcpy (msg_inicio_sesion.pass, buffer);

	printf("pass mensaje: %s\n",msg_inicio_sesion.pass);

	msgsnd (Id_Cola_Mensajes, (struct msgbuf *)&msg_inicio_sesion,
		sizeof(msg_inicio_sesion.Dato_Numerico)+sizeof(msg_inicio_sesion.name)+sizeof(msg_inicio_sesion.pass),IPC_NOWAIT); 

	printf("\n");
	printf("El mensaje se envio\n");

	msgrcv (Id_Cola_Mensajes, (struct msgbuf *)&msg_respuesta,
		sizeof(msg_respuesta.Dato_Numerico) + sizeof(msg_respuesta.name) + sizeof(msg_respuesta.pass), 1, 0);

	printf("Recibido mensaje tipo: %ld \n", msg_respuesta.Id_Mensaje); 
	printf("Dato_Numerico = %d \n", msg_respuesta.Dato_Numerico);
	printf("Mensaje = %s\n", msg_respuesta.name);
	printf("Mensaje2 = %s\n",msg_respuesta.pass);
	
	switch(msg_respuesta.Dato_Numerico){
		case 0:
			intentos--;
			if(intentos == 0){
				escribir_usuario(newsockfd, "Usuario Bloqueado",buffer);

				msg_respuesta.Id_Mensaje = 2;
				msgsnd (Id_Cola_Mensajes, (struct msgbuf *)&msg_inicio_sesion,
				sizeof(msg_inicio_sesion.Dato_Numerico)+sizeof(msg_inicio_sesion.name)+sizeof(msg_inicio_sesion.pass),IPC_NOWAIT);

				msgctl (Id_Cola_Mensajes, IPC_RMID, (struct msqid_ds *)NULL);
				free(buffer);
				exit(1);
			}
			inicio_sesion(buffer,Id_Cola_Mensajes,newsockfd,intentos);
		case 1:
			escribir_usuario(newsockfd, "Usuario Valido",buffer);
			break;
		case -1: 
			escribir_usuario(newsockfd, "Usuario Bloqueado",buffer);

			msg_respuesta.Id_Mensaje = 2;
			msgsnd (Id_Cola_Mensajes, (struct msgbuf *)&msg_respuesta,
			sizeof(msg_respuesta.Dato_Numerico)+sizeof(msg_respuesta.name)+sizeof(msg_respuesta.pass),IPC_NOWAIT);
			msgctl (Id_Cola_Mensajes, IPC_RMID, (struct msqid_ds *)NULL);
			free(buffer);
			exit(1);
		default:
			msgctl (Id_Cola_Mensajes, IPC_RMID, (struct msqid_ds *)NULL);
			free(buffer);
			exit(1);
	}
}

void chequeo_terminacion(char* buffer, int32_t Id_Cola_Mensajes,int32_t newsockfd){
	buffer[strlen(buffer)-1] = '\0';
	if( !strcmp( "fin", buffer ) ) {
		/* Se borra y cierra la cola de mensajes.
		IPC_RMID indica que se quiere borrar. El puntero del final son datos
		que se quieran pasar para otros comandos. IPC_RMID no necesita datos,
		así que se pasa un puntero a NULL.*/
		msgctl (Id_Cola_Mensajes, IPC_RMID, (struct msqid_ds *)NULL);
		close(newsockfd);
		exit(0);
	}
}

void escribir_usuario(int32_t newsockfd, char* msg, char* buffer){
	/* zero indicates end of transmission */
	int32_t n;
	memset( buffer, 0, TAM );
	/*ssize_t enviados = send(newsockfd, msg, sizeof(msg), 0);
	if(enviados<0){
		printf("Error en envio al cliente");
		free(buffer);
		exit(1);
	}*/
	printf("%s\n",msg);
	n = write( newsockfd, msg, strlen(msg));
	if ( n < 0 ) {
		perror( "escritura en socket" );
		exit( 1 );
	}
}

void recibir_usuario(char* buffer, int32_t newsockfd){
	int32_t n;
	/*memset( buffer, 0, TAM );
	n = recv(newsockfd,(void *)buffer, TAM-1, 0);
	if ( n < 0 ) {
		printf("lectura de socket");
		free(buffer);
		exit(1);
	}	
	printf( "Recibí: %s", buffer );*/
	memset( buffer, '\0', TAM );
	n = read( newsockfd, buffer, TAM );
	if ( n < 0 ) {
		perror( "lectura de socket" );
		exit( 1 );
	}
	printf("%s",buffer);
}