#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <arpa/inet.h>

#define PORT 5000
#define BUFFER_SIZE 40000//60000

int main(int argc, char *argv[]) {
    int server_fd, client_fd;
    struct sockaddr_in server_addr, client_addr;
    socklen_t client_len = sizeof(client_addr);
    char buffer[BUFFER_SIZE];
    FILE *file;
    ssize_t bytes_read;

    // Crear el socket
    if ((server_fd = socket(AF_INET, SOCK_STREAM, 0)) == -1) {
        perror("Error al crear el socket");
        exit(EXIT_FAILURE);
    }

    // Configurar la dirección del servidor
    server_addr.sin_family = AF_INET;
    server_addr.sin_addr.s_addr = INADDR_ANY;
    server_addr.sin_port = htons(PORT);
    char op = 1;
    int opcion = setsockopt(server_fd,SOL_SOCKET,SO_REUSEADDR,&op,sizeof(op));
    
    // Enlazar el socket a la dirección
    if (bind(server_fd, (struct sockaddr *)&server_addr, sizeof(server_addr)) == -1) {
        perror("Error al enlazar el socket");
        close(server_fd);
        exit(EXIT_FAILURE);
    }

    // Escuchar conexiones entrantes
    if (listen(server_fd, 1) == -1) {
        perror("Error al escuchar conexiones");
        close(server_fd);
        exit(EXIT_FAILURE);
    }

    printf("Servidor esperando conexiones en el puerto %d...\n", PORT);
    for(;;){
    // Aceptar una conexión entrante
    if ((client_fd = accept(server_fd, (struct sockaddr *)&client_addr, &client_len)) == -1) {
        perror("Error al aceptar la conexión");
        close(server_fd);
        continue; //exit(EXIT_FAILURE);
    }//if

    printf("Cliente conectado.\n");

    // Abrir el archivo MP3
    file = fopen(argv[1], "rb");
    if (!file) {
        perror("Error al abrir el archivo");
        close(client_fd);
        close(server_fd);
        exit(EXIT_FAILURE);
    }//if

    // Leer y enviar el archivo
    while ((bytes_read = fread(buffer, 1, BUFFER_SIZE, file)) > 0) {
        if (send(client_fd, buffer, bytes_read, 0) == -1) {
            perror("Error al enviar datos");
            fclose(file);
            close(client_fd);
            close(server_fd);
            exit(EXIT_FAILURE);
        }//if
        sleep(0.1);
    }//while

    printf("Archivo enviado.\n");

    // Cerrar todo
    fclose(file);
    close(client_fd);
    }//for
    close(server_fd);

    return 0;
}
