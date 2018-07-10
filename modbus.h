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
char SLAVE_ADDR;

struct File_Record{
    unsigned short reference_type;
    unsigned short file_number;
    unsigned short record_num;
    unsigned short record_len;
};

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

//Request creator - general
unsigned char* request_create();

//Response creator - general
void response_create(unsigned char [],int);

//Function code 0x01 - reads the binary status of coils in the remote device
//Error code is Function code + 0x80 = 0x81
//Exception code is 01 - function code not supported
//Exception code is 03 - quantity of outputs not in the range [1,2000]
//Exception code is 02 - starting_addr and starting_addr+quantity_op are not within range
//Exception code is 04 - unable to read outputs
//Response is of the form byte_count, output_status
unsigned char* read_coils(unsigned short starting_addr, unsigned short quantity_coils);
void read_coils_response(unsigned char* buf, unsigned char* reply,int recv_ret_val);

//Function code 0x02 - reads the binary status of discrete inputs in the remote device
//Error code is function code + 0x02 = 0x82
//Exception code is 01 - function code not supported
//Exception code is 03 - quantity of outputs not in the range [1,2000]
//Exception code is 02 - starting_addr and starting_addr+quantity_op are not within range
//Exception code is 04 - unable to read outputs
//Response is of the form byte_count, discrete input status
unsigned char* read_discrete_ip(unsigned short starting_addr, unsigned short quantity_ip);
void read_discrete_ip_response(unsigned char* buf, unsigned char* reply, int recv_ret_val);

//Function code 0x03 - reads the value of holding registers in the remote device
//Error code is Function code + 0x80 = 0x83
//Exception code is 01 - function code not supported
//Exception code is 03 - quantity of outputs not in the range [1,2000]
//Exception code is 02 - starting_addr and starting_addr+quantity_op are not within range
//Exception code is 04 - unable to read outputs
//Response is of the form byte_count, register values
unsigned char* read_holding_reg(unsigned short starting_addr, unsigned short quantity_reg);
void read_holding_reg_response(unsigned char* buf, unsigned char* reply, int recv_ret_val);

//Function code 0x04 - reads the value of input registers in the remote device
//Error code is Function code + 0x80 = 0x84
//Exception code is 01 - function code not supported
//Exception code is 03 - quantity of outputs not in the range [1,2000]
//Exception code is 02 - starting_addr and starting_addr+quantity_op are not within range
//Exception code is 04 - unable to read outputs
//Response is of the form byte_count, input register values
unsigned char* read_ip_reg(unsigned short starting_addr, unsigned short quantity_ip_reg);
void read_ip_reg_response(unsigned char* buf, unsigned char* reply,int recv_ret_val);

//Function code 0x05 - writes a single coil in the remote device
//Error code is Function code + 0x80 = 0x85
//Exception code is 01 - function code not supported
//Exception code is 03 - quantity of outputs not in the range [1,2000]
//Exception code is 02 - starting_addr and starting_addr+quantity_op are not within range
//Exception code is 04 - unable to read outputs
//Response is of the form output address, output value
unsigned char* write_single_coil(unsigned short op_addr, unsigned short op_val);
void write_single_coil_response(unsigned char* buf, unsigned char* reply,int recv_ret_val);

//Function code 0x06 - writes a single register in the remote device
//Error code is Function code + 0x80 = 0x86
//Exception code is 01 - function code not supported
//Exception code is 03 - quantity of outputs not in the range [1,2000]
//Exception code is 02 - starting_addr and starting_addr+quantity_op are not within range
//Exception code is 04 - unable to read outputs
//Response is of the form register address, register value
unsigned char* write_single_reg(unsigned short reg_addr, unsigned short reg_val);
void write_single_reg_response(unsigned char* buf, unsigned char* reply,int recv_ret_val);

//Function code 0x0F - write multiple coils in the remote device
//Error code is Function code + 0x80 = 0x8F
//Exception code is 01 - function code not supported
//Exception code is 03 - quantity of outputs not in the range [1,2000]
//Exception code is 02 - starting_addr and starting_addr+quantity_op are not within range
//Exception code is 04 - unable to read outputs
//Response is of the form starting address, quantity of outputs
unsigned char* write_multiple_coils(unsigned short starting_addr, unsigned short quantity_op, unsigned short byte_count, unsigned short op_val[]);
void write_multiple_coils_response(unsigned char* buf, unsigned char* reply,int recv_ret_val);

//Function code 0x10 - write multiple registers in the remote device
//Error code is Function code + 0x80 = 0x90
//Exception code is 01 - function code not supported
//Exception code is 03 - quantity of outputs not in the range [1,2000]
//Exception code is 02 - starting_addr and starting_addr+quantity_op are not within range
//Exception code is 04 - unable to read outputs
//Response is of the form starting address, quantity of registers
unsigned char* write_multiple_reg(unsigned short starting_addr, unsigned short quantity_reg, unsigned short byte_count, unsigned short reg_val[]);
void write_multiple_reg_response(unsigned char* buf, unsigned char* reply,int recv_ret_val);

//Function code 0x14 - read file records in the remote device
//Error code is Function code + 0x80 = 0x94
//Exception code is 01 - function code not supported
//Exception code is 03 - quantity of outputs not in the range [1,2000]
//Exception code is 02 - starting_addr and starting_addr+quantity_op are not within range
//Exception code is 04 - unable to read outputs
//Response is of the form request data length, subrequest i, subrequest i data
unsigned char* read_file_rec(unsigned short byte_count, struct File_Record records[]);
void read_file_rec_response(unsigned char* buf, unsigned char* reply,int recv_ret_val);

//Function code 0x15 - write file records in the remote device
//Error code is Function code + 0x80 = 0x95
//Exception code is 01 - function code not supported
//Exception code is 03 - quantity of outputs not in the range [1,2000]
//Exception code is 02 - starting_addr and starting_addr+quantity_op are not within range
//Exception code is 04 - unable to read outputs
//Response is of the form request data length, subrequest i, subrequest i data
unsigned char* write_file_rec(unsigned short request_data_len, struct File_Record records[]);
void write_file_rec_response(unsigned char* buf, unsigned char* reply,int recv_ret_val);

//Function code 0x16 - modify the contents of a specified holding register using a combination of an AND mask, an OR mask, and the register's current contents
//Error code is Function code + 0x80 = 0x96
//Exception code is 01 - function code not supported
//Exception code is 03 - quantity of outputs not in the range [1,2000]
//Exception code is 02 - starting_addr and starting_addr+quantity_op are not within range
//Exception code is 04 - unable to read outputs
//Response is of the form reference address, and mask, or mask
unsigned char* mask_write_reg(unsigned short ref_addr, unsigned short and_mask, unsigned short or_mask);
void mask_write_reg_response(unsigned char* buf, unsigned char* reply,int recv_ret_val);

//Function code 0x17 - both read and write multiple registers
//Exception code is 01 - function code not supported
//Exception code is 03 - quantity of outputs not in the range [1,2000]
//Exception code is 02 - starting_addr and starting_addr+quantity_op are not within range
//Exception code is 04 - unable to read outputs
//Response is of the form byte count, read registers values
unsigned char* rw_multiple_reg(unsigned short r_starting_addr, unsigned short read_quantity, unsigned short w_starting_addr, unsigned short write_quantity, unsigned short w_byte_count, unsigned short w_reg_val[]);
void rw_multiple_reg_response(unsigned char* buf, unsigned char* reply,int recv_ret_val);

//Function code 0x18 - read FIFO queue of the register
//Error code is Function code + 0x80 = 0x96
//Exception code is 01 - function code not supported
//Exception code is 03 - quantity of outputs not in the range [1,2000]
//Exception code is 02 - starting_addr and starting_addr+quantity_op are not within range
//Exception code is 04 - unable to read outputs
//Response is of the form byte count, fifo count, fifo value registers
unsigned char* read_FIFO_q(unsigned short FIFO_ptr_addr);
void read_FIFO_q_response(unsigned char* buf, unsigned char* reply,int recv_ret_val);

//Function code 0x2B - mechanism for tunneling service requests and method invocations,  as well as their returns, inside MODBUS PDUs
//Error code is Function code + 0x80 = 0xAB
//Exception code is 01 - function code not supported
//Exception code is 03 - quantity of outputs not in the range [1,2000]
//Exception code is 02 - starting_addr and starting_addr+quantity_op are not within range
//Exception code is 04 - unable to read outputs
//Response is of the form MEI type, MEI type specific data
unsigned char* encap_interface_transport(unsigned short MEI_type, unsigned short MEI_type_data);
void encap_interface_transport_response(unsigned char* buf, unsigned char* reply,int recv_ret_val);
