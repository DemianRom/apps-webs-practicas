#include <sys/types.h>
#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <string.h>
#include <sys/socket.h>
#include <netdb.h>
#include <arpa/inet.h>
#include <pthread.h>


void* lee (void* arg)
{
int n, buf_size=65000,s;
int cd = *((int*) arg);
char buf1[buf_size];
struct sockaddr_storage emisor;
socklen_t emisor_len, ctam;
ssize_t nread;

 for (;;) {
        bzero(buf1, sizeof(buf1));
        emisor_len = sizeof(struct sockaddr_storage);
        n = recvfrom(cd, buf1, buf_size, 0,(struct sockaddr *) &emisor, &emisor_len);
        if (n == -1)
            continue;               /* Ignore failed request */

       char host[NI_MAXHOST], service[NI_MAXSERV];

       s = getnameinfo((struct sockaddr *) &emisor,emisor_len, host, NI_MAXHOST,service, NI_MAXSERV, NI_NUMERICHOST | NI_NUMERICSERV);
       if (s == 0)
            printf(" %d bytes recibidos desde %s:%s\n datos:%s\n",n, host, service,buf1);
        else
            fprintf(stderr, "getnameinfo: %s\n", gai_strerror(s));

}//for
//return (void *)res;
}//lee


void* escribe (void* arg)
{
int n, buf_size=65000,s;
int cd = *((int*) arg);
char buf1[buf_size];
struct sockaddr_storage emisor;
socklen_t emisor_len, ctam;
ssize_t nread;
struct addrinfo dir;
struct addrinfo *result, *rp;
memset(&dir, 0, sizeof(struct addrinfo));
dir.ai_family = AF_UNSPEC;    /* Allow IPv4 or IPv6 */
    dir.ai_socktype = SOCK_DGRAM; /* Datagram socket */
    dir.ai_protocol = 0;          /* Any protocol */
    dir.ai_canonname = NULL;
    dir.ai_addr = NULL;
    dir.ai_next = NULL;

   s = getaddrinfo("127.0.0.1", "1234", &dir, &result);
    if (s != 0) {
        fprintf(stderr, "getaddrinfo: %s\n", gai_strerror(s));
        exit(EXIT_FAILURE);
    }//if
    bzero(buf1, sizeof(buf1));
    emisor_len = sizeof(struct sockaddr_storage);
    printf("\nEscribe un mensaje, posteriormente presiona <enter>:\n");
    char *linea;
    linea=NULL;
    size_t tam; 
    tam=0;

    while((n=getline(&linea,&tam,stdin))!=-1){
        printf("strlen da %d \n",(int)strlen(linea));
        if(strlen(linea)==1){
           printf("fflush\n");
          fflush(stdin);
          continue;
        }//if
        linea[strlen(linea)-1]='\0';
       if(strncasecmp(linea,"SALIR",5)==0){
	printf("escribio SALIR\n");
        //n1= send(cd,linea,strlen(linea),0);
        //fflush(f);
	//fclose(f);
	close(cd);
        free(linea);
        //linea=NULL;
        tam=0;
	exit(0);
	} else {
        printf("Preparado para enviar %d bytes, con el mensaje: %s\n",(int)strlen(linea),linea);
	//n1= send(cd,linea,strlen(linea),0);
        if (sendto(cd, linea, strlen(linea), 0, (struct sockaddr *)result->ai_addr, result->ai_addrlen)<=0)
        perror("sendto()");  
	printf("Se envio el mensaje-> %s\n",linea);
 	//fflush(f);
        memset(linea,'\0',tam);
        }//else

        fflush(stdin);
        //free(linea);
        //linea=NULL;
       // tam=0;
//       free(linea);
}//while
//free(linea);
tam=0;
}//escribe


int main(int argc, char *argv[])
{
    struct addrinfo dir;
    struct addrinfo *result, *rp;
    int cd, s,v=1;
    char hbuf[NI_MAXHOST], sbuf[NI_MAXSERV];
    struct sockaddr_storage peer_addr;
    socklen_t peer_addr_len, ctam;
    ssize_t nread;
    int buf_size = 65000;
    char buf[buf_size];

   memset(&dir, 0, sizeof(struct addrinfo));
    dir.ai_family = AF_INET6;    /* Allow IPv4 or IPv6 */
    dir.ai_socktype = SOCK_DGRAM; /* Datagram socket */
    dir.ai_flags = AI_PASSIVE;    /* For wildcard IP address */
    dir.ai_protocol = 0;          /* Any protocol */
    dir.ai_canonname = NULL;
    dir.ai_addr = NULL;
    dir.ai_next = NULL;

   s = getaddrinfo(NULL, "1234", &dir, &result);
    if (s != 0) {
        fprintf(stderr, "getaddrinfo: %s\n", gai_strerror(s));
        exit(EXIT_FAILURE);
    }


   for (rp = result; rp != NULL; rp = rp->ai_next) {
        cd = socket(rp->ai_family, rp->ai_socktype,
                rp->ai_protocol);
        if (cd == -1)
            continue;

	int op = 0;
        int r = setsockopt(cd, IPPROTO_IPV6, IPV6_V6ONLY, &op, sizeof(op));
	if (setsockopt(cd, SOL_SOCKET, SO_REUSEADDR, &v,sizeof(int)) == -1) {
            perror("setsockopt");
            exit(1);
        }//if

	
       if (bind(cd, rp->ai_addr, rp->ai_addrlen) == 0)
	  break;
                  /* Success */

       //close(cd);
    }//for

   if (rp == NULL) {               /* No address succeeded */
        fprintf(stderr, "Could not bind\n");
        exit(EXIT_FAILURE);
    }//if

   freeaddrinfo(result);           /* No longer needed */

   printf("Socket iniciado, creando hilos.. \n");
   pthread_t t_escribe, t_lee;
   pthread_create(&t_escribe, NULL, &escribe,&cd);
   pthread_create(&t_lee, NULL, &lee,&cd);
   pthread_join(t_escribe,NULL);
   pthread_join(t_lee,NULL);


}//main
