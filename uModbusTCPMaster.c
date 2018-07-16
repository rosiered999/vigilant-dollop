#include<math.h>
#include<ctype.h>

//TO-DOS
//TCP MODBUS ADU = 253 bytes + MBAP (7 bytes) = 260 bytes. Modbus Application protocol v1.1b3 pg5
//write something like a signal handler to keep checking
// if the connection is open, if not reconnect (POLL)

//DOUBTS
//write a sig handler for freeing the request and response ADUs?

//Global Definitions
uint8_t *request_ADU;
uint8_t *response_ADU;
int msg_len;
uint32_t last_pos_request = 0;
uint32_t last_pos_response = 0;
uint8_t is_network_init = 0;
uint32_t sockfd;
int slave_address;

void data_struct_init()
{
    request_ADU = malloc(260);
    response_ADU = malloc(260);
    //here put in the first few TCP header values
    uint8_t* temp_ptr = request_ADU;
    (temp_ptr) = '\x00';
    temp_ptr++;
    (temp_ptr) = '\x01';
    temp_ptr++;
    (temp_ptr) = '\x00';
    temp_ptr++;
    (temp_ptr) = '\x00';
    temp_ptr++;
    (temp_ptr) = '\x00';
    temp_ptr++;
    msg_len = temp_ptr - request_ADU; //default

}

uint32_t network_init (uint8_t *ip_address, uint16_t port_num){
    struct addrinfo hints, *servinfo, *p;
    int rv;
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
      return UMODBUS_SEND_ERROR;
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
    data_struct_init();
    if(sockfd==0)
        return UMODBUS_CONNECTION_ERROR;
    is_network_init = 1;
    return UMODBUS_STATUS_SUCCESS;
}

uint32_t send_recv(uint32_t sockfd,
                   uint32_t* buf,
                   uint32_t* send_len,
                   uint32_t* reply,
                   uint32_t* recv_len)
{
    /*if(is_network_init!=1) USE POLL; http://man7.org/linux/man-pages/man2/poll.2.html; with events=0 only revents
    {
        return UMODBUS_CONNECTION_ERROR;
    }*/
    //TCP reconnection loop
    if((*send_ret_val=send(sockfd,request_ADU,send_len,0)) < 0) //or strlen(request_ADU != last_pos_request)
    {
        return UMODBUS_SEND_ERROR;
    }

    //printf("send ret val in send rev%d\n", *send_ret_val);

    if ((*recv_ret_val = recv(sockfd, response_ADU, recv_len, 0)) <0)
    {
       perror("recv");
       return UMODBUS_RECEIVE_ERROR;
    }
        //printf("receiving...\n");
        response_ADU[*recv_ret_val] = '\0';
        //printf("recv ret val in send rev%d\n", *recv_ret_val);
        return UMODBUS_STATUS_SUCCESS;
}

uint32_t send_error_check (uint32_t sockfd,
                          uint8_t* buf,
                          uint32_t* send_ret_val)
{
    if((send_ret_val=send(sockfd,buf,msg_len,0)) < 0)
    {
        return UMODBUS_SEND_ERROR;
    }
    else
    {
        return UMODBUS_STATUS_SUCCESS;
    }
}

uint32_t recv_error_check (int32_t sockfd,
                          uint8_t* reply,
                          uint32_t* recv_ret_val)
{
    if ((*recv_ret_val = recv(sockfd, reply, 50-1, 0)) == -1)
    {
       perror("recv");
       return UMODBUS_RECEIVE_ERROR;
    }
    else
    {
        //printf("receiving...\n");
        reply[*recv_ret_val] = '\0';
        return UMODBUS_STATUS_SUCCESS;
    }
}

uint32_t read_coils (uint16_t starting_addr,
                     uint16_t coil_count,
                     uint8_t *coils_status,
                     uint8_t  coils_status_size)
{
    uint32_t ret_val = UMODBUS_STATUS_SUCCESS;
    uint32_t send_len, recv_len;
    if(coils_status==NULL|| starting_addr<=0 || isalpha(starting_addr) )
    {
        ret_val = UMODBUS_INVALID_PARAM;
    }
    else if(coils_status_size<coil_count)
    {
        ret_val = UMODBUS_SIZE_INVALID;
    }
    else
    {
        //MODBUS protocol starts returning after skipping one value. To prevent that:
        starting_addr--;

        //Assigning values to the request string
        uint32_t *temp_ptr = request_ADU+msg_len;
        temp_ptr = '\x06';
        temp_ptr++;
        temp_ptr = slave_address;
        temp_ptr++;
        temp_ptr = '\x01';
        temp_ptr++;
        temp_ptr = (starting_addr>>8)& '\xFF';
        temp_ptr++;
        temp_ptr = (starting_addr)& '\xFF';
        temp_ptr++;
        temp_ptr = (coil_count>>8)& '\xFF';
        temp_ptr++;
        temp_ptr = (coil_count)& '\xFF';
        temp_ptr++;
        temp_ptr = '\x00';
        temp_ptr++;
        send_len = temp_ptr - request_ADU;
        recv_len = 200;
        //Send and receive function
        if(send_recv(sockfd, request_ADU, send_len, response_ADU, recv_len)==UMODBUS_RECEIVE_ERROR)
        {
            ret_val = UMODBUS_RECEIVE_ERROR;
        }
        else
        {
            //Predicting the response size
            //TCP header + packet size + slave adress + function code + byte count returned + bytes returned
            int response_size = 5 + 1 + 1 + 1 + 1 + ceil(coil_count/8);
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

    return ret_val;
}

uint32_t read_discrete_inputs (uint16_t starting_addr,
                               uint16_t inputs_count,
                               uint8_t *inputs_status,
                               uint8_t  inputs_status_size)
{
    //Definitions
    uint32_t send_ret_val;
    uint32_t recv_ret_val;
    int inputs_status_index=0;

    //Returning errors
    if(inputs_status==NULL||starting_addr<=0 || isalpha(starting_addr) )
        return UMODBUS_INVALID_PARAM;
    if(inputs_status_size<inputs_count)
        return UMODBUS_SIZE_INVALID;

    //MODBUS protocol starts returning after scoils_status_indexipping one value. To prevent that:
    starting_addr--;

    //Assigning values to the request string
    *(request_ADU+5) = '\x06';
    *(request_ADU+6) = slave_address;
    *(request_ADU+7) = '\x02';
    *(request_ADU+8) = (starting_addr>>8)& '\xFF';
    *(request_ADU+9) = (starting_addr)& '\xFF';
    *(request_ADU+10) = (inputs_count>>8)& '\xFF';
    *(request_ADU+11) = (inputs_count)& '\xFF';
    *(request_ADU+12) = '\x00';

    //Send and receive function
    if(send_recv(sockfd, &send_ret_val,&recv_ret_val)==UMODBUS_RECEIVE_ERROR)
        return UMODBUS_RECEIVE_ERROR;

    //Predicting the response size
    //TCP header + packet size + slave adress + function code + byte count returned + bytes returned
    int response_size = 5 + 1 + 1 + 1 + 1 + ceil(inputs_count/8);

    //Filling the response string
    for(int i=1;i<=response_size;i++)
    {
        int temp_byte = (response_ADU[8+i]);
        for(int j=1;j<=8;j++)
        {
            int bit = (temp_byte)&1;
            inputs_status[inputs_status_index] = bit;
            inputs_status_index++;
            temp_byte = temp_byte>>1;
        }
    }
    return UMODBUS_STATUS_SUCCESS;
}

uint32_t read_holding_reg (uint16_t starting_addr,
                               uint16_t reg_count,
                               uint8_t *reg_values,
                               uint8_t  reg_values_size)
{
    //Definitions
    uint32_t send_ret_val;
    uint32_t recv_ret_val;
    int reg_values_index=0;

    //Returning errors
    if(reg_values==NULL||starting_addr<=0 || isalpha(starting_addr) )
        return UMODBUS_INVALID_PARAM;
    if(reg_values_size<2*reg_count)
        return UMODBUS_SIZE_INVALID;

    //MODBUS protocol starts returning after scoils_status_indexipping one value. To prevent that:
    starting_addr--;

    //Assigning values to the request string
    *(request_ADU+5) = '\x06';
    *(request_ADU+6) = slave_address;
    *(request_ADU+7) = '\x03';
    *(request_ADU+8) = (starting_addr>>8)& '\xFF';
    *(request_ADU+9) = (starting_addr)& '\xFF';
    *(request_ADU+10) = (reg_count>>8)& '\xFF';
    *(request_ADU+11) = (reg_count)& '\xFF';
    *(request_ADU+12) = '\x00';

    //Send and receive function
    if(send_recv(sockfd, &send_ret_val,&recv_ret_val)==UMODBUS_RECEIVE_ERROR)
        return UMODBUS_RECEIVE_ERROR;

    //Predicting the response size
    //TCP header + packet size + slave adress + function code + byte count returned + bytes returned
    int response_size = 5 + 1 + 1 + 1 + 1 + 2*reg_count;

    //Filling the response string
    for(int i=9;i<response_size;i+=2)
    {
        int lo = response_ADU[i+1];
        int hi = response_ADU[i];
        //printf("%x %x\n",response_ADU[i],response_ADU[i+1]);
        reg_values[reg_values_index] = (hi>>4) | lo;
        reg_values_index++;
    }
    return UMODBUS_STATUS_SUCCESS;
}

uint32_t read_input_reg (uint16_t starting_addr,
                               uint16_t reg_count,
                               uint8_t *reg_values,
                               uint8_t  reg_values_size)
{
    //Definitions
    uint32_t send_ret_val;
    uint32_t recv_ret_val;
    int reg_values_index=0;

    //Returning errors
    if(reg_values==NULL||starting_addr<=0 || isalpha(starting_addr) )
        return UMODBUS_INVALID_PARAM;
    if(reg_values_size<2*reg_count)
        return UMODBUS_SIZE_INVALID;

    //MODBUS protocol starts returning after scoils_status_indexipping one value. To prevent that:
    starting_addr--;

    //Assigning values to the request string
    *(request_ADU+5) = '\x06';
    *(request_ADU+6) = slave_address;
    *(request_ADU+7) = '\x04';
    *(request_ADU+8) = (starting_addr>>8)& '\xFF';
    *(request_ADU+9) = (starting_addr)& '\xFF';
    *(request_ADU+10) = (reg_count>>8)& '\xFF';
    *(request_ADU+11) = (reg_count)& '\xFF';
    *(request_ADU+12) = '\x00';

    //Send and receive function
    if(send_recv(sockfd, &send_ret_val,&recv_ret_val)==UMODBUS_RECEIVE_ERROR)
        return UMODBUS_RECEIVE_ERROR;

    //Predicting the response size
    //TCP header + packet size + slave adress + function code + byte count returned + bytes returned
    int response_size = 5 + 1 + 1 + 1 + 1 + 2*reg_count;

    //Filling the response string
    for(int i=9;i<response_size;i+=2)
    {
        int lo = response_ADU[i+1];
        int hi = response_ADU[i];
        //printf("%x %x\n",response_ADU[i],response_ADU[i+1]);
        reg_values[reg_values_index] = (hi>>4) | lo;
        reg_values_index++;
    }
    return UMODBUS_STATUS_SUCCESS;
}

uint32_t write_single_coil (uint16_t coil_addr, uint16_t coil_status)
{
    ;
}

uint32_t write_single_reg (uint16_t reg_addr, uint16_t reg_val)
{
    reg_addr--;
    uint8_t* buf = malloc(14);
    buf[0] = '\x00';
    buf[1] = '\x01';
    buf[2] = '\x00';
    buf[3] = '\x00';
    buf[4] = '\x00';
    buf[5] = '\x06';
    buf[6] = slave_address;
    buf[7] = '\x06';
    buf[8] = (reg_addr>>8)& '\xFF';
    buf[9] = (reg_addr)& '\xFF';

    buf[10] = (reg_val>>8)& '\xFF';
    buf[11] = (reg_val)& '\xFF';

    buf[12] = '\x00';
    uint32_t send_ret_val;

    uint8_t* reply = malloc(200);
    uint32_t recv_ret_val;
    if(send_recv(sockfd, &send_ret_val,&recv_ret_val)==UMODBUS_RECEIVE_ERROR)
        return UMODBUS_RECEIVE_ERROR;
    //printf("received\n");
}


uint32_t write_multiple_coils (uint16_t starting_addr,
                              uint16_t coils_count,
                              uint8_t *coils_status,
						  	  uint16_t coils_status_size)
{
    starting_addr--;
    int last = 0;
    //memset(&buf,0,200;
    uint16_t* buf = malloc(15);
    buf[0] = '\x00';
    buf[1] = '\x01';
    buf[2] = '\x00';
    buf[3] = '\x00';
    buf[4] = '\x00';
    buf[5] = '\x09';
    buf[6] = slave_address;
    buf[7] = '\x0F';
    buf[8] = '\x00';
    buf[9] = '\x13';
    buf[10] = '\x00';
    buf[11] = '\x0A';
    buf[12] = '\x02';
    buf[13] = '\xCD';
    buf[14] = '\x01';
    buf[15] = '\x00';
    msg_len = 15;

    buf[5] = 6 + ceil(coils_count/8);
    printf("%d\n", buf[5]);
    buf[6] = slave_address;
    buf[7] = '\x0F';
    buf[8] = coils_count;
    int k = 0;
    int l = 9;
    for(int i=1;i<=coils_count;i+=4)
    {
        uint16_t byte = 0;
        for (int j=0; j < 4; ++j)
        {
            //printf("%d\n", coils_status[i+j]);
            if (coils_status[i+j])
                byte |= 1 << 3-j;
        }
        if(k%2==0)
            buf[l] = byte;
        else
        {
            int temp;
            //printf("in multiple coils4\n" );
            temp = byte | (buf[l]<<4);
            //printf("%x\n", temp);
            buf[l] = temp;
            l++;
        }
        //printf("%x\n", byte);
        k++;
    }
    for(int i=l;i<200;i++)
    {
        buf[i] = '\x00';
    }

    for(int i=0;i<msg_len;i++)
    {
        printf("%d\n", buf[i]);
    }
    printf("%d \n", msg_len);
    uint32_t send_ret_val;

    //if((send_error_check(sockfd, buf, send_ret_val)==UMODBUS_SEND_ERROR))
    //    return UMODBUS_SEND_ERROR;
    //printf("sent\n");
    uint8_t* reply = malloc(200);
    uint32_t recv_ret_val;
    if(send_recv(sockfd, &send_ret_val,&recv_ret_val)==UMODBUS_RECEIVE_ERROR)
        return UMODBUS_RECEIVE_ERROR;
    //printf("received\n");
}

uint32_t mask_write_reg (uint16_t ref_addr,
                        uint16_t and_mask,
                        uint16_t or_mask)
{
    uint8_t* buf = malloc(14);
    buf[0] = '\x00';
    buf[1] = '\x01';
    buf[2] = '\x00';
    buf[3] = '\x00';
    buf[4] = '\x00';
    buf[5] = '\x06';
    buf[6] = slave_address;

}
