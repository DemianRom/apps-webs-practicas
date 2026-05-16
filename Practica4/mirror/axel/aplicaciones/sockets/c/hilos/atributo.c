#include <stdio.h>
#include <stdlib.h>
#include <pthread.h>

void *imprime(void *p); //prototipo

void main(){
 pthread_t t1,t2;
 pthread_attr_t attr;
 char *msj1="Hilo 1";
 char *msj2="Hilo 2";
 int r1,r2,r,s;
size_t tam=0;
 r = pthread_attr_init(&attr);
 printf("Tabla de atributos creada \n");
 struct sched_param sch_params;
 int status;
 status = pthread_attr_getschedparam(&attr,&sch_params);
 printf("La prioridad sera de %d\n",sch_params.sched_priority);
 /*int rango;
 printf("Rango de prioridades: %d -> %d\n",sched_get_priority_min(rango),sched_get_priority_max(rango)); */
 r = pthread_attr_setdetachstate(&attr, PTHREAD_CREATE_DETACHED);
 s = pthread_attr_getstacksize(&attr,&tam);
 printf("La pila sera de %ld bytes\n",tam);
 int policy;
 printf("Imprimiendo política de planificacion antes de alterar el atributo:\n");
 pthread_attr_getschedpolicy(&attr,&policy);
 if(policy==SCHED_RR)
    printf("politica de planificacion SCHED_RR\n");
 else if(policy==SCHED_OTHER)
   printf("politica de planificacion SCHED_OTHER\n");
 else if(policy==SCHED_FIFO)
  printf("politica de planificacion SCHED_FIFO"); 
 printf("Politica de planificacion despues de alterar la tabla de atributos:\n");
 s = pthread_attr_setschedpolicy(&attr, SCHED_RR);
 sch_params.sched_priority = 99;
 s = pthread_attr_setschedparam(&attr,&sch_params);
pthread_attr_getschedpolicy(&attr,&policy);
 if(policy==SCHED_RR)
    printf("politica de planificacion SCHED_RR\n");
 else if(policy==SCHED_OTHER)
   printf("politica de planificacion SCHED_OTHER\n");
 else if(policy==SCHED_FIFO)
  printf("politica de planificacion SCHED_FIFO");
 pthread_attr_getschedparam(&attr,&sch_params);
 printf("La prioridad es %d\n",sch_params.sched_priority);
 r1 = pthread_create(&t1, &attr, imprime, (void*)msj1);
 r2 = pthread_create(&t2, NULL, imprime, (void*)msj2);
 int t; 
 r = pthread_attr_destroy(&attr);
 pthread_join(t1,NULL);
 pthread_join(t2,NULL);
 printf("la creacion de t1 devolvio %d\n",r1);
 printf("la creacion de t2 devolvio %d\n",r2);
 exit(0);
}

void *imprime(void *p){
 char *mensaje=(char *)p;
 printf("%s \n",mensaje);
}//imprime
