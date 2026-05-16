#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <arpa/inet.h>
#include <netinet/tcp.h> //TCP_NODELAY

#define PORT 12345
//#define BUFFER_SIZE 4096
#define BUFFER_SIZE 60000

int main(int argc, char *argv[]) {
    if (argc < 2) {
        fprintf(stderr, "Uso: %s <archivo.mp3>\n", argv[0]);
        exit(1);
    }

    int sockfd, newsockfd;
    struct sockaddr_in server_addr, client_addr;
    socklen_t client_len = sizeof(client_addr);
    char buffer[BUFFER_SIZE];
    ssize_t bytes_read;

    // Crear socket
    if ((sockfd = socket(AF_INET, SOCK_STREAM, 0)) < 0) {
        perror("Error al crear el socket");
        exit(1);
    }
    char op = 1;
    int opcion = setsockopt(sockfd,SOL_SOCKET,SO_REUSEADDR,&op,sizeof(op));
    
    if(op<0){
     perror("error al habilitar SO_REUSEADDR\n");
    }//if
    // Configurar la dirección del servidor
    memset(&server_addr, 0, sizeof(server_addr));
    server_addr.sin_family = AF_INET;
    server_addr.sin_addr.s_addr = INADDR_ANY;
    server_addr.sin_port = htons(PORT);

    // Enlazar el socket
    if (bind(sockfd, (struct sockaddr *)&server_addr, sizeof(server_addr)) < 0) {
        perror("Error al enlazar el socket");
        close(sockfd);
        exit(1);
    }

    // Escuchar conexiones entrantes
    if (listen(sockfd, 5) < 0) {
        perror("Error al escuchar en el socket");
        close(sockfd);
        exit(1);
    }

    printf("Servidor escuchando en el puerto %d...\n", PORT);
   for(;;){
    // Aceptar una conexión entrante
    if ((newsockfd = accept(sockfd, (struct sockaddr *)&client_addr, &client_len)) < 0) {
        perror("Error al aceptar la conexión");
        close(sockfd);
        exit(1);
    }
    int v2 =1;
   int opcion2 = setsockopt(sockfd,SOL_SOCKET,TCP_NODELAY,&v2,sizeof(v2));
    printf("Conexión aceptada desde %s:%d\n", inet_ntoa(client_addr.sin_addr), ntohs(client_addr.sin_port));

    // Abrir el archivo MP3
    FILE *mp3_file = fopen(argv[1], "rb");
    if (!mp3_file) {
        perror("Error al abrir el archivo MP3");
        close(newsockfd);
        close(sockfd);
        exit(1);
    }

    // Enviar el archivo MP3 al cliente
    while ((bytes_read = fread(buffer, 1, BUFFER_SIZE, mp3_file)) > 0) {
        if (send(newsockfd, buffer, bytes_read, 0) < 0) {
            perror("Error al enviar datos");
            break;
        }
    }

    printf("Archivo MP3 enviado.\n");

    // Cerrar todo
    fclose(mp3_file);
    close(newsockfd);
    }//for
    close(sockfd);

    return 0;
}
