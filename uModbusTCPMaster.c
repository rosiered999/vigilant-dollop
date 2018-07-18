#include<math.h>
#include<ctype.h>

//TO-DOS
//write something like a signal handler to keep checking
//write a sig handler for freeing the request and response ADUs?
// if the connection is open, if not reconnect (POLL)
//packed structure in send and recv research; struct with all pointers if no packed

//Macros
#define HEADER_LEN 9
#define TCP_HEADER_LEN 4
#define MAX_MODBUS_FRAME_SIZE 260

//Global Definitions
uint8_t *total_ADU;
uint8_t *request_ADU;
uint8_t *response_ADU;
uint8_t msg_len;
uint8_t is_network_init = 0;
uint32_t sockfd;
uint32_t slave_address;
uint32_t transaction_id = 0; //pairing req and response, synchronous,

//fill header function, slave,
uint8_t* fill_header(uint16_t length)
{
    uint8_t* temp_ptr;
    temp_ptr = request_ADU;
    *(temp_ptr) = (transaction_id>>8)& '\xFF'; //transaction ID high
    temp_ptr++;
    *(temp_ptr) = transaction_id & '\xFF'; //transaction ID low
    temp_ptr++;
    transaction_id++;
    *(temp_ptr) = '\x00'; //Protocol ID high
    temp_ptr++;
    *(temp_ptr) = '\x00'; //Protocol ID low
    temp_ptr++;
    *(temp_ptr) = (length>>8)& '\xFF'; //Length high
    temp_ptr++;
    *temp_ptr = length & '\xFF';
    temp_ptr++;
    *temp_ptr = slave_address;
    temp_ptr++;
    return temp_ptr;
}

void data_struct_init(){
//MBAP Header -> transaction ID two bytes, Protocol ID two bytes, Length two bytes, unit ID one byte

    total_ADU = malloc(2*MAX_MODBUS_FRAME_SIZE);//only one memset
    //new pointer pointing to requestadu + tcp header len
    request_ADU = total_ADU;
    response_ADU = total_ADU+MAX_MODBUS_FRAME_SIZE;
}

uint32_t network_init (uint8_t *ip_address, uint16_t port_num){
    struct addrinfo hints, *servinfo, *p;
    int rv;
    int ret_val;
    char s[INET6_ADDRSTRLEN];
    char port_number[100];
    sprintf(port_number,"%d",port_num);

    memset(&hints,0,sizeof(hints));
    hints.ai_family = AF_UNSPEC;
    hints.ai_socktype = SOCK_STREAM;

    if((rv=getaddrinfo(ip_address,port_number,&hints,&servinfo))!=0)
    {
      fprintf(stderr, "getaddrinfo %s\n", gai_strerror(rv));
      printf("%d\n", errno);
      ret_val = UMODBUS_SEND_ERROR;
    }
    else
    {
        for(p=servinfo; p!=NULL;p=p->ai_next)
        {
            if((sockfd = socket(p->ai_family,p->ai_socktype,p->ai_protocol))==-1)
            {
                perror("client:socket");
                continue;
            }
            if (setsockopt(sockfd, SOL_SOCKET, SO_KEEPALIVE, &(int){ 1 }, sizeof(int)) < 0)
            {
                perror("setsockopt(SO_KEEPALIVE) failed");
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
           ret_val = UMODBUS_CONNECTION_ERROR;
        }
        else
        {
            struct sockaddr* p1;
            if(p->ai_family==AF_INET)
            {
              p1 = &(((struct sockaddr_in*)p)->sin_addr);
            }
            else
                p1 = &(((struct sockaddr_in6*)p)->sin6_addr);

            inet_ntop(p->ai_family, p1,s,sizeof(s));
            //printf("client connecting to %s\n", s);
            printf("socekt %d\n", sockfd);
            freeaddrinfo(servinfo);
            if(sockfd==0)
            {
                ret_val = UMODBUS_CONNECTION_ERROR;
            }
            else
            {
                is_network_init = 1;
                data_struct_init();
                ret_val = UMODBUS_STATUS_SUCCESS;
            }
        }
    }
    return ret_val;
}

uint32_t send_recv(uint32_t sockfd,
                   uint32_t* buf,
                   uint32_t* send_len,
                   uint32_t* reply,
                   uint32_t* recv_len){
    //Definitions
    int send_ret_val, recv_ret_val;
    int ret_val;
    //TCP reconnection loop
    if(send_len!=0 && ((send_ret_val=send(sockfd,request_ADU,send_len,0)) < 0)) //or strlen(request_ADU != last_pos_request)
    {
        ret_val = UMODBUS_SEND_ERROR;
    }
    else if(send_len==0 || ((send_ret_val=send(sockfd,request_ADU,send_len,0)) > 0))
    {
        if ((recv_ret_val = recv(sockfd, response_ADU, recv_len, 0)) <0)
        {
           perror("recv");
           ret_val = UMODBUS_RECEIVE_ERROR;
        }
        else
        {
            //printf("receiving...\n");
            response_ADU[recv_ret_val] = '\0';
            //printf("recv ret val in send rev%d\n", recv_ret_val);
            ret_val = UMODBUS_STATUS_SUCCESS;
        }
    }
    return ret_val;
}

uint32_t read_coils (uint16_t starting_addr,
                     uint16_t coil_count,
                     uint8_t *coils_status,
                     uint8_t  coils_status_size){
    uint32_t ret_val = UMODBUS_STATUS_SUCCESS;

    if(coils_status==NULL|| starting_addr<=0 )
    {
        ret_val = UMODBUS_INVALID_PARAM;
    }
    else if(coils_status_size<coil_count)
    {
        ret_val = UMODBUS_SIZE_INVALID;
    }
    else
    {
        uint32_t send_len, recv_len;
        //MODBUS protocol starts returning after skipping one value. To prevent that:
        starting_addr--;

        //Assigning values to the request string

        uint8_t* temp_ptr;
        temp_ptr = fill_header(6);
        *temp_ptr = '\x01';
        temp_ptr++;
        *temp_ptr = (starting_addr>>8)& '\xFF';
        temp_ptr++;
        *temp_ptr = (starting_addr)& '\xFF';
        temp_ptr++;
        *temp_ptr = (coil_count>>8)& '\xFF';
        temp_ptr++;
        *temp_ptr = (coil_count)& '\xFF';
        temp_ptr++;
        *temp_ptr = '\x00';
        temp_ptr++;

        send_len = (temp_ptr - request_ADU);
        recv_len = HEADER_LEN;

        int ret; //pased headerlen from which byte len is there size of len, default; define as macros
        //length in header matches bytes ret
        if((ret = send_recv(sockfd, request_ADU, send_len, response_ADU, recv_len))==UMODBUS_RECEIVE_ERROR)
        {
            ret_val = UMODBUS_RECEIVE_ERROR;
        }
        else
        {
            send_len = 0;
            int response_size = response_ADU[8];
            int ret;
            if((ret=recv(sockfd, (response_ADU+recv_len),response_size,0))<0)
            {
                ret_val = UMODBUS_RECEIVE_ERROR;
            }
            else
            {
                int coils_status_index=0;
                //Filling the response string
                for(int i=1;i<=response_size;i++)
                {
                    int temp_byte = (response_ADU[8+i]);
                    for(int j=1;j<=8;j++)
                    {
                        int bit = (temp_byte)&1;
                        coils_status[coils_status_index] = bit;
                        coils_status_index++;
                        temp_byte = temp_byte>>1;
                    }
                }
                ret_val = UMODBUS_STATUS_SUCCESS;
            }
        }
    }
    msg_len = 0;
    return ret_val;
}

uint32_t write_single_coil (uint16_t coil_addr, uint16_t coil_status)
{
    uint32_t ret_val = UMODBUS_STATUS_SUCCESS;

    if(coil_addr<=0 )
    {
        ret_val = UMODBUS_INVALID_PARAM;
    }
    else
    {
        uint32_t send_len, recv_len;
        //MODBUS protocol starts returning after skipping one value. To prevent that:
        coil_addr--;

        //Assigning values to the request string
        uint8_t *temp_ptr = request_ADU+msg_len;
        *(temp_ptr) = '\x00'; //Length high
        temp_ptr++;
        *temp_ptr = '\x06';
        temp_ptr++;
        *temp_ptr = slave_address;
        temp_ptr++;
        *temp_ptr = '\x05';
        temp_ptr++;
        *temp_ptr = (coil_addr>>8)& '\xFF';
        temp_ptr++;
        *temp_ptr = (coil_addr)& '\xFF';
        temp_ptr++;

        if(coil_status == 1)
        {
            *temp_ptr = '\xFF';
            temp_ptr++;
            *temp_ptr = '\x00';
            temp_ptr++;
        }
        else
        {
            *temp_ptr = '\x00';
            temp_ptr++;
            *temp_ptr = '\x00';
            temp_ptr++;
        }

        send_len = (temp_ptr - request_ADU);
        recv_len = HEADER_LEN;

        for(int i=0;i<send_len;i++)
        {
            printf("%x\n", *(request_ADU+i));
        }

        int ret;
        if((ret = send_recv(sockfd, request_ADU, send_len, response_ADU, recv_len))==UMODBUS_RECEIVE_ERROR)
        {
            ret_val = UMODBUS_RECEIVE_ERROR;
        }
        else
        {
            send_len = 0;
            int response_size = response_ADU[8];
            int ret;
            if((ret=recv(sockfd, (response_ADU+recv_len),response_size,0))<0)
            {
                ret_val = UMODBUS_RECEIVE_ERROR;
            }
            else
            {
                /*int coils_status_index=0;
                //Filling the response string
                for(int i=1;i<=response_size;i++)
                {
                    int temp_byte = (response_ADU[8+i]);
                    printf("%x\n", temp_byte);
                }*/
                ret_val = UMODBUS_STATUS_SUCCESS;
            }
        }
    }
    msg_len = TCP_HEADER_LEN;
    return ret_val;
}
