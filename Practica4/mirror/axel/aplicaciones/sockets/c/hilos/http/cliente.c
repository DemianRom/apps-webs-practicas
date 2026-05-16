#include <stdio.h>
#include <stdlib.h>
#include <netinet/in.h>
#include <sys/socket.h>
#include <arpa/inet.h>
#include <unistd.h>
#include <netdb.h>

/* HTTP server to connect to */
#define HTTP_HOST "148.204.58.221"
/* Port to connect to, as string */
#define HTTP_PORT "80"
/* HTTP path to request */
#define HTTP_PATH "/axel/aplicaciones/sockets/java/"


#define SSTRLEN(s) (sizeof(s) - 1)
#define CHECK(r) { if (r == -1) { printf("Error: " #r "\n"); exit(1); } }

#define REQUEST "GET " HTTP_PATH " HTTP/1.0\r\nHost: " HTTP_HOST "\r\n\r\n"

static char response[1024];

int main(void)
{
	static struct addrinfo hints;
	struct addrinfo *res,*p;
	int st, sock;

	printf("Preparing HTTP GET request for http://" HTTP_HOST
	       ":" HTTP_PORT HTTP_PATH "\n");

	hints.ai_family = AF_INET;
	hints.ai_socktype = SOCK_STREAM;
	st = getaddrinfo(HTTP_HOST, HTTP_PORT, &hints, &res);
	printf("getaddrinfo status: %d\n", st);

	if (st != 0) {
		printf("Unable to resolve address, quitting\n");
		return 0;
	}//if

      for (; res; res = res->ai_next) {

        sock = socket(res->ai_family, res->ai_socktype, res->ai_protocol);
	if(sock==-1){
           perror("error en la funcion socket() \n");
           continue;
        }//if
        printf("socket creado con descriptor: %d\n",sock);
	//CHECK(sock);
	printf("sock = %d\n", sock);

	if(connect(sock, res->ai_addr, res->ai_addrlen)==-1){
           perror("Error en la función connect() \n");
           close(sock);
           continue;
        }//if
        break;
       }//for
       
       if(res == NULL){
        perror("error al conectar con el servidor..\n");
        return 2;
       }//if
	CHECK(send(sock, REQUEST, SSTRLEN(REQUEST), 0));

	printf("Response:\n\n");

	while (1) {
		int len = recv(sock, response, sizeof(response) - 1, 0);

		if (len < 0) {
			printf("Error reading response\n");
			return 0;
		}

		if (len == 0) {
			break;
		}

		response[len] = 0;
		printf("%s", response);
	}

	printf("\n");

	(void)close(sock);
	return 0;
}
