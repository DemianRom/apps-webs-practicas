/**
*   http://mpg123.org/api/group__mpg123__error.shtml
*  gcc -o mp3player reproducir.c -lmpg123 -lao
*  ./reproducir tu_archivo.mp3
*/
#include <stdio.h> 
#include <stdlib.h> 
#include <mpg123.h> 
#include <ao/ao.h> 
#define BITS 8 
int main(int argc, char *argv[]) {
 if (argc < 2) { 
    fprintf(stderr, "Uso: %s <archivo.mp3>\n", argv[0]);
     return 1; 
 } 
 mpg123_handle *mh;
 unsigned char *buffer; 
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
 buffer = (unsigned char*) malloc(buffer_size * sizeof(unsigned char)); 
 // Abrir el archivo MP3 
 if (mpg123_open(mh, argv[1]) != MPG123_OK) {
  fprintf(stderr, "No se pudo abrir el archivo: %s\n", argv[1]);
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
   while ((r=mpg123_read(mh, buffer, buffer_size, &done)) == MPG123_OK) {
    ao_play(dev, (char*)buffer, done);
    printf("r=%d\n",r);
   }//while 
   printf("r2=%d\n",r);
     // Limpiar 
   free(buffer); 
   ao_close(dev); 
   mpg123_close(mh); 
   mpg123_delete(mh); 
   mpg123_exit(); 
   ao_shutdown(); 
   return 0;
    } 
