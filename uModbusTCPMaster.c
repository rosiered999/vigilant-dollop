#include "uModbusTCPMaster.h"

int network_init(uint8_t *ip_address, uint16_t port_number)
{
    int sockfd;
    struct addrinfo hints, *servinfo, *p;
    int rv;
    char s[INET6_ADDRSTRLEN];

    memset(&hints,0,sizeof(hints));
    hints.ai_family = AF_UNSPEC;
    hints.ai_socktype = SOCK_STREAM;

    if((rv=getaddrinfo(hostname,PORT,&hints,&servinfo))!=0)
    {
      fprintf(stderr, "getaddrinfo %s\n", gai_strerror(rv));
      return UMODBUS_GETADDRINFO_ERROR;
    }

    for(p=servinfo; p!=NULL;p=p->ai_next)
    {
      if((sockfd = socket(p->ai_family,p->ai_socktype,p->ai_protocol))==-1)
      {
        perror("client:socket");
        continue;
      }
      if (setsockopt(sockfd, SOL_SOCKET, SO_KEEPALIVE, &(int){ 1 }, sizeof(int)) < 0)
        perror("setsockopt(SO_KEEPALIVE) failed");
      if(connect(sockfd, p->ai_addr,p->ai_addrlen)==-1)
      {
        continue;
      }
      break;
    }
    if(p==NULL)
    {
       fprintf(stderr, "client failed to connect\n");
       return UMODBUS_CONNECTION_ERROR;
    }

    struct sockaddr* p1;
    if(p1->sa_family==AF_INET)
    {
      p1 = &(((struct sockaddr_in*)p1)->sin_addr);
    }
    else
        p1 = &(((struct sockaddr_in6*)p1)->sin6_addr);

    inet_ntop(p->ai_family, p1,s,sizeof(s));
    //printf("client connecting to %s\n", s);

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

int recv_error_check(int sockfd, unsigned char* reply, unsigned int* recv_ret_val)
{
    if ((recv_ret_val = recv(sockfd, reply, 50-1, 0)) == -1)
    {
       perror("recv");
       return UMODBUS_RECEIVE_ERROR;
    }
    else
    {
        //printf("receiving...\n");
        reply[recv_ret_val] = '\0';
        return UMODBUS_STATUS_SUCCESS;
    }
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
        //read_coils(starting_addr);
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
            printf("%d: ",i);
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
    else if(c=='\x14')
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
    else if(c=='\x17')
    {
        printf("enter starting address high bits: ");
        scanf("%x", &buf[8]);
        printf("enter starting address low bits: ");
        scanf("%x", &buf[9]);
        printf("quantity to read high bits: ");
        scanf("%x", &buf[10]);
        printf("quantity to read low bits: ");
        scanf("%x", &buf[11]);
        printf("write starting address high: ");
        scanf("%x", &buf[12]);
        printf("write starting address low: ");
        scanf("%x", &buf[13]);
        printf("quantity to write high: ");
        scanf("%x", &buf[14]);
        printf("quantity to write low: ");
        scanf("%x", &buf[15]);
        printf("write byte count: ");
        scanf("%x", &buf[16]);
        int num_values = (int)buf[16];
        last =16;
        for(int i=1;i<=num_values;i+=2)
        {
            printf("%d1: ",i);
            scanf("%x",&buf[last+i]);
            printf("%d2: ",i);
            scanf("%x",&buf[last+i+1]);
        }
        last = last+num_values;
    }
    else
    {
        printf("Error\n");
        return "";
    }
    for(int i=last;i<200;i++)
    {
        buf[i] = '\x00';
    }
    int temp = last - 6;
    buf[5] = (unsigned char)temp;
    //printf("%X\n", buf[5]);
    MSG_LEN = last;
    return buf;
}

void response_create(unsigned char reply[], int recv_ret_val)
{
    printf("client: confirmed received %d bytes \n",recv_ret_val);
    for(int i=0;i<recv_ret_val;i++)
    {
       char p = ((i==recv_ret_val-1)?'\n':' ');
       printf("%x%c", reply[i],p);
    }
}

unsigned char* read_coils(unsigned short starting_addr, unsigned short quantity_coils)
{
    starting_addr--;
    //quantity_coils++;
    int last = 0;
    unsigned char* buf = malloc(14);
    buf[0] = '\x00';
    buf[1] = '\x01';
    buf[2] = '\x00';
    buf[3] = '\x00';
    buf[4] = '\x00';
    buf[5] = '\x06';
    buf[6] = SLAVE_ADDR;
    buf[7] = '\x01';
    buf[8] = (starting_addr>>8)& '\xFF';
    buf[9] = (starting_addr)& '\xFF';
    buf[10] = (quantity_coils>>8)& '\xFF';
    buf[11] = (quantity_coils)& '\xFF';
    buf[12] = '\x00';
    MSG_LEN = 12;
    return buf;
}
void read_coils_response(unsigned char* buf, unsigned char* reply,int recv_ret_val)
{
    printf("client: confirmed received %d bytes \n",recv_ret_val);
   printf("slave id %d\n", reply[6]);
   printf("function code %d\n", reply[7]);
   printf("byte code %d\n", reply[8]);
   unsigned int low = buf[9];
   unsigned int high = buf[8];
   unsigned int starting_addr = low | (high << 8);
   int x = 1;
   for(int i=1;i<=reply[8];i++)
   {
       int b = (reply[8+i]);
       printf("%d-%d :", starting_addr+x+7,starting_addr+x);
       for(int i=1;i<=8;i++)
       {
           char p = ((i==8)?'\n':' ');
           int hi = (b)&1;
           printf("%d%c", hi,p);
           b = b>>1;
       }
       printf("\n");
       x+=8;
   }
}

unsigned char* read_holding_reg(unsigned short starting_addr, unsigned short quantity_reg)
{
    starting_addr--;
    unsigned char* buf = malloc(14);
    buf[0] = '\x00';
    buf[1] = '\x01';
    buf[2] = '\x00';
    buf[3] = '\x00';
    buf[4] = '\x00';
    buf[5] = '\x06';
    buf[6] = SLAVE_ADDR;
    buf[7] = '\x03';
    buf[8] = (starting_addr>>8)& '\xFF';
    buf[9] = (starting_addr)& '\xFF';
    buf[10] = (quantity_reg>>8)& '\xFF';
    buf[11] = (quantity_reg)& '\xFF';
    buf[12] = '\x00';
    MSG_LEN = 12;
    return buf;
}
void read_holding_reg_response(unsigned char* buf, unsigned char* reply, int recv_ret_val)
{
   printf("client: confirmed received %d bytes \n",recv_ret_val);
   printf("slave id %d\n", reply[6]);
   printf("function code %d\n", reply[7]);
   printf("byte code %d\n", reply[8]);
   unsigned int low = buf[9];
   unsigned int high = buf[8];
   unsigned int starting_addr = low | (high << 8);
  // printf("%d\n", reply[8]);
   for(int i=1;i<=reply[8];i+=2)
   {
       char p = ((i==reply[8]-1)?'\n':' ');
           int b = reply[8+i];
          int c = reply[i+8+1];
           printf("%X%X%c",b,c,p);
   }
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
        //read_coils(starting_addr);
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
            printf("%d: ",i);
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
    else if(c=='\x14')
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
    else if(c=='\x17')
    {
        printf("enter starting address high bits: ");
        scanf("%x", &buf[8]);
        printf("enter starting address low bits: ");
        scanf("%x", &buf[9]);
        printf("quantity to read high bits: ");
        scanf("%x", &buf[10]);
        printf("quantity to read low bits: ");
        scanf("%x", &buf[11]);
        printf("write starting address high: ");
        scanf("%x", &buf[12]);
        printf("write starting address low: ");
        scanf("%x", &buf[13]);
        printf("quantity to write high: ");
        scanf("%x", &buf[14]);
        printf("quantity to write low: ");
        scanf("%x", &buf[15]);
        printf("write byte count: ");
        scanf("%x", &buf[16]);
        int num_values = (int)buf[16];
        last =16;
        for(int i=1;i<=num_values;i+=2)
        {
            printf("%d1: ",i);
            scanf("%x",&buf[last+i]);
            printf("%d2: ",i);
            scanf("%x",&buf[last+i+1]);
        }
        last = last+num_values;
    }
    else
    {
        printf("Error\n");
        return "";
    }
    for(int i=last;i<200;i++)
    {
        buf[i] = '\x00';
    }
    int temp = last - 6;
    buf[5] = (unsigned char)temp;
    //printf("%X\n", buf[5]);
    MSG_LEN = last;
    return buf;
}

void response_create(unsigned char reply[], int recv_ret_val)
{
    printf("client: confirmed received %d bytes \n",recv_ret_val);
    for(int i=0;i<recv_ret_val;i++)
    {
       char p = ((i==recv_ret_val-1)?'\n':' ');
       printf("%x%c", reply[i],p);
    }
}

/*unsigned char* read_coils(unsigned short starting_addr, unsigned short quantity_coils)
{
    starting_addr--;
    //quantity_coils++;
    int last = 0;
    unsigned char* buf = malloc(14);
    buf[0] = '\x00';
    buf[1] = '\x01';
    buf[2] = '\x00';
    buf[3] = '\x00';
    buf[4] = '\x00';
    buf[5] = '\x06';
    buf[6] = SLAVE_ADDR;
    buf[7] = '\x01';
    buf[8] = (starting_addr>>8)& '\xFF';
    buf[9] = (starting_addr)& '\xFF';
    buf[10] = (quantity_coils>>8)& '\xFF';
    buf[11] = (quantity_coils)& '\xFF';
    buf[12] = '\x00';
    MSG_LEN = 12;
    return buf;
}
*/
void read_coils_response(unsigned char* buf, unsigned char* reply,int recv_ret_val)
{
    printf("client: confirmed received %d bytes \n",recv_ret_val);
   printf("slave id %d\n", reply[6]);
   printf("function code %d\n", reply[7]);
   printf("byte code %d\n", reply[8]);
   unsigned int low = buf[9];
   unsigned int high = buf[8];
   unsigned int starting_addr = low | (high << 8);
   int x = 1;
   for(int i=1;i<=reply[8];i++)
   {
       int b = (reply[8+i]);
       printf("%d-%d :", starting_addr+x+7,starting_addr+x);
       for(int i=1;i<=8;i++)
       {
           char p = ((i==8)?'\n':' ');
           int hi = (b)&1;
           printf("%d%c", hi,p);
           b = b>>1;
       }
       printf("\n");
       x+=8;
   }
}

unsigned char* read_holding_reg(unsigned short starting_addr, unsigned short quantity_reg)
{
    starting_addr--;
    unsigned char* buf = malloc(14);
    buf[0] = '\x00';
    buf[1] = '\x01';
    buf[2] = '\x00';
    buf[3] = '\x00';
    buf[4] = '\x00';
    buf[5] = '\x06';
    buf[6] = SLAVE_ADDR;
    buf[7] = '\x03';
    buf[8] = (starting_addr>>8)& '\xFF';
    buf[9] = (starting_addr)& '\xFF';
    buf[10] = (quantity_reg>>8)& '\xFF';
    buf[11] = (quantity_reg)& '\xFF';
    buf[12] = '\x00';
    MSG_LEN = 12;
    return buf;
}
void read_holding_reg_response(unsigned char* buf, unsigned char* reply, int recv_ret_val)
{
   printf("client: confirmed received %d bytes \n",recv_ret_val);
   printf("slave id %d\n", reply[6]);
   printf("function code %d\n", reply[7]);
   printf("byte code %d\n", reply[8]);
   unsigned int low = buf[9];
   unsigned int high = buf[8];
   unsigned int starting_addr = low | (high << 8);
  // printf("%d\n", reply[8]);
   for(int i=1;i<=reply[8];i+=2)
   {
       char p = ((i==reply[8]-1)?'\n':' ');
           int b = reply[8+i];
          int c = reply[i+8+1];
           printf("%X%X%c",b,c,p);
   }
}
