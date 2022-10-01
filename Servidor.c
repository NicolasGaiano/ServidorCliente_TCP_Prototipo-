#include <unistd.h>
#include <netinet/in.h>
#include <arpa/inet.h>
#include <sys/types.h>
#include <sys/socket.h>
#include <stdlib.h>
#include <string.h>
#include <stdio.h>
#include <signal.h>
#include <sys/wait.h>
#include<time.h>
#define SIZE 1460 //tam paquete máximo (en Bytes)

/////////////////////////////////////////////////////////////////////////////////////////////////
void recibir_archivo(int sock1, char *nombre); // función recibir archivo

void registro(char *archivo_recib,char *ip, unsigned short int puerto);//función de registro de
                                                                       //actividad

int terminar=0;// variable global para terminar servidor padre mediante SIGHUB

void handler(int sig);

struct infoarchivo{ //estructura para recepción de los datos del archivo
	unsigned long tamanio;
	unsigned long paquetes;
	char nombrearchivo[512];
}arch;
/////////////////////////////////////////////////////////////////////////////////////////////////

//////////////////////////////<<SERVIDORTCP/IP>>/////////////////////////////////////////////////
int main(int argc, char *argv[])
{
  unsigned int         socket_servidor;       // Descriptor del socket
  unsigned int         socket_nuevo;          // Descriptor del socket de conexión (hijo)
  int                  tam_estructura;        // Tamaño de las estructuras
  char                 buf_enviar[1500];      // Buffer de 1500 bytes para los datos a enviar
  char                 buf_recibido[1500];    // Buffer de 1500 bytes para los datos a recibir
  int                  bytesrecibidos, bytesaenviar, bytestransmitidos;  // Contadores
  int                  i=0;                   //contador de mensajes
  unsigned char        *p1;             // puntero para manejar la serialización/deserialización
  pid_t pid_n;
  int                  FIN=0;          // flag de finalización
  int                  cTAM;           // variable para almacenar el tamaño del archivo a recibir

///////////////INSTALACIÓN DEL MANEJADOR DE SEÑALES////////////////////////
  signal(SIGHUP,handler);// la señal 1 finaliza el servidor padre
  // una vez recibida se queda a la espera de una última atención
/////////////////////////////////////////////////////////////////////////



////////////////////////INICIO DEL SOCKET////////////////////////////////////////
  //estructura con los datos del servidor. Almacena(y transforma) IP y puerto

  struct sockaddr_in   estructura_servidor;
  estructura_servidor.sin_family      = AF_INET;            // Familia TCP/IP
  estructura_servidor.sin_port        = htons(33333);       // Número de puerto
  estructura_servidor.sin_addr.s_addr = htonl(INADDR_ANY);  // INADDR_ANY = cualquier dirección
                                                            //IP

//////////////////////CREACIÓN DEL SOCKET DE FLUJO//////////////////////////////
  //"0" selecciona el protocolo automáticamente
  socket_servidor = socket(AF_INET, SOCK_STREAM, 0);
  if (socket_servidor==-1){
      printf("[-]Fallo la creacion del socket\n");
      exit(0);
  }
 // printf("Cree el descriptor del socket %d\n",socket_servidor);//DEBUG

  //se asocia el socket a un puerto local. El núcleo usa el número de puerto para asociar los
  //paquetes entrantes
  //con un descriptor de socket de un cierto proceso. Esto es solo necesario si quiero recibir
  //paquetes
  bind(socket_servidor, (struct sockaddr *)&estructura_servidor, sizeof(estructura_servidor));

  /*una vez asociado el socket al puerto, se comienza a escuchar de peticiones. La cola de peticiones puede ser hasta 5*/
  listen(socket_servidor, 5);

	printf("\n[+]Escuchando puerto 33333...\n");

  struct sockaddr_in   estructura_cliente;     // Estructura con los datos del cliente
  struct in_addr       client_ip_addr;         // IP del cliente
  tam_estructura = sizeof(estructura_cliente);


 while(!terminar){

       //mediante accept, se acepta una de esas peticiones y se crea un nuevo socket(hijo) para
       //tratarla
 socket_nuevo = accept(socket_servidor, (struct sockaddr *)&estructura_cliente, &tam_estructura);
       if (socket_nuevo==-1){
           printf("\n[-]Fallo la creacion del socket\n");
           exit(1);
       }

       //información de los procesos/
       printf("\n****************************************************\n");
 printf("[*]SERVIDOR PADRE  PID:%d \n[*]IP del cliente:%s \n[*]Port del cliente:%hu \n",getpid(),
                inet_ntoa(estructura_cliente.sin_addr), ntohs(estructura_cliente.sin_port));

       printf("[*]para finalizar el SERVIDOR PADRE enviele SIGHUP\n");
       printf("****************************************************\n");


////////////////PROCESO HIJO (atención del cliente)//////////////////////////////////////////
   if((pid_n=fork())==0){
      do{
            // el proceso hijo le contestara al cliente mediante un socket nuevo
            //el proceso hijo no necesita el socket del servidor padre -> se cierra
            close (socket_servidor);

            //información del hijo*/
            printf("\n****************************************************\n");
            printf("SERVIDOR HIJO PID:%d atendiendo nueva peticion \n",getpid());
            printf("****************************************************\n");

////////////RECEPCIÓN Y ENVIO DEL PRIMER PAQUETE LA ESTRUCTURA INFOARCHIVO//////////////////////

     //se recibe la estructura,bytesrecibidos almacena el número de bytes que ocupa la estructura
            bytesrecibidos=recv(socket_nuevo, buf_recibido, sizeof(buf_recibido), 0);
                if (bytesrecibidos==-1){
                    printf("[-]No se pudo recibir el mensaje\n");
                    exit(2);
                    }
            //La estructura tiene 530 bytes, 512 char, 18 unsigned long
                if(bytesrecibidos==sizeof(struct infoarchivo)){
                    // se reenvía el paquete
                    send(socket_nuevo, buf_recibido, bytesrecibidos,0);
                }
                else{
                 //si la cantidad de bytes recibidos es distinta, significa que hubo un problema
                    // se a comunica el problema al cliente mediante el nombrearchivo
                    arch.tamanio=0;
                    arch.paquetes=0;
                    sprintf(arch.nombrearchivo,"[-]Se recibieron menos bytes de lo esperado.");
                    //serialización
                    p1=buf_recibido;
                    memcpy(buf_recibido, &arch.tamanio, sizeof(arch.tamanio));
                    p1=p1 + sizeof(arch.tamanio);
                    memcpy(p1, &arch.paquetes, sizeof(arch.paquetes));
                    p1=p1+sizeof(arch.paquetes);
                    memcpy(p1, &arch.nombrearchivo, sizeof(arch.nombrearchivo));
                    printf("%s\n", p1);
                    send(socket_nuevo, buf_recibido, bytesrecibidos,0);
                    printf("[-]Enviado el problema al cliente.\n");
                    }

            //deserialización de la estructura recibida
            p1=buf_recibido;
            memcpy(&arch.tamanio, buf_recibido, sizeof(arch.tamanio));
            p1=p1 + sizeof(arch.tamanio);
            memcpy(&arch.paquetes, p1, sizeof(arch.paquetes));
            p1=p1+sizeof(arch.paquetes);
            memcpy(&arch.nombrearchivo, p1, sizeof(arch.nombrearchivo));
            printf("[*]El nombre del archivo a enviar es %s\n",arch.nombrearchivo);
            printf("[*]El tamaño del archivo es %lu Bytes\n", arch.tamanio);
            printf("[*]La cantidad de paquetes a recibir son %lu\n", arch.paquetes);

            cTAM=arch.tamanio;//se guarda el tamaño original


///////////////////////////////////RECEPCIÓN DEL ARCHIVO///////////////////////////////////////

  recibir_archivo(socket_nuevo, arch.nombrearchivo);

  registro(arch.nombrearchivo,inet_ntoa(estructura_cliente.sin_addr), ntohs(estructura_cliente.sin_port));

//////////////////recepción del paquete final//////////////////////////////////////////////////
  bytesrecibidos=recv(socket_nuevo, buf_recibido, sizeof(buf_recibido), 0);

            //deserialización de la estructura recibida
            p1=buf_recibido;
            memcpy(&arch.tamanio, buf_recibido, sizeof(arch.tamanio));
            p1=p1 + sizeof(arch.tamanio);
            memcpy(&arch.paquetes, p1, sizeof(arch.paquetes));
            p1=p1+sizeof(arch.paquetes);
            memcpy(&arch.nombrearchivo, p1, sizeof(arch.nombrearchivo));

///////////////////////////////////////////////////////////////////////////////////////////////

/////////////////COMPARACIÓN DE PAQUETE FINAL/////////////////////////////////////////
	if (arch.tamanio==cTAM){
 	   FIN=1;//esta condición se cumple si el cliente envía el paquete inicial
  	  printf("\n<<Archivo RECIBIDO>>\nse guardo como %s\n",arch.nombrearchivo);
	}
	else{
 	printf ("\n[-]ERROR de archivo\n");
		FIN=0; // finalizar de todos modos (podría tomarse otra acción)
	}
////////////////////////////////////////////////////////////////////////////////////////

   } while (FIN==0);
           // se cierra socket_nuevo y termina el hijo
            close(socket_nuevo);
            exit(3);

/////////////FINALIZA ATENCIÓN AL CLIENTE///////////////////////////////////////////////

 }
 else{
        // proceso padre
        //printf("Soy el padre %d, recibi un pedido de conexión, la derive a mi hijo %d\n", getpid(),pid_n);//debug
        close(socket_nuevo);  //se cierra(libera e impide envío/recepción) socket_nuevo.
        }
    //el SERVIDOR PADRE continúa escuchando peticiones en el puerto 33333
    }

///////////FINALIZACIÓN//////////////////////////////////////////////////////////////////
  wait(NULL);   //eliminó procesos zombies
  close(socket_servidor);   //cierro el servidor
  return(0);
}

////////////////////////////MANEJADOR DE SEÑALES////////////////////////////////////////////////
void handler(int sig)
{
	if (sig==SIGHUP)
	{
	terminar=1;
        printf("\n****************************************************\n");
	printf("[+]señal HUP recibida \n<<cuando se establezca la proxima conexión el servidor terminará>>\n");
        printf("****************************************************\n");
	}
}
/////////////////////////////////////////////////////////////////////////////////////////////////

//////////////////////FUNCIÓN RECIBIR ARCHIVO////////////////////////////////////////////////////
void recibir_archivo(int sock1, char *nombre){
  int n;
  FILE *fp;
  char *recibido = nombre;
  char paquete[SIZE]; //arreglo de char para recibir el paquete

  fp = fopen(recibido, "w");// se crea el archivo de recepción
  while (1) {
    n = recv(sock1, paquete, SIZE, 0); //se recibe un paquete
    if (n <= 0){ //cuando concluye el archivo la recepción se termina
      break;
      return;
    }
    fprintf(fp, "%s", paquete); // el paquete recibido se guarda en el archivo
    bzero(paquete, SIZE);// se pone el arreglo de char a cero para recibir el siguiente paquete
  }
  return;
}
////////////////////////////////////////////////////////////////////////////////////////////////

//////////////////////FUNCIÓN REGISTRO///////////////////////////////////////////////////
void registro(char *archivo_recib,char *ip, unsigned short int puerto){
  FILE *fp;
  char *registro = "servidor.log";
  fp = fopen(registro, "a");//a indica no sobre escribir y crear si no existe
  fprintf(fp, "Se recibio el archivo %s", archivo_recib);
  fprintf(fp, " del cliente con IP: %s", ip);
  fprintf(fp, " y puerto:%hu. ", puerto);

  time_t t;// obtencion de fecha y hora
  struct tm *tm;
  char fechayhora[100];
  t=time(NULL);
  tm=localtime(&t);
  strftime(fechayhora, 100, "el dia %d/%m/%Y  a las %H:%M:%S hs", tm);

  fprintf(fp, "%s \n", fechayhora);
  fprintf(fp, "-------------------------------------------------------------");
  fprintf(fp, "-------------------------------------------------------------\n ");

return;
}
////////////////////////////////////////////////////////////////////////////////////////////////
