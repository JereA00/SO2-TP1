#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/msg.h>


typedef struct
{
	char name[20];
	char password[20];
}users;

typedef struct 
{
	long Id_Mensaje;
	int32_t Dato_Numerico;
	//char* name1;
	//char* pass1;
	char name[30];
	char pass[30];
}mi_tipo_mensaje;

void check_user(users cliente, mi_tipo_mensaje mensaje, int32_t Id_Cola_Mensajes);
void bloquear_user(users cliente);
void inicio_sesion(int32_t Id_Cola_Mensajes, mi_tipo_mensaje mensaje, users cliente, int32_t estado);
void enviar_pserver(mi_tipo_mensaje mensaje, char* msg, int32_t bloqueado, int32_t id, int32_t Id_Cola_Mensajes);

int main(int argc, char *argv[]){
	key_t Clave1;
	int32_t Id_Cola_Mensajes;

	users cliente;

	mi_tipo_mensaje mensaje;
	//mensaje.name1 = (char *) calloc(30,sizeof(char));
	//mensaje.pass1 = (char *) calloc(30,sizeof(char));


	/* Igual que en cualquier recurso compartido (memoria compartida, semaforos 
	o colas) se obtien una clave a partir de un fichero existente cualquiera 
	y de un entero cualquiera. Todos los procesos que quieran compartir este
	semaforo, deben usar el mismo fichero y el mismo entero.*/
	Clave1 = ftok ("/bin/ls", 40);
	if (Clave1 == (key_t)-1)
	{
		printf("Error al obtener clave para cola mensajes\n");
		exit(-1);
	}
	
	/*	Se crea la cola de mensajes y se obtiene un identificador para ella.
	 El IPC_CREAT indica que cree la cola de mensajes si no lo está ya.
	 el 0600 son permisos de lectura y escritura para el usuario que lance
	 los procesos.*/
	Id_Cola_Mensajes = msgget (Clave1, 0600 | IPC_CREAT);
	if (Id_Cola_Mensajes == -1)
	{
		printf("Error al obtener identificador para cola mensajes\n");
		exit (-1);
	}
	printf("id de cola de mensajes; %d\n",Id_Cola_Mensajes);
	while(1){
		msgrcv (Id_Cola_Mensajes, (struct msgbuf *)&mensaje,
			sizeof(mensaje.Dato_Numerico) + sizeof(mensaje.name) + sizeof(mensaje.pass), 1, 0);

		printf("recibio\n");
		printf("Dato numerico recibido: %d \n",mensaje.Dato_Numerico);
		printf("name recibido: %s\n",mensaje.name);
		printf("pass recibida: %s\n", mensaje.pass);

		switch (mensaje.Id_Mensaje){
			case 1:
				check_user(cliente, mensaje, Id_Cola_Mensajes);
				break;
			case 2:
				bloquear_user(cliente);
				break;
		}

			//TERMINAR PROCESO Y CERRAR COLA
			/* Se borra y cierra la cola de mensajes. IPC_RMID indica que se quiere borrar. El puntero del final son datos
			que se quieran pasar para otros comandos. IPC_RMID no necesita datos, así que se pasa un puntero a NULL.*/
		msgctl (Id_Cola_Mensajes, IPC_RMID, (struct msqid_ds *)NULL);
		}
	}

void check_user(users cliente, mi_tipo_mensaje mensaje, int32_t Id_Cola_Mensajes){
	printf("entra a la funcion check_user \n");
	FILE *f;
	f = fopen("base_de_datos.txt","r"); //abrimos la base de datos para solo lectura
	if(f == NULL){
		printf("No se ha podido abrir el fichero\n");
		exit(1);
	}
	int32_t estado;

	printf("abrio la base de datos\n");

	strcpy(cliente.name,strtok(mensaje.name,"\n"));
	strcpy(cliente.password, strtok(mensaje.pass,"\n"));

	printf("cliente name: %s\n",cliente.name);
	printf("cliente pass: %s\n",cliente.password);

	char* chain_2 = calloc(30,sizeof(char));
	char* name_2 = calloc(30,sizeof(char));
	char* password_2 = calloc(30,sizeof(char));



	while ( fscanf(f,"%s", chain_2) == 1)
    {
		strcpy(name_2,strtok(chain_2,"-"));
		strcpy(password_2, strtok(NULL,"-"));
		estado=atoi(strtok(NULL,"\n"));
		//printf("name:%s\n",name_2);
		//printf("pass:%s\n",password_2);
		//printf("estado:%d\n",estado);


        if(strcmp(name_2, cliente.name)==0) {//if match found
        	printf("usuario correcto\n");
        	if(strcmp(password_2, cliente.password)==0){
        		if(estado==1){
        			printf("usuario bloqueado\n");
        			enviar_pserver(mensaje,"",-1,1, Id_Cola_Mensajes);
        		}
        		printf("password correcta\n");
				enviar_pserver(mensaje,"",1,1, Id_Cola_Mensajes);
        	}
        	enviar_pserver(mensaje,"",0,1, Id_Cola_Mensajes);
    	}
    }
    free(chain_2);
    free(name_2);
    free(password_2);
	fclose(f);
}

void enviar_pserver(mi_tipo_mensaje mensaje, char* msg, int32_t bloqueado, int32_t id, int32_t Id_Cola_Mensajes){
	mensaje.Id_Mensaje = id;
	mensaje.Dato_Numerico = bloqueado;
	strcpy (mensaje.name, msg);

	msgsnd (Id_Cola_Mensajes, (struct msgbuf *)&mensaje, 
	sizeof(mensaje.Dato_Numerico)+sizeof(mensaje.name)+sizeof(mensaje.pass), IPC_NOWAIT);
}

void bloquear_user(users cliente){
	FILE *f;
	f = fopen("base_de_datos.txt", "r+"); //abrimos la base de datos para solo lectura

	if(f == NULL){
		printf("No se ha podido abrir el fichero\n");
		exit(1);
	}
	int32_t bloqueado;

	typedef struct 
	{
		char name[30];
		char pass[30];
		int32_t lock;
	}texto; 

	char* chain = calloc(30,sizeof(char));
	char* fila = (char*) calloc(20,sizeof(char));

	int32_t cont_filas=0;
	int32_t i = 0;

	while(fgetc(f)!=EOF){
		if(fgetc(f)=='\n'){
			cont_filas++;
		}
	}
	if(cont_filas == 0){
		exit(1);
	}

	texto reescribir[cont_filas];

	for( i=0 ; i< cont_filas; i++)
    {	
    	fseek(f,0,SEEK_SET);
    	strcpy(reescribir[i].name,strtok(chain,"-"));
		strcpy(reescribir[i].pass, strtok(NULL,"-"));
		bloqueado = atoi(strtok(NULL,"\n"));
    	if(!strcmp(reescribir[i].name,cliente.name)){
    		reescribir[i].lock = 1;
    	}
    	reescribir[i].lock = bloqueado;
    	i++;
    }
    fclose(f);

    FILE *f2;
    f2 = fopen("base_de_datos.txt", "w");
    i=0;
    while(i<cont_filas){
    	fprintf(f2, "%s-%s-%i\n",reescribir[i].name,reescribir[i].pass,reescribir[i].lock);
    	i++;
    }
    free(chain);
    free(fila);
    fclose(f2);
}