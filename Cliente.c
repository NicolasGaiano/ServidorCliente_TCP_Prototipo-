#include <unistd.h>
#include <netinet/in.h>
#include <arpa/inet.h>
#include <sys/types.h>
#include <sys/socket.h>
#include <stdlib.h>
#include <string.h>
#include <stdio.h>
#define MAXBUF 1024
#define SIZE 1460  //tam paquete maximo (en Bytes)

/////////////////////////////////////////////////////////////////////////////////////////

void enviar_archivo (FILE *fp, int sock1); //función enviar archivo

int archivo_name (char *cadena); //función convertir la ruta completa del archivo en el nombre de
                                 //archivo

struct infoarchivo{//estructura para mandar los datos del archivo
	unsigned long tamanio;
	unsigned long paquetes;
	char nombrearchivo[512];
}arch;

//////////////////////////////////////////////////////////////////////////////////////////

/////////////////////////<<CLIENTE TCP/IP>>///////////////////////////////////////////////

int main(int argc, char *argv[]){
  unsigned int         socket_cliente;     // Descriptor del socket
  struct sockaddr_in   client_addr;        // Estructura con los datos del cliente
  char                 buf_enviar[1500];   // Buffer de 1500 bytes para los datos a enviar
  char                 buf_recibido[1500]; // Buffer de 1500 bytes para los datos a recibir
  char                 ipserver[16];
  char                 ruta_archivo[1500];    // direccion del archivo a transmitir
  int                  bytesrecibidos,bytesaenviar, bytestransmitidos; // Contadores
  int                  conectado;          // variable auxiliar
  int                  packs,resto;        // variables auxiliares para los paquetes
  int                  tam_estructura;     // Tamaño de las estructuras
  unsigned char        *p1;     //puntero para manipular los buffers
  int                  cTAM;    //variable para almacenar el tamaño del archivo a recibir
  int                  enviar;  //flag habilitar envio
  int                  flagmenu=0;//flag de menu de envios multiples
  char                 urespuesta[2];//cadena de caracteres para respuesta si/no


/////////////INGRESO DE LA IP////////////////////////////////////////////////////////////
  if (argc!=2){
    printf("\n****************************************************\n");
    printf("[+]Debe ingresar la ip del servidor de la forma:\n  clienteTCP www.xxx.yyy.zzz\n");
    printf("****************************************************\n");
    exit(0);
  }
  strncpy(ipserver,argv[1],16); //se almacena la IP ingresada en el arreglo ipserver

//////////////////////////////////////////////////////////////////////////////////////////


////////////////INICIO DEL SOCKET/////////////////////////////////////////////////////////
  //estructura con los datos del servidor. Almacena(y transforma) IP y puerto
  struct sockaddr_in   estructura_servidor;
  estructura_servidor.sin_family      = AF_INET;         // Familia TCP/IP
  estructura_servidor.sin_port        = htons(33333);    // Número de Port


do{//BUCLE_PARA_MULTIPLES_ENVIOS/////////////////////////////////////////////////////////////////


//////////////////////CREACION DEL SOCKET DE FLUJO///////////////////////////////////////
  // "0" selecciona el protocolo automáticamente
  socket_cliente = socket(AF_INET, SOCK_STREAM, 0);
  if(socket_cliente==-1){
    printf("[-]Fallo la creacion del socket\n");
    exit(1);
  }
 // printf("Cree el descriptor del socket %d\n",socket_cliente);//debug


  // en caso de que la IP ingresada no sea la del servidor, finaliza el programa
  if(inet_aton(ipserver, &estructura_servidor.sin_addr)==0) {//se carga la estructura con la
                                                             //dirección IP del servidor
    printf ("[-]La dirección ingresada no es una dirección IP válida.\n");
    exit(2);
  }
////////////////////////////////////////////////////////////////////////////////////////////



//////////////////CONEXIÓN CON EL SERVIDOR//////////////////////////////////////////////////
  tam_estructura = sizeof(estructura_servidor);
  conectado=connect(socket_cliente, (struct sockaddr *)&estructura_servidor, tam_estructura);
  if (conectado==-1){
    printf("[-]Error de conexion\n");
    exit(3);
   }
  printf ("\n<<ESTABLECIENDO CONEXION>>\n");


///////////////ESTRUCTURA DEL ARCHIVO (ruta->nombre)///////////////////////////////////////////
  FILE *fich; //puntero para manipular el archivo
  // se Ingresa la ruta del archivo que se desea enviar
  memset(ruta_archivo,0x00,MAXBUF);
  printf("[+]Ingrese la ruta completa del archivo que se desea enviar...\n");
  scanf("%s",ruta_archivo);

  //obtencion del nombre del archivo a enviar a partir de la ruta
  fich=fopen(ruta_archivo, "r");
     if (fich == NULL){
    	   printf ("\n[-]ERROR no se puede leer el archivo\n");
    	   exit(4);
  	   }

  strcpy(arch.nombrearchivo,ruta_archivo);
  archivo_name (arch.nombrearchivo);//función de conversión

//  printf("La direccion expecificada es %s\n",ruta_archivo);debug

  //tamaño del archivo
  //fseek desplaza la posición actual de lectura/escritura del fichero a otro punto
  fseek(fich, 0L, SEEK_END);
  //ftell devuelve la posición actual(en bytes), en este caso será el final (debido al seek_end),
  //es decir, el tamaño total
  arch.tamanio=ftell(fich);

////////////cálculo de la cantidad de paquetes a enviar///////////////////////////////////////////
  packs=arch.tamanio/1460;
  resto=arch.tamanio%1460;   //en resto se almacena el tam del el último paquete
    if(resto==0){
       arch.paquetes=packs;  //si el resto da cero significa que se enviaran todos paquetes de 1460
                             //bytes
    }
    else{
       arch.paquetes=packs+1;//si el resto es distinto de cero voy a enviar un paquete adicional
                             //para los bits restantes
    }

////////////////info del envío////////////////////////////////////////////////////////////////
  printf("\n****************************************************\n");
  printf("[*]Se enviara el archivo: %s\n",arch.nombrearchivo);
  printf("[*]El tamaño del archivo es %lu Bytes\n", arch.tamanio);
  printf("[*]La cantidad de paquetes a enviar son %lu\n", arch.paquetes);
  printf("****************************************************\n");

///////////////////////////////////////////////////////////////////////////////////////////////


////////////////////////ENVÍO DEL PRIMER PAQUETE (estructura infoarchivo)//////////////////////

    //creación del buffer para envío
    unsigned char buf_primero[sizeof(struct infoarchivo)];
    p1=buf_primero;

    //Serialización de la estructura para su envío al servidor
    memcpy(buf_primero, &arch.tamanio, sizeof(arch.tamanio));
    p1=p1 + sizeof(arch.tamanio);
    memcpy(p1, &arch.paquetes, sizeof(arch.paquetes));
    p1=p1+sizeof(arch.paquetes);
    memcpy(p1, &arch.nombrearchivo, sizeof(arch.nombrearchivo));

    cTAM=arch.tamanio;//se guarda el tamaño original para luego comparar

    bytestransmitidos=send(socket_cliente, buf_primero, sizeof(struct infoarchivo),0);

    //deserialización de la estructura reenviada por el servidor
    bytesrecibidos=recv(socket_cliente, buf_recibido, sizeof(buf_recibido), 0);
    p1=buf_recibido;
    memcpy(&arch.tamanio, buf_recibido, sizeof(arch.tamanio));
    p1=p1 + sizeof(arch.tamanio);
    memcpy(&arch.paquetes, p1, sizeof(arch.paquetes));
    p1=p1+sizeof(arch.paquetes);
    memcpy(&arch.nombrearchivo, p1, sizeof(arch.nombrearchivo));


/////////////////COMPARACIÓN DE PAQUETE INICIAL/////////////////////////////////////////

if (arch.tamanio==cTAM){
    enviar=1;//esta condición se cumple si el servidor responde con el paquete inicial
}
////////////////////////////////////////////////////////////////////////////////////////

///////////////////////////////ENVÍO DEL ARCHIVO////////////////////////////////////////
  if(enviar==1){


	printf ("[+]enviando archivo...\n");
	FILE *fp;
  	char *filename = ruta_archivo;

  	fp = fopen(filename, "r"); //se abre al archivo a enviar para su lectura
  		if (fp == NULL) {
    		    printf ("\n[-]ERROR no se puede leer el archivo\n");
    		    exit(5);
  		}

  	enviar_archivo(fp, socket_cliente);// enviar
  	printf("\n<<Archivo enviado>>\n");
	enviar =0;
   }




//////////////////ENVÍO DEL PAQUETE FINAL////////////////////////////////////////////////////
    p1=buf_primero;
    /*Serializo la estructura para mandarla al cliente*/
    memcpy(buf_primero, &arch.tamanio, sizeof(arch.tamanio));
    p1=p1 + sizeof(arch.tamanio);
    memcpy(p1, &arch.paquetes, sizeof(arch.paquetes));
    p1=p1+sizeof(arch.paquetes);
    memcpy(p1, &arch.nombrearchivo, sizeof(arch.nombrearchivo));

    bytestransmitidos=send(socket_cliente, buf_primero, sizeof(struct infoarchivo),0);

    close(socket_cliente); //se cierra socket cliente
/////////////////////////////////////////////////////////////////////////////////////////////

//////////////////////MENÚ (multiples envios)////////////////////////////////////////////////
     flagmenu=0;
	do{
	  printf("\n****************************************************\n");
	  printf("\n[+]Desea enviar otro archivo? si/no\n");
	  scanf("%s",urespuesta);
	  printf("****************************************************\n");

 	  if (strcmp(urespuesta,"si")==0){
              enviar=1;
	      flagmenu=1;
	  }
          if (strcmp(urespuesta,"no")==0){
              enviar=0;
              flagmenu=1;
          }
        }while (flagmenu==0);
/////////////////////////////////////////////////////////////

}while(enviar==1);//Fin_de_bucle_para_multiples_envios///////////////////////////////////////////
    exit(6);
} // fin del programa


//////////////////////FUNCIÓN OBTENER EL NOMBRE DE ARCHIVO///////////////////////
int archivo_name (char *cadena){
    int i,j,flag;
    for(i=0;i<strlen(cadena);i++)
       {
        if(cadena[i]== '/' ){
            flag=i+1; // se almacena la posición de la última barra
        }
    }
    for(j=flag; j<=strlen(cadena); j++){
        cadena[j-flag]=cadena[j];// se ubica el nombre del archivo en la nueva cadena
    }
}
//////////////////////////////////////////////////////////////////////////////////////

//////////////////////FUNCIÓN ENVIAR ARCHIVO//////////////////////////////////////////
void enviar_archivo (FILE *fp, int sock1){
  char paquete[SIZE] = {0};//arreglo de 1460 char
  while(fgets(paquete, SIZE, fp) != NULL) { //se carga el arreglo con un paquete del archivo
      if (send(sock1, paquete, sizeof(paquete), 0) == -1) {//se envía el paquete
          printf ("ERROR de envio");
          exit(7);
      }
    bzero(paquete, SIZE);// se pone a cero el arreglo para cargar el siguiente paquete

  }
}
/////////////////////////////////////////////////////////////////////////////////////////////
