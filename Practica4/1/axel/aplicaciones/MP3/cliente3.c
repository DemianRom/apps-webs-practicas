#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <arpa/inet.h>
#include <mpg123.h>
#include <ao/ao.h>

#define BITS 8
#define PORT 5000
#define BUFFER_SIZE 60000

int main() {
    int client_fd;
    struct sockaddr_in server_addr;
    char buffer[BUFFER_SIZE];
    FILE *file;
    ssize_t bytes_received;

    // Crear el socket
    if ((client_fd = socket(AF_INET, SOCK_STREAM, 0)) == -1) {
        perror("Error al crear el socket");
        exit(EXIT_FAILURE);
    }

    // Configurar la dirección del servidor
    server_addr.sin_family = AF_INET;
    server_addr.sin_port = htons(PORT);
    if (inet_pton(AF_INET, "127.0.0.1", &server_addr.sin_addr) <= 0) {
        perror("Error al configurar la dirección del servidor");
        close(client_fd);
        exit(EXIT_FAILURE);
    }

    // Conectar al servidor
    if (connect(client_fd, (struct sockaddr *)&server_addr, sizeof(server_addr)) == -1) {
        perror("Error al conectar al servidor");
        close(client_fd);
        exit(EXIT_FAILURE);
    }

    printf("Conectado al servidor.\n");

    // Abrir el archivo para escribir
    file = fopen("tmp.mp3", "wb");
    if (!file) {
        perror("Error al abrir el archivo");
        close(client_fd);
        exit(EXIT_FAILURE);
    }

    // Recibir el archivo
    while ((bytes_received = recv(client_fd, buffer, BUFFER_SIZE, 0)) > 0) {
        fwrite(buffer, 1, bytes_received, file);
    }

    printf("Archivo recibido.\n");

    // Cerrar todo
    fclose(file);
    close(client_fd);

    // Reproducir el archivo MP3 recibido
    //system("mpg123 tmp.mp3");
    
    /***********************************/
 mpg123_handle *mh;
 unsigned char *buffer2; 
 size_t buffer_size; 
 size_t done; 
 int err; 
 int driver; 
 ao_device *dev; 
 ao_sample_format format; 
 int channels, encoding; long rate; 
 // Inicializar mpg123 
 mpg123_init(); 
 mh = mpg123_new(NULL, &err); 
 buffer_size = mpg123_outblock(mh); 
 buffer2 = (unsigned char*) malloc(buffer_size * sizeof(unsigned char)); 
 // Abrir el archivo MP3 
 if (mpg123_open(mh, "tmp.mp3") != MPG123_OK) {
  fprintf(stderr, "No se pudo abrir el archivo: %s\n", "tmp.mp3");
   return 1;
  }//if 
    // Obtener información del archivo MP3 
  
  mpg123_getformat(mh, &rate, &channels, &encoding); 
  
  // Configurar el formato de salida 
  format.bits = mpg123_encsize(encoding) * BITS; 
  format.rate = rate; 
  format.channels = channels; 
  format.byte_format = AO_FMT_NATIVE; 
  format.matrix = 0; 
  
  // Inicializar libao 
  ao_initialize(); 
  driver = ao_default_driver_id(); 
  dev = ao_open_live(driver, &format, NULL);
   if (dev == NULL) {
    fprintf(stderr, "Error al abrir el dispositivo de audio.\n");
    return 1; 
    }//if 
    
   // Reproducir el archivo MP3 
   int r;
   while ((r=mpg123_read(mh, buffer2, buffer_size, &done)) == MPG123_OK) {
    ao_play(dev, (char*)buffer2, done);
    printf("r=%d\n",r);
   }//while 
   printf("r2=%d\n",r);
     // Limpiar 
   free(buffer2); 
   ao_close(dev); 
   mpg123_close(mh); 
   mpg123_delete(mh); 
   mpg123_exit(); 
   ao_shutdown(); 
    /*************************************/
    return 0;
}
