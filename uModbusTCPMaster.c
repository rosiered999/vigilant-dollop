#include<math.h>
#include<ctype.h>

//TODO:write something like a signal handler to keep checking

//TODO:write a sig handler for freeing the request and response ADUs?

// TODO:if the connection is open, if not reconnect (POLL)

/*TODO:packed structure in send and recv research; struct with all pointers if no packed
https://stackoverflow.com/questions/8568432/is-gccs-attribute-packed-pragma-pack-unsafe*/

//TODO:pased headerlen from which byte len is there size of len, default; define as macros

//TODO:length in header matches bytes ret

//remove length from struct, data single yte array; send base will be packet pointers; change data[1] but pointer stuff same
// in init put send packet pointer to send and recv to recv buff
//mm=em compariosn for header len
struct packet {
    uint8_t transaction_id_hi;
    uint8_t transaction_id_lo;
    uint8_t protocol_id_hi;
    uint8_t protocol_id_lo;
    uint8_t length_hi;
    uint8_t length_lo;
    uint8_t slave_address;
    uint8_t function_code;
    uint8_t PDU_data[1];

} __attribute__((packed));

//Macros
#define HEADER_LEN 9
#define TCP_HEADER_LEN 4
#define DEFAULT_MSG_LEN 6
#define MAX_MODBUS_FRAME_SIZE 260

//Global Definitions
uint8_t *memory_pool;
uint8_t *send_buffer;
uint8_t *receive_buffer;
struct packet *request_ADU;
struct packet *response_ADU;
uint8_t is_network_init = 0;
uint32_t sockfd;
uint32_t slave_address;
uint32_t transaction_id = 0;

void fill_packet(uint8_t function_code, struct packet *request_ADU, uint16_t length){
    request_ADU->transaction_id_hi = (transaction_id>>8)& '\xFF';
    request_ADU->transaction_id_lo = (transaction_id)& '\xFF';
    transaction_id++;
    request_ADU->protocol_id_hi = '\x00';
    request_ADU->protocol_id_lo = '\x00';
    request_ADU->length_hi = (length>>8)& '\xFF';
    request_ADU->length_lo = (length)& '\xFF';
    request_ADU->slave_address = slave_address;
    request_ADU->function_code = function_code;
}

uint8_t* fill_header(uint16_t length){
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
    memory_pool = malloc(2*MAX_MODBUS_FRAME_SIZE);
    request_ADU = send_buffer = memory_pool;
    response_ADU = receive_buffer = memory_pool+MAX_MODBUS_FRAME_SIZE;
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

uint32_t send_recv(uint32_t sockfd, ///change the global vars
                   /*uint32_t* buf,*/
                   uint8_t *send_buf,
                   uint32_t* send_len,
                   uint8_t *receive_buf,
                    uint32_t receive_len){
    //Definitions
    int send_ret_val, recv_ret_val;
    int recv_len = HEADER_LEN;
    int ret_val = UMODBUS_STATUS_SUCCESS;
    //TCP reconnection loop
    send_ret_val = send(sockfd,send_buf,send_len,0);
    if(send_ret_val < 0)
    {
        ret_val = UMODBUS_SEND_ERROR;
    }
    else if(send_ret_val > 0)
    {
        //printf("receiving header\n");
        recv_ret_val = recv(sockfd, receive_buf, recv_len, 0); ///receive header
        if (recv_ret_val<0)
        {
           perror("recv");
           ret_val = UMODBUS_RECEIVE_ERROR;
        }
        else
        {
            ret_val = UMODBUS_STATUS_SUCCESS;
            int response_size = receive_len;
            //printf("response %d\n", response_size);
            if(response_size==0)
                response_size = 252;
            int ret;
            //printf("sendlen%d recvlen%d\n", send_len, recv_len);
            for(int i=0;i<20;i++)
            {
            //    printf("sendrecv%x \n", *(receive_buf+i));
            }
            //printf("receiving rest\n");
            ret=recv(sockfd, (receive_buf+recv_len),response_size,0); //receive the rest
            for(int i=recv_len;i<recv_len+5;i++)
            {
                //printf("sendrecv2222222%x \n", *(receive_buf+i));
            }
            if(ret<0)
            {
                ret_val = UMODBUS_RECEIVE_ERROR;
            }
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
        int receive_len = ceil(coil_count/8);

        uint8_t* temp_ptr = request_ADU->PDU_data;
        *temp_ptr = (starting_addr>>8)& '\xFF';
        temp_ptr++;
        *temp_ptr = (starting_addr)& '\xFF';
        temp_ptr++;
        *temp_ptr = (coil_count>>8)& '\xFF';
        temp_ptr++;
        *temp_ptr = (coil_count)& '\xFF';
        temp_ptr++;

        fill_packet('\x01', request_ADU, DEFAULT_MSG_LEN);

        send_len = temp_ptr - send_buffer;
        temp_ptr = request_ADU->PDU_data;
        printf("sendlen%d req ADU %d \n", send_len, sizeof(request_ADU));
        recv_len = HEADER_LEN;

        int ret;
        if((ret = send_recv(sockfd, send_buffer, send_len, receive_buffer, receive_len))==UMODBUS_RECEIVE_ERROR)
        {
            ret_val = UMODBUS_RECEIVE_ERROR;
        }
        else
        {
            //memory comparison for the headers of the receive and the send buffers
            int result = memcmp(request_ADU, response_ADU, HEADER_LEN);

            printf("result%d\n",result );

                send_len = 0;
                int coils_status_index=0;
                //Filling the response string
                int response_size = receive_buffer[8];
                for(int i=1;i<=response_size;i++)
                {
                    int temp_byte = (receive_buffer[8+i]);
                    printf("%x\n", temp_byte);
                    for(int j=1;j<=8;j++)
                    {
                        int bit = (temp_byte)&1;
                        coils_status[coils_status_index] = bit;
                        coils_status_index++;
                        temp_byte = temp_byte>>1;
                    }
                }
            if(result==1)
            {
                ret_val = UMODBUS_STATUS_SUCCESS;
            }
            else
            {
                ret_val = UMODBUS_RECEIVE_ERROR;
            }
        }
    }
    memset(memory_pool,0,2*MAX_MODBUS_FRAME_SIZE);
    request_ADU = memory_pool;
    send_buffer = memory_pool;
    response_ADU = memory_pool+MAX_MODBUS_FRAME_SIZE;
    receive_buffer = memory_pool+MAX_MODBUS_FRAME_SIZE;
    return ret_val;
}

uint32_t read_discrete_inputs (uint16_t starting_addr,
                               uint16_t inputs_count,
                               uint8_t *inputs_status,
                               uint8_t  inputs_status_size){
    uint32_t ret_val = UMODBUS_STATUS_SUCCESS;

    if(inputs_status==NULL|| starting_addr<=0 )
    {
        ret_val = UMODBUS_INVALID_PARAM;
    }
    else if(inputs_status_size<inputs_count)
    {
        ret_val = UMODBUS_SIZE_INVALID;
    }
    else
    {
        uint32_t send_len, recv_len;
        //MODBUS protocol starts returning after skipping one value. To prevent that:
        starting_addr--;

        //Assigning values to the request string
        int receive_len = ceil(inputs_count/8);

        uint8_t* temp_ptr = request_ADU->PDU_data;
        *temp_ptr = (starting_addr>>8)& '\xFF';
        temp_ptr++;
        *temp_ptr = (starting_addr)& '\xFF';
        temp_ptr++;
        *temp_ptr = (inputs_count>>8)& '\xFF';
        temp_ptr++;
        *temp_ptr = (inputs_count)& '\xFF';
        temp_ptr++;

        fill_packet('\x02', request_ADU, DEFAULT_MSG_LEN);

        send_len = temp_ptr - send_buffer;
        temp_ptr = request_ADU->PDU_data;
        printf("sendlen%d req ADU %d \n", send_len, sizeof(request_ADU));
        recv_len = HEADER_LEN;

        int ret;
        if((ret = send_recv(sockfd, send_buffer, send_len, receive_buffer, receive_len))==UMODBUS_RECEIVE_ERROR)
        {
            ret_val = UMODBUS_RECEIVE_ERROR;
        }
        else
        {
            //memory comparison for the headers of the receive and the send buffers
            int result = memcmp(request_ADU, response_ADU, HEADER_LEN);

            printf("result%d\n",result );

                send_len = 0;
                int inputs_status_size=0;
                //Filling the response string
                int response_size = receive_buffer[8];
                for(int i=1;i<=response_size;i++)
                {
                    int temp_byte = (receive_buffer[8+i]);
                    //printf("%x\n", temp_byte);
                    for(int j=1;j<=8;j++)
                    {
                        int bit = (temp_byte)&1;
                        printf("%d\n", bit);
                        inputs_status[inputs_status_size] = bit;
                        inputs_status_size++;
                        temp_byte = temp_byte>>1;
                    }
                }
            if(result==1)
            {
                ret_val = UMODBUS_STATUS_SUCCESS;
            }
            else
            {
                ret_val = UMODBUS_RECEIVE_ERROR;
            }
        }
    }
    memset(memory_pool,0,2*MAX_MODBUS_FRAME_SIZE);
    request_ADU = memory_pool;
    send_buffer = memory_pool;
    response_ADU = memory_pool+MAX_MODBUS_FRAME_SIZE;
    receive_buffer = memory_pool+MAX_MODBUS_FRAME_SIZE;
    return ret_val;
}

uint32_t read_holding_reg (uint16_t starting_addr,
                          uint16_t reg_count,
						  uint8_t *reg_values,
						  uint8_t  reg_values_size){
    uint32_t ret_val = UMODBUS_STATUS_SUCCESS;

    if(reg_values==NULL|| starting_addr<=0 )
    {
        ret_val = UMODBUS_INVALID_PARAM;
    }
    else if(reg_values_size<reg_count)
    {
        ret_val = UMODBUS_SIZE_INVALID;
    }
    else
    {
        uint32_t send_len, recv_len;
        //MODBUS protocol starts returning after skipping one value. To prevent that:
        starting_addr--;

        //Assigning values to the request string
        int receive_len = ceil(reg_count*2);

        uint8_t* temp_ptr = request_ADU->PDU_data;
        *temp_ptr = (starting_addr>>8)& '\xFF';
        temp_ptr++;
        *temp_ptr = (starting_addr)& '\xFF';
        temp_ptr++;
        *temp_ptr = (reg_count>>8)& '\xFF';
        temp_ptr++;
        *temp_ptr = (reg_count)& '\xFF';
        temp_ptr++;

        fill_packet('\x03', request_ADU, DEFAULT_MSG_LEN);

        send_len = temp_ptr - send_buffer;
        temp_ptr = request_ADU->PDU_data;
        printf("sendlen%d req ADU %d \n", send_len, sizeof(request_ADU));
        recv_len = HEADER_LEN;

        int ret;
        if((ret = send_recv(sockfd, send_buffer, send_len, receive_buffer, receive_len))==UMODBUS_RECEIVE_ERROR)
        {
            ret_val = UMODBUS_RECEIVE_ERROR;
        }
        else
        {
            //memory comparison for the headers of the receive and the send buffers
            int result = memcmp(request_ADU, response_ADU, HEADER_LEN);

            printf("result%d\n",result );

                send_len = 0;
                int reg_values_size=0;
                //Filling the response string
                int response_size = receive_buffer[8];
                printf("response size %d\n", response_size);
                for(int i=1;i<=response_size;i+=2)
                {
                    int hi = *(receive_buffer+8+i);
                    int lo =  *(receive_buffer+8+i+1);
                    int val = lo | (hi<<8);
                    reg_values[reg_values_size] = val;
                    reg_values_size++;
                }
            if(result==1)
            {
                ret_val = UMODBUS_STATUS_SUCCESS;
            }
            else
            {
                ret_val = UMODBUS_RECEIVE_ERROR;
            }
        }
    }
    memset(memory_pool,0,2*MAX_MODBUS_FRAME_SIZE);
    request_ADU = memory_pool;
    send_buffer = memory_pool;
    response_ADU = memory_pool+MAX_MODBUS_FRAME_SIZE;
    receive_buffer = memory_pool+MAX_MODBUS_FRAME_SIZE;
    return ret_val;
}

uint32_t read_input_reg (uint16_t starting_addr,
                          uint16_t reg_count,
						  uint8_t *reg_values,
						  uint8_t  reg_values_size){
      uint32_t ret_val = UMODBUS_STATUS_SUCCESS;

      if(reg_values==NULL|| starting_addr<=0 )
      {
          ret_val = UMODBUS_INVALID_PARAM;
      }
      else if(reg_values_size<reg_count)
      {
          ret_val = UMODBUS_SIZE_INVALID;
      }
      else
      {
          uint32_t send_len, recv_len;
          //MODBUS protocol starts returning after skipping one value. To prevent that:
          starting_addr--;

          //Assigning values to the request string
          int receive_len = ceil(reg_count*2);

          uint8_t* temp_ptr = request_ADU->PDU_data;
          *temp_ptr = (starting_addr>>8)& '\xFF';
          temp_ptr++;
          *temp_ptr = (starting_addr)& '\xFF';
          temp_ptr++;
          *temp_ptr = (reg_count>>8)& '\xFF';
          temp_ptr++;
          *temp_ptr = (reg_count)& '\xFF';
          temp_ptr++;

          fill_packet('\x04', request_ADU, DEFAULT_MSG_LEN);

          send_len = temp_ptr - send_buffer;
          temp_ptr = request_ADU->PDU_data;
          printf("sendlen%d req ADU %d \n", send_len, sizeof(request_ADU));
          recv_len = HEADER_LEN;

          int ret;
          if((ret = send_recv(sockfd, send_buffer, send_len, receive_buffer, receive_len))==UMODBUS_RECEIVE_ERROR)
          {
              ret_val = UMODBUS_RECEIVE_ERROR;
          }
          else
          {
              //memory comparison for the headers of the receive and the send buffers
              int result = memcmp(request_ADU, response_ADU, HEADER_LEN);

              printf("result%d\n",result );

                  send_len = 0;
                  int reg_values_size=0;
                  //Filling the response string
                  int response_size = receive_buffer[8];
                  printf("response size %d\n", response_size);
                  for(int i=1;i<=response_size;i+=2)
                  {
                      int hi = *(receive_buffer+8+i);
                      int lo =  *(receive_buffer+8+i+1);
                      int val = lo | (hi<<8);
                      reg_values[reg_values_size] = val;
                      reg_values_size++;
                  }
              if(result==1)
              {
                  ret_val = UMODBUS_STATUS_SUCCESS;
              }
              else
              {
                  ret_val = UMODBUS_RECEIVE_ERROR;
              }
          }
      }
      memset(memory_pool,0,2*MAX_MODBUS_FRAME_SIZE);
      request_ADU = memory_pool;
      send_buffer = memory_pool;
      response_ADU = memory_pool+MAX_MODBUS_FRAME_SIZE;
      receive_buffer = memory_pool+MAX_MODBUS_FRAME_SIZE;
      return ret_val;
}

uint32_t write_single_coil (uint16_t coil_addr, uint16_t coil_status){
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
        //int length = ceil(coil_count/8);

        uint8_t* temp_ptr = request_ADU->PDU_data;
        *temp_ptr = (coil_addr>>8)& '\xFF';
        temp_ptr++;
        *temp_ptr = (coil_addr)& '\xFF';
        temp_ptr++;
        if(coil_status==1)
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
        fill_packet('\x05', request_ADU, DEFAULT_MSG_LEN);
        //temp_ptr = request_ADU->PDU_data;

        send_len = temp_ptr - send_buffer;
        //printf("sendlen%d req ADU %d elngth %d \n", send_len, sizeof(request_ADU), length);
        recv_len = HEADER_LEN;
        int receive_len = temp_ptr - request_ADU->PDU_data; //same as the sent data len
        int ret;
        if((ret = send_recv(sockfd, send_buffer, send_len, receive_buffer, receive_len))==UMODBUS_RECEIVE_ERROR)
        {
            ret_val = UMODBUS_RECEIVE_ERROR;
        }
        else
        {
            //memory comparison for the headers of the receive and the send buffers
            int result = memcmp(send_buffer, receive_buffer, HEADER_LEN);
            //printf("result%d\n",result );
                int response_size = receive_buffer[8];
                for(int i=1;i<=response_size;i++)
                {
                    int temp_byte = (receive_buffer[8+i]);
                    printf("%x\n", temp_byte);
                }
            if(result==1)
            {
                ret_val = UMODBUS_STATUS_SUCCESS;
            }
            else
            {
                ret_val = UMODBUS_RECEIVE_ERROR;
            }
        }
    }
    memset(memory_pool,0,2*MAX_MODBUS_FRAME_SIZE);
    request_ADU = memory_pool;
    send_buffer = memory_pool;
    response_ADU = memory_pool+MAX_MODBUS_FRAME_SIZE;
    receive_buffer = memory_pool+MAX_MODBUS_FRAME_SIZE;
    return ret_val;
}

uint32_t write_single_reg (uint16_t reg_addr, uint16_t reg_val){
    uint32_t ret_val = UMODBUS_STATUS_SUCCESS;

    if(reg_addr<=0 )
    {
        ret_val = UMODBUS_INVALID_PARAM;
    }
    else
    {
        uint32_t send_len, recv_len;
        //MODBUS protocol starts returning after skipping one value. To prevent that:
        reg_addr--;

        //Assigning values to the request string
        //int length = ceil(coil_count/8);

        uint8_t* temp_ptr = request_ADU->PDU_data;
        *temp_ptr = (reg_addr>>8)& '\xFF';
        temp_ptr++;
        *temp_ptr = (reg_addr)& '\xFF';
        temp_ptr++;
        *temp_ptr = (reg_val>>8) & '\xFF';
        temp_ptr++;
        *temp_ptr = reg_val & '\xFF';
        temp_ptr++;
        fill_packet('\x06', request_ADU, DEFAULT_MSG_LEN);
        //temp_ptr = request_ADU->PDU_data;

        send_len = temp_ptr - send_buffer;
        //printf("sendlen%d req ADU %d elngth %d \n", send_len, sizeof(request_ADU), length);
        recv_len = HEADER_LEN;
        int receive_len = temp_ptr - request_ADU->PDU_data; //same as the sent data len
        int ret;
        if((ret = send_recv(sockfd, send_buffer, send_len, receive_buffer, receive_len))==UMODBUS_RECEIVE_ERROR)
        {
            ret_val = UMODBUS_RECEIVE_ERROR;
        }
        else
        {
            //memory comparison for the headers of the receive and the send buffers
            int result = memcmp(send_buffer, receive_buffer, HEADER_LEN);
            //printf("result%d\n",result );
                int response_size = receive_buffer[8];
                for(int i=1;i<=response_size;i++)
                {
                    int temp_byte = (receive_buffer[8+i]);
                    printf("%x\n", temp_byte);
                }
            if(result==1)
            {
                ret_val = UMODBUS_STATUS_SUCCESS;
            }
            else
            {
                ret_val = UMODBUS_RECEIVE_ERROR;
            }
        }
    }
    memset(memory_pool,0,2*MAX_MODBUS_FRAME_SIZE);
    request_ADU = memory_pool;
    send_buffer = memory_pool;
    response_ADU = memory_pool+MAX_MODBUS_FRAME_SIZE;
    receive_buffer = memory_pool+MAX_MODBUS_FRAME_SIZE;
    return ret_val;
}

uint32_t write_multiple_coils (uint16_t starting_addr,
                              uint16_t coils_count,
                              uint8_t *coils_status,
						  	  uint16_t coils_status_size){
    uint32_t ret_val = UMODBUS_STATUS_SUCCESS;

    if(starting_addr<=0 )
    {
        ret_val = UMODBUS_INVALID_PARAM;
    }
    else
    {
        uint32_t send_len, recv_len;
        //MODBUS protocol starts returning after skipping one value. To prevent that:
        starting_addr--;

        //Assigning values to the request string
        //int length = ceil(coil_count/8);

        uint8_t* temp_ptr = request_ADU->PDU_data;
        int byte_count = ceil(coils_status_size/8)*8;
        for(int i=0;i<coils_status_size;i+=8)
        {
            int byte=0;
            for(int j=0;j<8;j++)
            {
                byte |= coils_status[i+j] & 1;
            }
            printf("%x\n", byte);
        }
    }
/*        *temp_ptr = (coil_addr>>8)& '\xFF';
        temp_ptr++;
        *temp_ptr = (coil_addr)& '\xFF';
        temp_ptr++;
        *temp_ptr = '\xFF';
        temp_ptr++;
        *temp_ptr = '\x00';
        temp_ptr++;
        fill_packet('\x0F', request_ADU, DEFAULT_MSG_LEN);
        //temp_ptr = request_ADU->PDU_data;

        send_len = temp_ptr - send_buffer;
        //printf("sendlen%d req ADU %d elngth %d \n", send_len, sizeof(request_ADU), length);
        recv_len = HEADER_LEN;
        int receive_len = temp_ptr - request_ADU->PDU_data; //same as the sent data len
        int ret;
        if((ret = send_recv(sockfd, send_buffer, send_len, receive_buffer, receive_len))==UMODBUS_RECEIVE_ERROR)
        {
            ret_val = UMODBUS_RECEIVE_ERROR;
        }
        else
        {
            //memory comparison for the headers of the receive and the send buffers
            int result = memcmp(send_buffer, receive_buffer, HEADER_LEN);
            //printf("result%d\n",result );
                int response_size = receive_buffer[8];
                for(int i=1;i<=response_size;i++)
                {
                    int temp_byte = (receive_buffer[8+i]);
                    printf("%x\n", temp_byte);
                }
            if(result==1)
            {
                ret_val = UMODBUS_STATUS_SUCCESS;
            }
            else
            {
                ret_val = UMODBUS_RECEIVE_ERROR;
            }
        }
    }
    memset(memory_pool,0,2*MAX_MODBUS_FRAME_SIZE);
    request_ADU = memory_pool;
    send_buffer = memory_pool;
    response_ADU = memory_pool+MAX_MODBUS_FRAME_SIZE;
    receive_buffer = memory_pool+MAX_MODBUS_FRAME_SIZE;
    return ret_val;*/
}

/*uint32_t write_multiple_reg (uint16_t starting_addr,
                            uint16_t reg_count,
							uint16_t *reg_val,
							uint16_t reg_values_size){
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
        //int length = ceil(coil_count/8);

        uint8_t* temp_ptr = request_ADU->PDU_data;
        *temp_ptr = (coil_addr>>8)& '\xFF';
        temp_ptr++;
        *temp_ptr = (coil_addr)& '\xFF';
        temp_ptr++;
        if(coil_status==1)
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
        fill_packet('\x05', request_ADU, DEFAULT_MSG_LEN);
        //temp_ptr = request_ADU->PDU_data;

        send_len = temp_ptr - send_buffer;
        //printf("sendlen%d req ADU %d elngth %d \n", send_len, sizeof(request_ADU), length);
        recv_len = HEADER_LEN;
        int receive_len = temp_ptr - request_ADU->PDU_data; //same as the sent data len
        int ret;
        if((ret = send_recv(sockfd, send_buffer, send_len, receive_buffer, receive_len))==UMODBUS_RECEIVE_ERROR)
        {
            ret_val = UMODBUS_RECEIVE_ERROR;
        }
        else
        {
            //memory comparison for the headers of the receive and the send buffers
            int result = memcmp(send_buffer, receive_buffer, HEADER_LEN);
            //printf("result%d\n",result );
                int response_size = receive_buffer[8];
                for(int i=1;i<=response_size;i++)
                {
                    int temp_byte = (receive_buffer[8+i]);
                    printf("%x\n", temp_byte);
                }
            if(result==1)
            {
                ret_val = UMODBUS_STATUS_SUCCESS;
            }
            else
            {
                ret_val = UMODBUS_RECEIVE_ERROR;
            }
        }
    }
    memset(memory_pool,0,2*MAX_MODBUS_FRAME_SIZE);
    request_ADU = memory_pool;
    send_buffer = memory_pool;
    response_ADU = memory_pool+MAX_MODBUS_FRAME_SIZE;
    receive_buffer = memory_pool+MAX_MODBUS_FRAME_SIZE;
    return ret_val;
}

uint32_t mask_write_reg (uint16_t ref_addr,
                        uint16_t and_mask,
                        uint16_t or_mask){
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
        //int length = ceil(coil_count/8);

        uint8_t* temp_ptr = request_ADU->PDU_data;
        *temp_ptr = (coil_addr>>8)& '\xFF';
        temp_ptr++;
        *temp_ptr = (coil_addr)& '\xFF';
        temp_ptr++;
        if(coil_status==1)
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
        fill_packet('\x05', request_ADU, DEFAULT_MSG_LEN);
        //temp_ptr = request_ADU->PDU_data;

        send_len = temp_ptr - send_buffer;
        //printf("sendlen%d req ADU %d elngth %d \n", send_len, sizeof(request_ADU), length);
        recv_len = HEADER_LEN;
        int receive_len = temp_ptr - request_ADU->PDU_data; //same as the sent data len
        int ret;
        if((ret = send_recv(sockfd, send_buffer, send_len, receive_buffer, receive_len))==UMODBUS_RECEIVE_ERROR)
        {
            ret_val = UMODBUS_RECEIVE_ERROR;
        }
        else
        {
            //memory comparison for the headers of the receive and the send buffers
            int result = memcmp(send_buffer, receive_buffer, HEADER_LEN);
            //printf("result%d\n",result );
                int response_size = receive_buffer[8];
                for(int i=1;i<=response_size;i++)
                {
                    int temp_byte = (receive_buffer[8+i]);
                    printf("%x\n", temp_byte);
                }
            if(result==1)
            {
                ret_val = UMODBUS_STATUS_SUCCESS;
            }
            else
            {
                ret_val = UMODBUS_RECEIVE_ERROR;
            }
        }
    }
    memset(memory_pool,0,2*MAX_MODBUS_FRAME_SIZE);
    request_ADU = memory_pool;
    send_buffer = memory_pool;
    response_ADU = memory_pool+MAX_MODBUS_FRAME_SIZE;
    receive_buffer = memory_pool+MAX_MODBUS_FRAME_SIZE;
    return ret_val;
}

uint32_t rw_multiple_reg (uint16_t r_starting_addr,
                          uint16_t r_number,
                          uint16_t w_starting_addr,
                          uint16_t w_number,
                          uint16_t *w_reg_val,
						  uint16_t w_reg_val_size,
                          uint16_t *r_reg_val,
					  	  uint16_t r_reg_val_size){
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
        //int length = ceil(coil_count/8);

        uint8_t* temp_ptr = request_ADU->PDU_data;
        *temp_ptr = (coil_addr>>8)& '\xFF';
        temp_ptr++;
        *temp_ptr = (coil_addr)& '\xFF';
        temp_ptr++;
        if(coil_status==1)
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
        fill_packet('\x05', request_ADU, DEFAULT_MSG_LEN);
        //temp_ptr = request_ADU->PDU_data;

        send_len = temp_ptr - send_buffer;
        //printf("sendlen%d req ADU %d elngth %d \n", send_len, sizeof(request_ADU), length);
        recv_len = HEADER_LEN;
        int receive_len = temp_ptr - request_ADU->PDU_data; //same as the sent data len
        int ret;
        if((ret = send_recv(sockfd, send_buffer, send_len, receive_buffer, receive_len))==UMODBUS_RECEIVE_ERROR)
        {
            ret_val = UMODBUS_RECEIVE_ERROR;
        }
        else
        {
            //memory comparison for the headers of the receive and the send buffers
            int result = memcmp(send_buffer, receive_buffer, HEADER_LEN);
            //printf("result%d\n",result );
                int response_size = receive_buffer[8];
                for(int i=1;i<=response_size;i++)
                {
                    int temp_byte = (receive_buffer[8+i]);
                    printf("%x\n", temp_byte);
                }
            if(result==1)
            {
                ret_val = UMODBUS_STATUS_SUCCESS;
            }
            else
            {
                ret_val = UMODBUS_RECEIVE_ERROR;
            }
        }
    }
    memset(memory_pool,0,2*MAX_MODBUS_FRAME_SIZE);
    request_ADU = memory_pool;
    send_buffer = memory_pool;
    response_ADU = memory_pool+MAX_MODBUS_FRAME_SIZE;
    receive_buffer = memory_pool+MAX_MODBUS_FRAME_SIZE;
    return ret_val;
}

uint32_t read_FIFO_q (uint16_t  FIFO_ptr_addr,
					  uint16_t* FIFO_count,
					  uint32_t* FIFO_values){
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
        //int length = ceil(coil_count/8);

        uint8_t* temp_ptr = request_ADU->PDU_data;
        *temp_ptr = (coil_addr>>8)& '\xFF';
        temp_ptr++;
        *temp_ptr = (coil_addr)& '\xFF';
        temp_ptr++;
        if(coil_status==1)
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
        fill_packet('\x05', request_ADU, DEFAULT_MSG_LEN);
        //temp_ptr = request_ADU->PDU_data;

        send_len = temp_ptr - send_buffer;
        //printf("sendlen%d req ADU %d elngth %d \n", send_len, sizeof(request_ADU), length);
        recv_len = HEADER_LEN;
        int receive_len = temp_ptr - request_ADU->PDU_data; //same as the sent data len
        int ret;
        if((ret = send_recv(sockfd, send_buffer, send_len, receive_buffer, receive_len))==UMODBUS_RECEIVE_ERROR)
        {
            ret_val = UMODBUS_RECEIVE_ERROR;
        }
        else
        {
            //memory comparison for the headers of the receive and the send buffers
            int result = memcmp(send_buffer, receive_buffer, HEADER_LEN);
            //printf("result%d\n",result );
                int response_size = receive_buffer[8];
                for(int i=1;i<=response_size;i++)
                {
                    int temp_byte = (receive_buffer[8+i]);
                    printf("%x\n", temp_byte);
                }
            if(result==1)
            {
                ret_val = UMODBUS_STATUS_SUCCESS;
            }
            else
            {
                ret_val = UMODBUS_RECEIVE_ERROR;
            }
        }
    }
    memset(memory_pool,0,2*MAX_MODBUS_FRAME_SIZE);
    request_ADU = memory_pool;
    send_buffer = memory_pool;
    response_ADU = memory_pool+MAX_MODBUS_FRAME_SIZE;
    receive_buffer = memory_pool+MAX_MODBUS_FRAME_SIZE;
    return ret_val;
}

uint32_t encap_interface_transport (uint8_t MEI_type,
                                    uint16_t *MEI_type_data,
									uint32_t *MEI_type_data_size){
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
        //int length = ceil(coil_count/8);

        uint8_t* temp_ptr = request_ADU->PDU_data;
        *temp_ptr = (coil_addr>>8)& '\xFF';
        temp_ptr++;
        *temp_ptr = (coil_addr)& '\xFF';
        temp_ptr++;
        if(coil_status==1)
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
        fill_packet('\x05', request_ADU, DEFAULT_MSG_LEN);
        //temp_ptr = request_ADU->PDU_data;

        send_len = temp_ptr - send_buffer;
        //printf("sendlen%d req ADU %d elngth %d \n", send_len, sizeof(request_ADU), length);
        recv_len = HEADER_LEN;
        int receive_len = temp_ptr - request_ADU->PDU_data; //same as the sent data len
        int ret;
        if((ret = send_recv(sockfd, send_buffer, send_len, receive_buffer, receive_len))==UMODBUS_RECEIVE_ERROR)
        {
            ret_val = UMODBUS_RECEIVE_ERROR;
        }
        else
        {
            //memory comparison for the headers of the receive and the send buffers
            int result = memcmp(send_buffer, receive_buffer, HEADER_LEN);
            //printf("result%d\n",result );
                int response_size = receive_buffer[8];
                for(int i=1;i<=response_size;i++)
                {
                    int temp_byte = (receive_buffer[8+i]);
                    printf("%x\n", temp_byte);
                }
            if(result==1)
            {
                ret_val = UMODBUS_STATUS_SUCCESS;
            }
            else
            {
                ret_val = UMODBUS_RECEIVE_ERROR;
            }
        }
    }
    memset(memory_pool,0,2*MAX_MODBUS_FRAME_SIZE);
    request_ADU = memory_pool;
    send_buffer = memory_pool;
    response_ADU = memory_pool+MAX_MODBUS_FRAME_SIZE;
    receive_buffer = memory_pool+MAX_MODBUS_FRAME_SIZE;
    return ret_val;
}*/
