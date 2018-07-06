#include<stdio.h>
#include<stdlib.h>
#include<unistd.h>
#include<errno.h>
#include<string.h>
#include<netdb.h>
#include<sys/types.h>
#include<netinet/in.h>
#include<sys/socket.h>
#include<arpa/inet.h>
#define PORT "502"

void *get_in_addr(struct sockaddr *sa)
{
  if(sa->sa_family==AF_INET)
  {
    return &(((struct sockaddr_in*)sa)->sin_addr);
  }
  return &(((struct sockaddr_in6*)sa)->sin6_addr);
}

int network_init()
{
    int sockfd;
    char hostname[200];
    printf("Enter device IP address: ");
    scanf("%[^\n]", hostname);
   // if(strcmp(hostname,"\n")==0)
      //hostname[] = "localhost";
    printf("Initialising...\n");
    struct addrinfo hints, *servinfo, *p;
    int rv;
    char s[INET6_ADDRSTRLEN];

    memset(&hints,0,sizeof(hints));
    hints.ai_family = AF_UNSPEC;
    hints.ai_socktype = SOCK_STREAM;

    if((rv=getaddrinfo(hostname,PORT,&hints,&servinfo))!=0)
    {
      fprintf(stderr, "getaddrinfo %s\n", gai_strerror(rv));
      return 1;
    }

    for(p=servinfo; p!=NULL;p=p->ai_next)
    {
      if((sockfd = socket(p->ai_family,p->ai_socktype,p->ai_protocol))==-1)
      {
        perror("client:socket");
        continue;
      }
      if(connect(sockfd, p->ai_addr,p->ai_addrlen)==-1)
      {
        continue;
      }
      break;
    }
    if(p==NULL)
    {
       fprintf(stderr, "client failed to connect\n");
       return 2;
    }

    inet_ntop(p->ai_family, get_in_addr((struct sockaddr *)p->ai_addr),s,sizeof(s));
    printf("client connecting to %s\n", s);

    freeaddrinfo(servinfo);
    return sockfd;
}

unsigned char* request_create()
{
    unsigned char* buf = malloc(12);
    printf("\nEnter the required data: ");
    for(int i=0;i<11;i++)
    {
        unsigned char c;
        scanf("%x\n", &c);
        buf[i]=c;
    }
    buf[12] = '\x00';
/*    for(int i=0;i<12;i++)
    {
        printf("%x\n", buf[i]);
    }*/
    return buf;
}

int main(int argc, char* argv[])
{
    int numbytes;
    unsigned char reply[50];
    int sockfd = network_init();
    unsigned char* buf = request_create();
    int k;
    printf("connecting...\n");
      if((k =send(sockfd,buf,12,0)) < 0)
      {
        perror("send");
      }
      if ((numbytes = recv(sockfd, reply, 20-1, 0)) == -1) {
           perror("recv");
           exit(1);
       }

       reply[numbytes] = '\0';

       printf("client: received %d bytes \n",numbytes);
       for(int i=0;i<numbytes;i++)
       {
           char p = ((i==numbytes-1)?'\n':' ');
           printf("%x%c", reply[i],p);
       }
  close(sockfd);

  return 0;
}
