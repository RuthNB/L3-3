#include <string.h>
#include <unistd.h>
#include <stdlib.h>
#include <sys/types.h>
#include <sys/socket.h>
#include <fcntl.h>
#include <netinet/in.h>
#include <stdio.h>
#include <mysql.h>
#include <pthread.h>

typedef struct {
	char nombre[20];
	int socket;
}Tconectado;

typedef struct{
	int num;
	Tconectado conectados[100];
}TListaConectados;

pthread_mutex_t mutex=PTHREAD_MUTEX_INITIALIZER;
void Inicializar(MYSQL *conn)
{
	int i=0;
	//MYSQL *conn;
	int err;
	// Estructura especial para almacenar resultados de consultas 
	MYSQL_RES *resultado;
	MYSQL_ROW row;
	//Creamos una conexion al servidor MYSQL 
	conn = mysql_init(NULL);
	if (conn==NULL) 
	{
		printf ("Error al crear la conexion: %u %s\n", mysql_errno(conn), mysql_error(conn));
		exit (1);
	}
	//inicializar la conexion
	conn = mysql_real_connect (conn, "localhost","root", "mysql", "cluedo",0, NULL, 0);
	if (conn==NULL) 
	{
		printf ("Error al inicializar la conexion: %u %s\n", 
				mysql_errno(conn), mysql_error(conn));
		exit (1);
	}
	
}
int UsuarioConectado(char nombre[20],int socket,TListaConectados*lista)
{
	int i=0;
	int encontrado=0;
	while((i<(lista->num)&&(encontrado==0)))
	{
		if(strcmp(lista->conectados[i].nombre,nombre)==0)
			encontrado=1;
		else
			i=i+1;
	}
	if(encontrado==0)
	{
		if((lista->num)>=100)
			return -1;
		else
		{
			strcpy(lista->conectados[lista->num].nombre,nombre);
			lista->conectados[lista->num].socket=socket;
			lista->num=(lista->num)+1;
			printf("Added user with socket %d\n", socket);
			return 0;
		}
	}	   
}
int UsuarioDesonectado(char nombre[20],int socket,TListaConectados*lista)
{
	int i=0;
	int encontrado=0;
	while((i<(lista->num)&&(encontrado==0)))
	{
		if(strcmp(lista->conectados[i].nombre,nombre)==0)
			encontrado=1;
		else
			i=i+1;
	}
	while((i<((lista->num)-1)))
	{
		strcpy(lista->conectados[i].nombre,lista->conectados[i+1].nombre);
		printf("%s",lista->conectados[i].nombre);
		lista->conectados[i].socket = lista->conectados[i+1].socket;
		i = i + 1;
	}
	if(encontrado==1)
	{
		strcpy (lista->conectados[(lista->num)-1].nombre,"");
		lista->conectados[(lista->num)-1].socket = 0;
		return 0;
	}
	else
	   return -1; //si no se ha encontrado al usuario a eliminar de la lista
}
int VerEstadoUsuarios(char nombreconectados[100],int socket,TListaConectados*lista)
{
	lista->num =(lista->num)-1;
	sprintf(nombreconectados, "%d", lista->num); //primero agregamos la cantidad de conectados 
	int i;
	for (i=0;i<(lista->num);i++) //bucle que va agregando los conectados al string conectados
	{
		printf("%s",lista->conectados[i].nombre);
		sprintf(nombreconectados,"%s,%s,%d",nombreconectados,lista->conectados[i].nombre,lista->conectados[i].socket);
	}
}
int EncontrarSocket(char nombre[20],int socket,TListaConectados*lista)
{
	int encontrado=0;
	int i=0;
	while((i<(lista->num))&&(encontrado == 0))
	{
		if((strcmp(lista->conectados[i].nombre,nombre)==0))
			encontrado=1;
		else 
			i=i+1;
	}
	if(encontrado)
	{
		return lista->conectados[i].socket;
	}
	else
	   return -1;
}
void *AtenderCliente (void *socket,TListaConectados*lista)
{
	int sockets[100];
	int sock_conn;
	int *s;
	s = (int *) socket;
	sock_conn = *s;
	printf("Valor del socket iniciado%d \n",sock_conn);
	char peticion[512];
	char respuesta[512];
	int ret;
	int err;
	int i=0;
	MYSQL *conn;
	MYSQL_RES *resultado;
	MYSQL_ROW row;
	Inicializar(conn);
	int terminar = 0;
	while(terminar != 1)
	{	
		ret=read(sock_conn,peticion, sizeof(peticion));
		printf("Recibido\n");
		peticion[ret]='\0';
		//Escribimos el nombre en la consola
		printf ("Peticion: %s\n",peticion);
		//Vamos a ver que quieren
		char *p=strtok (peticion, "/");
		int codigo = atoi (p);
		printf("Codigo: %d \n",codigo);
		//Acabar 
		if (codigo==0)
			terminar = 1; 
		//Dime tus amigos
		else if (codigo == 1)
		{
			p=strtok(NULL,"/");
			MYSQL *conn;
			Inicializar(conn);
			char friends[512];
			char consulta[80];
			
			// construimos la consulta SQL
			strcpy (consulta,"SELECT FRIENDS FROM PLAYER WHERE PLAYER.ID = '"); 
			strcat (consulta, p);
			strcat (consulta,"'");
			printf("%s\n",consulta);
			// hacemos la consulta 
			err=mysql_query (conn, consulta); 
			printf("Error = %d\n", err);
			if (err!=0) 
			{
				printf ("Error al consultar datos de la base %u %s\n",
						mysql_errno(conn), mysql_error(conn));
				exit (1);
			}
			//recogemos el resultado de la consulta 
			resultado = mysql_store_result (conn); 
			row = mysql_fetch_row (resultado);
			if (row == NULL)
			{
				printf ("No se han obtenido datos en la consulta\n");
			}
			else 
				sprintf(respuesta,"%s,",row[0]);	
			printf("respuesta: %s\n", respuesta);
			mysql_close(conn);
		}
		//
		else if (codigo ==2)
		{
			p=strtok(NULL,"/");
			MYSQL *conn;
			Inicializar(conn);
			char player[60];
			int ganadas = 0;
			int jugadas = 0;
			float porcentaje = 0;
			char consulta[80];
			strcpy(player,p);
			strcpy (consulta,"SELECT PARTICIPATION.GAME FROM PARTICIPATION WHERE PARTICIPATION.POSITION = 1 AND PARTICIPATION.PLAYER ='"); 
			strcat (consulta, player);
			strcat (consulta,"'");
			// hacemos la consulta 
			err=mysql_query (conn, consulta); 
			if (err!=0) 
			{
				printf ("Error al consultar datos de la base %u %s\n",
						mysql_errno(conn), mysql_error(conn));
				exit (1);
			}
			//recogemos el resultado de la consulta 
			
			resultado = mysql_store_result (conn); 
			row = mysql_fetch_row (resultado);
			if (row == NULL)
				printf ("No se han obtenido datos en la consulta\n");
			else
			{
				
				while (row!= NULL)
				{
					ganadas++;
					row=mysql_fetch_row(resultado);
				}
			}
			strcpy (consulta,"SELECT PARTICIPATION.GAME FROM PARTICIPATION WHERE PARTICIPATION.PLAYER ='"); 
			strcat (consulta, player);
			strcat (consulta,"'");
			// hacemos la consulta 
			err=mysql_query (conn, consulta); 
			if (err!=0) 
			{
				printf ("Error al consultar datos de la base %u %s\n",
						mysql_errno(conn), mysql_error(conn));
				exit (1);
			}
			//recogemos el resultado de la consulta 
			resultado = mysql_store_result (conn); 
			row = mysql_fetch_row (resultado);
			if (row == NULL)
				printf ("No se han obtenido datos en la consulta\n");
			else
			{
				
				while (row!= NULL)
				{
					jugadas++;
					row=mysql_fetch_row(resultado);
				}
			}
			porcentaje=(ganadas*100.0)/jugadas;
			printf("%s ha ganado el %f por ciento de las partidas. Ha ganado %d partidas de las %d que ha jugado\n", player, porcentaje, ganadas, jugadas);
			sprintf(respuesta,"%f",porcentaje);
		}
		else if (codigo ==3)
		{
			
			p=strtok(NULL,"/");
			MYSQL *conn;
			Inicializar(conn);
			char player[60];
			int ganadas = 0;
			char consulta[80];
			printf("%s\n", p);
			strcpy(player,p);
			strcpy (consulta,"SELECT PARTICIPATION.GAME FROM PARTICIPATION WHERE PARTICIPATION.POSITION = 1 AND PARTICIPATION.PLAYER ='"); 
			strcat (consulta, player);
			strcat (consulta,"'");
			// hacemos la consulta 
			err=mysql_query (&conn, consulta); 
			if (err!=0) 
			{
				printf ("Error al consultar datos de la base %u %s\n",
						mysql_errno(conn), mysql_error(conn));
				exit (1);
			}
			//recogemos el resultado de la consulta 
			resultado = mysql_store_result (conn); 
			row = mysql_fetch_row (resultado);
			if (row == NULL)
				printf ("No se han obtenido datos en la consulta\n");
			else
			{
				while (row!= NULL)
				{
					ganadas++;
					row=mysql_fetch_row(resultado);
				}
			}
			printf("%s ha ganado %d partidas\n", player, ganadas);
			sprintf(respuesta,"%d",ganadas);
			mysql_close;
		}
		
		else if (codigo == 4)
		{
			p=strtok(NULL,"/");
			MYSQL *conn;
			Inicializar(conn);
			char player[60];
			int jugadas = 0;
			char consulta[80];
			printf("%s\n", p);
			strcpy(player,p);
			strcpy (consulta,"SELECT PARTICIPATION.GAME FROM PARTICIPATION WHERE PARTICIPATION.PLAYER ='"); 
			strcat (consulta, player);
			strcat (consulta,"'");
			// hacemos la consulta 
			err=mysql_query (conn, consulta); 
			if (err!=0) 
			{
				printf ("Error al consultar datos de la base %u %s\n",
						mysql_errno(conn), mysql_error(conn));
				exit (1);
			}
			//recogemos el resultado de la consulta 
			resultado = mysql_store_result (conn); 
			row = mysql_fetch_row (resultado);
			if (row == NULL)
				printf ("No se han obtenido datos en la consulta\n");
			else
			{
				while (row!= NULL)
				{
					jugadas++;
					row=mysql_fetch_row(resultado);
				}
			}
			printf("%s ha jugado %d partidas\n", player, jugadas);
			sprintf(respuesta,"%d",jugadas);
		}
		//Crear cuenta
		if (codigo == 5)
		{
			p=strtok(NULL,"/");
			MYSQL *conn;
			Inicializar(conn);
			char user[100];
			char password[100];
			char consulta[80];
			strcpy(user,p);
			p=strtok(NULL,"/");
			strcpy(password,p);
			// construimos la consulta SQL
			strcpy (consulta,"INSERT INTO PLAYER VALUES('"); 
			strcat (consulta, user);
			strcat (consulta,"','");
			strcat (consulta, password);
			strcat (consulta,"','-/-/-');");
			// hacemos la consulta 
			err = mysql_query (conn, consulta); 
			if (err!=0) 
			{
				printf ("Error al consultar datos de la base %u %s\n",
						mysql_errno(conn), mysql_error(conn));
				exit (1);
			}
			mysql_close (conn);
		}
		if (codigo == 6)
		{
			p=strtok(NULL,"/");
			MYSQL *conn;
			Inicializar(conn);
			char user[100];
			char password[100];
			char consulta[80];
			strcpy(user,p);
			p=strtok(NULL,"/");
			strcpy(password,p);
			
			printf("%s \n", password);
			strcpy(consulta, "SELECT COUNT(ID) FROM PLAYER WHERE PLAYER.ID='");
			strcat(consulta, user);
			strcat(consulta, "' AND PLAYER.PASSWORD= '");
			strcat(consulta, password);
			strcat(consulta, "';");
			err=mysql_query (conn, consulta); 
			
			if (err!=0) 
			{
				printf ("Error al consultar datos de la base %u %s\n",
						mysql_errno(conn), mysql_error(conn));
				exit (1);
			}
			resultado = mysql_store_result (conn);
			row = mysql_fetch_row (resultado);
			
			if (row == NULL)
				sprintf (respuesta,"No se han obtenido datos en la consulta\n");
			else{
				while (row !=NULL) 
				{
					sprintf (respuesta,"%s \n", row[0]);
					// obtenemos la siguiente fila
					row = mysql_fetch_row (resultado);
				}
				int m;
				m=atoi(respuesta);
				if(m==1)
				{
					sprintf(respuesta,"2/Usuario registrado correctamente");
					printf("User logueado: %s, socket: %d\n", user, sock_conn); 
					pthread_mutex_lock(&mutex); //no interrumpas ahora, bloqueamos el thread
					UsuarioConectado(user,sock_conn,&lista);
					pthread_mutex_unlock(&mutex); 
				}
				else
				   printf("Datos introducidos erroneos \n");
				
			}
			//si se selecciona algo se envia su correspondiente respuesta
			printf ("Respuesta: %s\n", respuesta);
		}
		write (sock_conn,respuesta,strlen(respuesta));
}
int main(int argc, char *argv[])
{	
	TListaConectados miLista;
	int sockets[100];
	int sock_conn, sock_listen;
	struct sockaddr_in serv_adr;
	 //INICIALITZACIONS
	 //Obrim el socket
	if((sock_listen=socket(AF_INET,SOCK_STREAM,0))<0)
		printf("Error creant socket\n");
	 //Fem el bind al port
	 //inicialitza a zero serv_addr
	memset(&serv_adr, 0, sizeof(serv_adr));
	serv_adr.sin_family = AF_INET;
	//El fica IP local 
	serv_adr.sin_addr.s_addr = htonl(INADDR_ANY); 
	//Indiquem per quin port entra el client
	serv_adr.sin_port = htons(9080);
	if (bind(sock_listen, (struct sockaddr *) &serv_adr, sizeof(serv_adr)) < 0)
		printf("Error al bind\n");
	 //Limitem el nombre de connexions pendents
	if (listen(sock_listen, 10) < 0)
		printf("Error en el listen\n");
	 //acceptem connexio d'un client￧
	int i=0;
	pthread_t thread[100];
	for(;;)
	{
		printf("Preparado para recibir la peticion\n");
		sock_conn = accept(sock_listen, NULL, NULL);
		printf("He recibido la conexion \n");
		
		sockets[i] = sock_conn;
		//sock_conn es el socket que usaremos en este cliente
		 //Crear thread y decirle lo que tiene que hacer
		
		pthread_create (&thread[i], NULL, AtenderCliente,&sockets[i]);
		i++;
	}
}
}

