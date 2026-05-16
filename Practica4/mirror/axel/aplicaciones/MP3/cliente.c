#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <arpa/inet.h>
#include <mpg123.h>
#include <ao/ao.h>

#define BITS 8
//#define PORT 12345
//#define BUFFER_SIZE 4096

int main(int argc, char *argv[]) {
    if (argc < 2) {
        fprintf(stderr, "Uso: %s <dirección_servidor>\n", argv[0]);
        exit(1);
    }

    int sockfd, BUFFER_SIZE=60000;//4096;
    unsigned short PORT = 12345;
    struct sockaddr_in server_addr;
    char buffer[BUFFER_SIZE];
    ssize_t bytes_received;

    mpg123_handle *mh;
    unsigned char *audio_buffer;
    size_t audio_buffer_size;
    size_t done;
    int err;

    int driver;
    ao_device *dev;

    ao_sample_format format;
    int channels, encoding;
    long rate;

    // Inicializar mpg123
    mpg123_init();
    mh = mpg123_new(NULL, &err);
    audio_buffer_size = mpg123_outblock(mh);
    printf("audio_buffer_size:%ld\n",audio_buffer_size);
    audio_buffer = (unsigned char*) malloc(audio_buffer_size * sizeof(unsigned char));
    if(audio_buffer==NULL){perror("No hay memoria suficiente para asignar\n");
       exit(1);
    }//if

    // Inicializar libao
    ao_initialize();
    driver = ao_default_driver_id();

    // Crear socket
    if ((sockfd = socket(AF_INET, SOCK_STREAM, 0)) < 0) {
        perror("Error al crear el socket");
        exit(1);
    }

    // Configurar la dirección del servidor
    memset(&server_addr, 0, sizeof(server_addr));
    server_addr.sin_family = AF_INET;
    server_addr.sin_port = htons(PORT);
    if (inet_pton(AF_INET, argv[1], &server_addr.sin_addr) <= 0) {
        perror("Error al convertir la dirección IP");
        close(sockfd);
        exit(1);
    }
       
    // Conectar al servidor
    if (connect(sockfd, (struct sockaddr *)&server_addr, sizeof(server_addr)) < 0) {
        perror("Error al conectar al servidor");
        close(sockfd);
        exit(1);
    }

    printf("Conectado al servidor %s:%d\n", argv[1], PORT);

    // Configurar mpg123 para decodificar desde un stream
    mpg123_open_feed(mh);
     
    // Recibir y reproducir el audio
    while ((bytes_received = recv(sockfd, buffer, BUFFER_SIZE, 0)) > 0) {  /*****buffer*///
        // Alimentar el decodificador con los datos recibidos
        printf("%ld bytes recibidos\n",bytes_received);
        mpg123_feed(mh, (unsigned char*)buffer, bytes_received);/**buffer****/


	int r;
        // Decodificar y reproducir el audio
        while ((r=mpg123_read(mh, audio_buffer, audio_buffer_size, &done)) == MPG123_OK) {
            printf("r:%d\n",r);
            if (format.bits == 0) {
                // Obtener el formato de audio la primera vez
                mpg123_getformat(mh, &rate, &channels, &encoding);
                format.bits = mpg123_encsize(encoding) * BITS;
                format.rate = rate;
                format.channels = channels;
                format.byte_format = AO_FMT_NATIVE;
                format.matrix = 0;

                // Abrir el dispositivo de audio
                dev = ao_open_live(driver, &format, NULL);
                if (dev == NULL) {
                    fprintf(stderr, "Error al abrir el dispositivo de audio.\n");
                    exit(1);
                }//if
            }//if
            ao_play(dev, (char*)audio_buffer, done);
        }//while
        printf("r2=%d\n",r);
    }//while

    printf("Reproducción completada.\n");

    // Limpiar
    free(audio_buffer);
    ao_close(dev);
    mpg123_close(mh);
    mpg123_delete(mh);
    mpg123_exit();
    ao_shutdown();

    close(sockfd);

    return 0;
}
