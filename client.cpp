#include"modbus.h"

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

unsigned char* request_create()
{
    int last = 0;
    unsigned char* buf = malloc(200);
    unsigned char c;
    buf[0] = '\x00';
    buf[1] = '\x01';
    buf[2] = '\x00';
    buf[3] = '\x00';
    buf[4] = '\x00';
    int count = 0;
    printf("\nEnter the required data: ");
    printf("the slave address: ");
    scanf("%x", &c);
    buf[6] = c;
    printf("the function code: ");
    scanf("%x", &c);
    buf[7] = c;
    if(c=='\x01')
    {
        printf("enter starting address high bits: ");
        scanf("%x", &buf[8]);
        printf("enter starting address low bits: ");
        scanf("%x", &buf[9]);
        printf("enter quantity of coils high bit: ");
        scanf("%x", &buf[10]);
        printf("enter quantity of coils low bit: ");
        scanf("%x", &buf[11]);
        last = 12;
    }
    else if(c=='\x02')
    {
        printf("enter starting address high bits: ");
        scanf("%x", &buf[8]);
        printf("enter starting address low bits: ");
        scanf("%x", &buf[9]);
        printf("enter quantity of inputs high bit: ");
        scanf("%x", &buf[10]);
        printf("enter quantity of inputs low bit: ");
        scanf("%x", &buf[11]);
        last =12;
    }
    else if(c=='\x03')
    {
        printf("enter starting address high bits: ");
        scanf("%x", &buf[8]);
        printf("enter starting address low bits: ");
        scanf("%x", &buf[9]);
        printf("enter quantity of registers high bit: ");
        scanf("%x", &buf[10]);
        printf("enter quantity of registers low bit: ");
        scanf("%x", &buf[11]);
        last = 12;
    }
    else if(c=='\x04')
    {
        printf("enter starting address high bits: ");
        scanf("%x", &buf[8]);
        printf("enter starting address low bits: ");
        scanf("%x", &buf[9]);
        printf("enter quantity of input registers high bit: ");
        scanf("%x", &buf[10]);
        printf("enter quantity of input registers low bit: ");
        scanf("%x", &buf[11]);
        last =12;
    }
    else if(c=='\x05')
    {
        printf("enter output address high bits: ");
        scanf("%x", &buf[8]);
        printf("enter output address low bits: ");
        scanf("%x", &buf[9]);
        printf("enter output value high bit: ");
        scanf("%x", &buf[10]);
        printf("enter output value low bit: ");
        scanf("%x", &buf[11]);
        last =12;
    }
    else if(c=='\x06')
    {
        printf("enter registers address high bits: ");
        scanf("%x", &buf[8]);
        printf("enter registers address low bits: ");
        scanf("%x", &buf[9]);
        printf("enter register value high bit: ");
        scanf("%x", &buf[10]);
        printf("enter register value low bit: ");
        scanf("%x", &buf[11]);
        last =12;
    }
    else if(c=='\x0F')
    {
        printf("enter starting address high bits: ");
        scanf("%x", &buf[8]);
        printf("enter starting address low bits: ");
        scanf("%x", &buf[9]);
        printf("enter quantity of outputs high bit: ");
        scanf("%x", &buf[10]);
        printf("enter quantity of outputs low bit: ");
        scanf("%x", &buf[11]);
        printf("enter byte count: ");
        scanf("%x", &buf[12]);
        int num_values = (int)buf[12];
        last =12;
        for(int i=1;i<=num_values;i++)
        {
            scanf("%x",&buf[last+i]);
        }
        last = last+num_values;
    }
    else if(c=='\x10')
    {
        printf("enter starting address high bits: ");
        scanf("%x", &buf[8]);
        printf("enter starting address low bits: ");
        scanf("%x", &buf[9]);
        printf("enter quantity of registers high bit: ");
        scanf("%x", &buf[10]);
        printf("enter quantity of registers low bit: ");
        scanf("%x", &buf[11]);
        printf("enter byte count: ");
        scanf("%x", &buf[12]);
        int num_values = (int)buf[12];
        last =12;
        for(int i=1;i<=num_values;i++)
        {
            printf("%d: ",i);
            scanf("%x",&buf[last+i]);
        }
        last = last+num_values;
    }
    for(int i=last;i<200;i++)
    {
        buf[i] = '\x00';
    }
    int temp = last - 6;
    buf[5] = (unsigned char)temp;
    printf("%X\n", buf[5]);
    MSG_LEN = last;
    return buf;
}

void response_create(unsigned char reply[], int recv_ret_val)
{
    printf("client: received %d bytes \n",recv_ret_val);
    for(int i=0;i<recv_ret_val;i++)
    {
       char p = ((i==recv_ret_val-1)?'\n':' ');
       printf("%x%c", reply[i],p);
    }
}

int main(int argc, char* argv[])
{
    int send_ret_val;
    int recv_ret_val;
    unsigned char reply[50];
    int sockfd = network_init();
    unsigned char* buf = request_create();

    printf("connecting...\n");

    if((send_ret_val=send(sockfd,buf,MSG_LEN,0)) < 0)
    {
        perror("send");
    }
    else
    {
        printf("sending...\n");
    }
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

    response_create(reply,recv_ret_val);
    close(sockfd);
    return 0;
}
