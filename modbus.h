//For input and its manipulation
#include<stdio.h>
#include<stdlib.h>
#include<string.h>
//For networking
#include<unistd.h>
#include<errno.h>
#include<netdb.h>
#include<sys/types.h>
#include<netinet/in.h>
#include<sys/socket.h>
#include<arpa/inet.h>
#define PORT "502"
#define FUNC_INFO 1

int MSG_LEN=12;

//Networking functions
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

    struct sockaddr* p1;
    if(p1->sa_family==AF_INET)
    {
      p1 = &(((struct sockaddr_in*)p1)->sin_addr);
    }
    else
        p1 = &(((struct sockaddr_in6*)p1)->sin6_addr);

    inet_ntop(p->ai_family, p1,s,sizeof(s));
    printf("client connecting to %s\n", s);

    freeaddrinfo(servinfo);
    return sockfd;
}

int send_error_check(int sockfd, unsigned char* buf)
{
    int send_ret_val;
    if((send_ret_val=send(sockfd,buf,MSG_LEN,0)) < 0)
    {
        perror("send");
    }
    else
    {
        printf("sending...\n");
    }
    return send_ret_val;
}
int recv_error_check(int sockfd, unsigned char* reply)
{
    int recv_ret_val;
    if ((recv_ret_val = recv(sockfd, reply, 50-1, 0)) == -1)
    {
       perror("recv");
       exit(1);
    }
    else
    {
        printf("receiving...\n");
    }
    reply[recv_ret_val] = '\0';
    return recv_ret_val;
}
//Info function
void print_info(char*);

//Request creator
unsigned char* request_create();

//Response creator
void response_create(unsigned char [],int);
