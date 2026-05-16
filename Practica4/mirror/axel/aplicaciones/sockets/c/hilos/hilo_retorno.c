#include <pthread.h>
#include <stdio.h>
#include <malloc.h>
#include <stdint.h>//tipo dato intptr_t 

struct sumandos{
int a;
int b;
};


void * suma (void* arg)
{
int res = 0;
struct sumandos *datos = (struct sumandos *)arg;
printf("\nDato a: %d",datos->a);
printf("\nDato b: %d",datos->b);
res = datos->a + datos->b;
return (void *)(intptr_t)res; //paso de int a void*
}

int main ()
{
pthread_t thread;

int resultado;

struct sumandos *x = (struct sumandos *)malloc(sizeof(struct sumandos));
x->a=3;
x->b=2;

pthread_create(&thread, NULL, suma,x);
pthread_join(thread,(void *)&resultado);
printf("\nEl resultado de sumar  %d y  %d  es %d.\n", x->a, x->b,resultado);
free(x);
return 0;
}
