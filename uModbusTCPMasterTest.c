#include "uModbusTCPMaster.h"
#include "uModbusTCPMaster.c"
#define MSG_LEN 12


int main(int argc, char* argv[])
{
    int send_ret_val;
    int recv_ret_val;
    unsigned char reply[50];
    uint8_t* hostname;
    printf("Enter device IP address: ");
    scanf("%[^\n]", hostname);
    int sockfd = network_init(hostname);
    printf("\nEnter the required data: ");
    printf("the slave address: ");
    scanf("%x", &SLAVE_ADDR);
    while(1)
    {
        unsigned int p,q;
        printf("enter starting address and ");
        printf("enter quantity of registers\n" );
        scanf("%d %d", &p,&q);
        printf("%x %x\n",p,q );
        unsigned char* buf = read_holding_reg(p,q);//request_create();
        printf("connecting...\n");
        send_ret_val = send_error_check(sockfd,buf);
        recv_ret_val = recv_error_check(sockfd,reply);
        read_holding_reg_response(buf, reply,recv_ret_val);
        //response_create(reply,recv_ret_val);
    }
}
