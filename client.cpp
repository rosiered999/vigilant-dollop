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
#include<iostream>
#include<bits/stdc++.h>
#define PORT "502"

void *get_in_addr(struct sockaddr *sa)
{
  if(sa->sa_family==AF_INET)
  {
    return &(((struct sockaddr_in*)sa)->sin_addr);
  }
  return &(((struct sockaddr_in6*)sa)->sin6_addr);
}

int main(int argc, char* argv[])
{
  int sockfd, numbytes;
 // unsigned char buf[12] = {0x00,0x01,0x00,0x00,0x00,0x06,0x11,0x04,0x00,0x08,0x00,0x01};

  //printf("%X begin\n",buf );
  char hostname[200];
  printf("Enter device IP address: ");
  scanf("%s", hostname);
  printf("Initialising...\n");
  unsigned char reply[50];
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
    //    printf("%s\n", p->ai_addr);
     // perror("client connect");
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
 // do {
    //  scanf("%s", buf);
        //printf("%s\n", buf);
    unsigned char buf[13];
    buf[0] = '\x00';
    buf[1] = '\x01';
    buf[2] = '\x00';
    buf[3] = '\x00';
    buf[4] = '\x00';
    buf[5] = '\x06';
    int k=5;
    printf("\nEnter the required data ending with 00: ");
    for(int i=0;i<=5;i++)
    {
        unsigned char c;
        scanf("%x\n", &c);
        k++;
        buf[k]=c;
      //  printf("%d\n", i);
    }
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
