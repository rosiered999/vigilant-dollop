#include"modbus.h"

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
    /*if(c=='\x01')
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
    else */if(c=='\x02')
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
    /*for(int i=0;i<recv_ret_val;i++)
    {
       char p = ((i==recv_ret_val-1)?'\n':' ');
       printf("%d : %d, %x\n", i,reply[i],reply[i]);
   }*/
   printf("slave id %d\n", reply[6]);
   printf("function code %d\n", reply[7]);
   printf("byte code %d\n", reply[8]);
   unsigned int low = buf[9];
   unsigned int high = buf[8];
   unsigned int starting_addr = low | (high << 8);
   int x = 1;
   for(int i=1;i<=reply[8];i++)
   {
       int bin_arr[8] = {0};
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

int main(int argc, char* argv[])
{
    int send_ret_val;
    int recv_ret_val;
    unsigned char reply[50];
    int sockfd = network_init();
    printf("\nEnter the required data: ");
    printf("the slave address: ");
    scanf("%x", &SLAVE_ADDR);
    while(1)
    {
        unsigned int p,q;
        printf("enter starting address and ");
        printf("enter quantity of coils\n" );
        scanf("%d %d", &p,&q);
        printf("%x %x\n",p,q );
        unsigned char* buf = read_coils(p,q);//request_create();
        printf("connecting...\n");
        send_ret_val = send_error_check(sockfd,buf);
        recv_ret_val = recv_error_check(sockfd,reply);
        read_coils_response(buf, reply,recv_ret_val);
        //response_create(reply,recv_ret_val);
        //printf("Press 'q' to quit or 'n' to continue\n");
    }
}
