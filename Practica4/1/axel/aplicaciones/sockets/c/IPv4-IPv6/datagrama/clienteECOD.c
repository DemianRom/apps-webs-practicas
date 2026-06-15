#include <sys/types.h>
#include <sys/socket.h>
#include <netdb.h>//getaddrinfo()
#include <stdio.h>  //printf, perror
#include <stdlib.h>  //exit()
#include <unistd.h> //read(), write()
#include <string.h>

#define BUF_SIZE 500

int
main(int argc, char *argv[])
{
    struct addrinfo dir;
    struct addrinfo *result, *rp;
    int cd, s, j,n, n1,n2,rv, max=10;
    size_t len;
    ssize_t nread;
    char buf[BUF_SIZE];
    //char eco[65535];
    char tmp[max];

   if (argc < 3) {
        fprintf(stderr, "Sintaxis: %s host port msg...\n", argv[0]);
        exit(EXIT_FAILURE);
    }



   memset(&dir, 0, sizeof(struct addrinfo));
    dir.ai_family = AF_UNSPEC;    /* Allow IPv4 or IPv6 */
    dir.ai_socktype = SOCK_DGRAM; 
    dir.ai_flags = 0;
    dir.ai_protocol = 0;          

   s = getaddrinfo(argv[1], "9999", &dir, &result);
    if (s != 0) {
        fprintf(stderr, "getaddrinfo: %s\n", gai_strerror(s));
        exit(EXIT_FAILURE);
    }

   for (rp = result; rp != NULL; rp = rp->ai_next) {
        cd = socket(rp->ai_family, rp->ai_socktype,
                     rp->ai_protocol);
        if (cd == -1)
            continue;

       if (connect(cd, rp->ai_addr, rp->ai_addrlen) != -1)
            break;                  

       close(cd);
    }//for

   if (rp == NULL) {               
        fprintf(stderr, "Could not connect\n");
        exit(EXIT_FAILURE);
    }

   freeaddrinfo(result);    
   
      
    struct addrinfo dir2,*dst;
    memset(&dir2,0,sizeof(dir2));
    dir2.ai_family = AF_UNSPEC;    /* Allow IPv4 or IPv6 */
    dir2.ai_socktype = SOCK_DGRAM; 
    dir2.ai_flags = AI_NUMERICHOST;
    dir2.ai_protocol = 0;
    rv = getaddrinfo(argv[1], "9999", &dir2, &dst);
    if (rv != 0) {
        fprintf(stderr, "getaddrinfo: %s\n", gai_strerror(rv));
        exit(EXIT_FAILURE);
    }//if
    
   /********************************************************/
    char *linea;
    linea=NULL; 
    size_t tam; 
    tam=0;
 printf("Escribe una serie de cadenas <enter> para enviar, SALIR para terminar\n");
   while((n=getline(&linea,&tam,stdin))!=-1){
        //printf("strlen da %d \n",(int)strlen(linea));
        if(strlen(linea)==1){
           printf("fflush\n");
          fflush(stdin);
          continue;
        }
        linea[strlen(linea)-1]='\0';
       char eco[(int)strlen(linea)];
  	int ii;
        printf("tamaño del eco: %d bytes-->%ld \n",(int)strlen(linea),sizeof(eco));
        printf("Preparado para enviar %d bytes, con el mensaje: %s\n",(int)strlen(linea),linea);
        int nn = (int)strlen(linea),npaquete=0,npaquetes;
        if(nn>max){
          npaquetes = (int)(strlen(linea)/max);
          int jj;
          char eco_tmp[max];
          for(ii=0;ii<npaquetes;ii++){
              memset(tmp,0,sizeof(tmp));
              memset(eco_tmp,0,sizeof(eco_tmp));
              for(jj=0;jj<max;jj++){
                  printf("ii=%d, tmp[%d]=%c\n",ii,jj,(char )linea[(ii*max)+jj]);
                  tmp[jj]=linea[(ii*max)+jj];
              }//for
              tmp[max]='\0';
              printf("Paquete %d a ser enviado:%s\n",ii,tmp);
              n1 = sendto(cd,tmp,strlen(tmp),0,(struct sockaddr *)dst->ai_addr,dst->ai_addrlen);
	        if(n1<0){
	          printf("Error de escritura en send()\n");
	        } else if(n1==0){
	            printf("Socket cerrado en send()\n");
	        }//if
		//memset(tmp_eco,0,sizeof(tmp_eco));//////////////////
		n2=recv(cd,eco_tmp,sizeof(eco_tmp),0);
	        if(n2<0){
	          printf("Error de lectura en recv()\n");
	        } else if(n==0){
	            printf("Socket cerrado en recv()\n");
	        }//if
                for(jj=0;jj<max;jj++){
                  //printf("ii=%d, tmp[%d]=%c\n",ii,jj,(char )linea[(ii*max)+jj]);
                  eco[(ii*max)+jj]=tmp[jj];
              }//for
          }//for
          int sobrantes = strlen(linea)%max;
          if(sobrantes>0){
             npaquetes= npaquetes+1;
             printf("Se enviarán los %d bytes restantes \n",sobrantes);
             char msj_sobrantes[sobrantes];
             char tmp_eco_sobrantes[sobrantes];
             //memset(tmp,'\0',sizeof(tmp));
	     memset(msj_sobrantes,'\0',sizeof(msj_sobrantes));
             memset(tmp_eco_sobrantes,'\0',sizeof(tmp_eco_sobrantes));
             for(jj=0;jj<sobrantes;jj++){
              printf("msj_sobrantes[%d]=%c\n",jj,(char )linea[((npaquetes-1)*max)+jj]);
              msj_sobrantes[jj]=linea[((npaquetes-1)*max)+jj];
             }//for
             //tmp[sobrantes]='\0';
	     msj_sobrantes[sobrantes]='\0';
             //printf("Paquete %d. Ultimo paquete a ser enviado:%s\n",ii,tmp);
             n1 = sendto(cd,msj_sobrantes,strlen(msj_sobrantes),0,(struct sockaddr *)dst->ai_addr,dst->ai_addrlen);
	     if(n1<0){
	       printf("Error de escritura en send()\n");
	     } else if(n1==0){
	       printf("Socket cerrado en send()\n");
	     } //if
             printf("Paquete %d con %d bytes. Ultimo paquete a ser enviado:%s\n",ii,n1,msj_sobrantes);
             n2=recv(cd,tmp_eco_sobrantes,sizeof(tmp_eco_sobrantes),0);
	        if(n2<0){
	          printf("Error de lectura en recv()\n");
	        } else if(n==0){
	            printf("Socket cerrado en recv()\n");
	        }//if
                tmp_eco_sobrantes[n2]='\0';
                printf("%d bytes recibidos.  Sobrantes recibidos:%s\n",n2,tmp_eco_sobrantes);
                for(jj=0;jj<sobrantes;jj++){
                  //printf("ii=%d, tmp[%d]=%c\n",ii,jj,(char )linea[(ii*max)+jj]);
                  //eco[(ii*max)+jj]=tmp[jj];
                  eco[((npaquetes-1)*max)+jj]=tmp_eco_sobrantes[jj];
              }//for
	     int pos =((npaquetes-1)*max)+sobrantes;
	    printf("pos=%d\n",pos);
              eco[pos]='\0';
              printf("Tam eco: %ld,  eco recibido: %s\n",strlen(eco),eco);
              }//if sobrantes>0
        } else {
         if(strncasecmp(linea,"SALIR",5)==0){
	   printf("escribio SALIR %ld\n",strlen(linea));
	   n1 = sendto(cd,linea,strlen(linea),0,(struct sockaddr *)dst->ai_addr,dst->ai_addrlen);
	   if(n1<0){
	     printf("Error de escritura en send()\n");
	   } else if(n1==0){
	       printf("Socket cerrado en send()\n");
	   }//if
	  close(cd);
          free(linea);
          linea=NULL;
          tam=0;
	  exit(0);
	} else {
           n1 = sendto(cd,linea,strlen(linea),0,(struct sockaddr *)dst->ai_addr,dst->ai_addrlen);
	   if(n1<0){
	     printf("Error de escritura en send()\n");
	   } else if(n1==0){
	       printf("Socket cerrado en send()\n");
	   }//if
           memset(eco,0,sizeof(eco));
           n2=recv(cd,eco,sizeof(eco),0);
           if(n2<0){
             printf("Error de lectura en recv()\n");
           } else if(n==0){
             printf("Socket cerrado en recv()\n");
           }//if
           eco[n2]='\0';
           printf("%d bytes recibidos. Eco recibido: %s\n",n2,eco);
           free(linea);
           linea=NULL;
           tam=0;
        }//else
      }//else
}//while

   
 
}
