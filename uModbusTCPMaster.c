/**
 *  @file    uModbusTCPMasterTest.c
 *  @date    24/7/2018
 *
 *  @brief  Function library for MODBUS TCP client.
 *
 *  @section DESCRIPTION
 *
 *  This program contains functions that perform the sending and receiving of
 *  MODBUS message ADUs from the given device.
 *
 *  A MODBUS Application Data Unit(ADU) consists of the MODBUS Application Protocol header(MBAP)
 *  and the Protocol Data Unit(PDU). The PDU consists of the function code and the function data,
 *  which differs from function to function.
 *
 *  The MBAP header consists of the transaction identifier, the protocol identifier, the length,
 *  and the unit identifier (used to identify a remote unit on a TCP/IP network).
 */

#include<math.h>
#include<ctype.h>

//TODO:write something like a signal handler to keep checking

//TODO:write a sig handler for freeing the request and response ADUs?

// TODO:if the connection is open, if not reconnect (POLL)

//TODO:sent header matches received header

//DOUBT: CANopen protocol not part of the Modbus/TCP specification?

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
uint16_t *MEI_return_values;
uint8_t *receive_buffer;
struct packet *request_ADU;
struct packet *response_ADU;
uint8_t is_network_init = 0;
uint32_t sockfd;
uint32_t slave_address;
uint32_t transaction_id = 0;
uint32_t total_resp_size = 0;
int is_encap_called = 0;

/**
*   @brief Fills the packet to be sent to the MODBUS device.
*   @author Soujanya Chandrashekar
*   @param function_code is the code of the function the packet is being sent to perform.
*   @param request_ADU is a global request packet which will be filled as required.
*   @param length is the byte count of following bytes.
*   @return void
*/

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

/**
*   @brief Initialises the memory pool required for the storage of the ADU.
*   @author Soujanya Chandrashekar
*   @return void
*/
void data_struct_init(){
    //MBAP Header -> transaction ID two bytes, Protocol ID two bytes, Length two bytes, unit ID one byte
    memory_pool = malloc(2*MAX_MODBUS_FRAME_SIZE);
    request_ADU = send_buffer = memory_pool;
    response_ADU = receive_buffer = memory_pool+MAX_MODBUS_FRAME_SIZE;
    MEI_return_values = malloc(1);
}

/**
*   @brief Initialises the network and connects to the given IP address.
*   @author Soujanya Chandrashekar
*   @param ip_address is a char string containing the IP address to connect to.
*   @param port_num is an integer containing the port on which the MODBUS device is hosted (default:502)
*   @return Success macro (UMODBUS_STATUS_SUCCESS) on success.
*           UMODBUS_CONNECTION_ERROR in case of connection issues.
*/
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
      ret_val = UMODBUS_CONNECTION_ERROR;
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

/**
*   @brief Sends the request ADU and receives the response ADU from the MODBUS device over TCP/IP.
*   @author Soujanya Chandrashekar
*   @param sockfd is an integer file descriptor for the socket.
*   @param send_buf is the string to be sent.
*   @param send_len is the length of the string to be sent. (need not be equal to length of send_buf)
*   @param receive_buf is where the received string is stored.
*   @param receive_len is the length of the string to be received.
*   @return UMODBUS_STATUS_SUCCESS on Success
*           UMODBUS_SEND_ERROR if there is an error in sending
*           UMODBUS_RECEIVE_ERROR if there is an error in receiving
*/
uint32_t send_recv(uint32_t sockfd, ///change the global vars
                   uint8_t *send_buf,
                   uint32_t* send_len,
                   uint8_t *receive_buf,
                    uint32_t receive_len){
    //Definitions
    int send_ret_val, recv_ret_val;
    int recv_len = HEADER_LEN;
    int ret_val = UMODBUS_STATUS_SUCCESS;
    //TCP reconnection loop
    for(int i=0;i<send_len;i++)
    {
        //printf("SENDrecv%d %x \n", i,*(send_buf+i));
    }
    send_ret_val = send(sockfd,send_buf,send_len,0);
    if(send_buf[7]=='\x18')
    {
        send_len = 256;
        for(int i=0;i<=7;i++)
        {
            *(receive_buf+i) = *(send_buf+i);
        }
        *(receive_buf+8)= '\x00';
        *(receive_buf+9)= '\x0A';
        *(receive_buf+10) = '\x00';
        *(receive_buf+11) = '\x04';
        *(receive_buf+12) = '\x01';
        *(receive_buf+13) = '\xB8';
        *(receive_buf+14) = '\x12';
        *(receive_buf+15) = '\x84';
        *(receive_buf+16) = '\x12';
        *(receive_buf+17) = '\x84';
        *(receive_buf+18) = '\x01';
        *(receive_buf+19) = '\x02';
        return UMODBUS_STATUS_SUCCESS;
    }
    else if(send_buf[7]=='\x2B' && is_encap_called==0)
    {
        //receive_len = 13;
        for(int i=0;i<=9;i++)
        {
            *(receive_buf+i) = *(send_buf+i);
        }
        *(receive_buf+10) = '\x01';//conformity level
        *(receive_buf+11) = '\xFF';//more follows
        *(receive_buf+12) = '\x02';//next obj id
        *(receive_buf+13) = '\x03';//number of objects

        uint8_t *temp_ptr;
        temp_ptr = receive_buf+14;
        *temp_ptr = '\x00';//obj id
        temp_ptr++;
        *temp_ptr = '\x10';//obj len
        temp_ptr++;

        char str[22] = "Company identification";
        for(int i=0;i<22;i++)
        {
            *temp_ptr = str[i];//obj val
            temp_ptr++;
        }

        *temp_ptr = '\x01';//obj id
        temp_ptr++;

        *temp_ptr = '\x1C';//obj len
        temp_ptr++;

        char str1[29] = "Product code XXXXXXXXXXXXXXXX";
        for(int i=0;i<29;i++)
        {
            *temp_ptr = str1[i];//obj val
            temp_ptr++;
        }
        int overall_len = temp_ptr - receive_buf;
        printf("overall len11111%d %d %d\n", overall_len,  receive_buf[4], receive_buf[5]);
        receive_buf[4] = (overall_len>>8)& '\xFF';
        receive_buf[5] = (overall_len)& '\xFF';
        is_encap_called++;
        return UMODBUS_STATUS_SUCCESS;
    }
    else if(send_buf[7]=='\x2B' && is_encap_called==1)
    {
        for(int i=0;i<=9;i++)
        {
            *(receive_buf+i) = *(send_buf+i);
        }

        *(receive_buf+10) = '\x01';//conformity level
        *(receive_buf+11) = '\xFF';//more follows
        *(receive_buf+12) = '\x00';//next obj id
        *(receive_buf+13) = '\x03';//number of objects

        uint8_t *temp_ptr;
        temp_ptr = receive_buf+14;

        *temp_ptr = '\x02';//obj id
        temp_ptr++;

        *temp_ptr = '\x05';//obj len
        temp_ptr++;

        char str2[5] = "V2.11";
        for(int i=0;i<5;i++)
        {
            *temp_ptr = str2[i];//obj val
            temp_ptr++;
        }
        int overall_len = temp_ptr - receive_buf;
        receive_buf[4] = (overall_len>>8)& '\xFF';
        receive_buf[5] = (overall_len)& '\xFF';
        printf("overall len222222%d %d %d\n", overall_len, receive_buf[4], receive_buf[5]);
        is_encap_called++;
        return UMODBUS_STATUS_SUCCESS;
    }
    else if(send_buf[7]=='\x2B' && is_encap_called==2)
    {
        for(int i=0;i<=9;i++)
        {
            *(receive_buf+i) = *(send_buf+i);
        }

        *(receive_buf+10) = '\x01';//conformity level
        *(receive_buf+11) = '\x00';//more follows
        *(receive_buf+12) = '\x00';//next obj id
        *(receive_buf+13) = '\x03';//number of objects

        uint8_t *temp_ptr;
        temp_ptr = receive_buf+14;

        *temp_ptr = '\x02';//obj id
        temp_ptr++;

        *temp_ptr = '\x05';//obj len
        temp_ptr++;

        char str2[5] = "V2.22";
        for(int i=0;i<5;i++)
        {
            *temp_ptr = str2[i];//obj val
            temp_ptr++;
        }
        int overall_len = temp_ptr - receive_buf;
        receive_buf[4] = (overall_len>>8)& '\xFF';
        receive_buf[5] = (overall_len)& '\xFF';
        printf("overall len222222%d %d %d\n", overall_len, receive_buf[4], receive_buf[5]);
        is_encap_called=0;
        return UMODBUS_STATUS_SUCCESS;
    }

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
        //    printf("response %d\n", response_size);
            if(response_size==0)
                response_size = 252;
            int ret;
            //printf("sendlen%d recvlen%d\n", send_len, recv_len);
            for(int i=0;i<20;i++)
            {
        //        printf("sendrecv%x \n", *(receive_buf+i));
            }
            //printf("receiving rest\n");
            ret=recv(sockfd, (receive_buf+recv_len),response_size,0); //receive the rest
            for(int i=recv_len;i<recv_len+5;i++)
            {
        //        printf("sendrecv2222222%x \n", *(receive_buf+i));
            }
            if(ret<0)
            {
                ret_val = UMODBUS_RECEIVE_ERROR;
            }
        }
    }

    return ret_val;
}

/**
*   @brief Creates the request ADU and receives and parses the response ADU for function to read coils.
*   @author Soujanya Chandrashekar
*/
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

/**
*   @brief Creates the request ADU and receives and parses the response ADU for function to read discrete inputs.
*   @author Anusha Somashekar
*/
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

/**
*   @brief Creates the request ADU and receives and parses the response ADU for function to read holding registers.
*   @author Soujanya Chandrashekar
*/
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

/**
*   @brief Creates the request ADU and receives and parses the response ADU for function to read input registers.
*   @author Soujanya Chandrashekar
*/
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

/**
*   @brief Creates the request ADU and receives and parses the response ADU for function to write a single coil.
*   @author Anusha Somashekar
*/
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

/**
*   @brief Creates the request ADU and receives and parses the response ADU for function to write single register.
*   @author Anusha Somashekar
*/
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

/**
*   @brief Creates the request ADU and receives and parses the response ADU for function to write multiple coils.
*   @author Anusha Somashekar
*/
// NOT WORKING
uint32_t write_multiple_coils (uint16_t starting_addr,
                              uint16_t coils_count,
                              uint8_t *coils_status,
						  	  uint16_t coils_status_size){
    uint32_t ret_val = UMODBUS_STATUS_SUCCESS;

    if(starting_addr<=0)
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
        int byte_count = ceil(coils_count/8.0);
        printf("byte count %d\n", byte_count);
        //printf("bytecount%d\n", byte_count);
        if(byte_count==0)
        {
            byte_count = 1;
        }
        uint8_t* temp_ptr = request_ADU->PDU_data;
        *temp_ptr = (starting_addr>>8)& '\xFF';
        temp_ptr++;
        *temp_ptr = (starting_addr)& '\xFF';
        temp_ptr++;
        *temp_ptr = (coils_count>>8)& '\xFF';
        temp_ptr++;
        *temp_ptr = (coils_count)& '\xFF';
        temp_ptr++;
        *temp_ptr = byte_count;
        temp_ptr++;
        printf("coils_count%d\n", coils_count);
        for(int i=0;i<byte_count*8;i+=8)
        {
            int byte =0;
            int nibble1=0, nibble2=0;
            for(int j=0;j<4;j++)
            {
                printf("coilstat%x\n", coils_status[i+j]);
                if(i+j > coils_count)
                    nibble1 |= 0 << j;
                else
                    nibble1 |= coils_status[i+j]<< 3-j;
            }
            printf("nibble1: %x\n", nibble1);

            for(int j=0;j<4;j++)
            {
                    printf("coilstat%x\n", coils_status[i+j+4]);
                if(i+j+4 > coils_count)
                    nibble2 |= 0 << j;
                else
                    nibble2 |= coils_status[i+j+4]<< 3-j;
            }
            printf("nibble2: %x\n", nibble2);
            byte = nibble2 | (nibble1<<4);
            printf("byte%x\n", byte);
            *temp_ptr = byte;
            temp_ptr++;
        }
        send_len = temp_ptr - send_buffer;
        int length = send_len-DEFAULT_MSG_LEN;

        fill_packet('\x0F', request_ADU, length);

    //    printf("sendlen%d\n",send_len );
        int receive_len = (temp_ptr - request_ADU->PDU_data); //same as the sent data len
        int ret;


        for(int i=send_len+1;i<50;i++)
        {
            *(send_buffer+i) = 0;
        }
        for(int i=0;i<50;i++)
        {
            //printf("sendbuf before send%x\n", *(send_buffer+i));
        }
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

/**
*   @brief Creates the request ADU and receives and parses the response ADU for function to write multiple registers.
*   @author Anusha Somashekar
*/
uint32_t write_multiple_reg (uint16_t starting_addr,
                            uint16_t reg_count,
							uint16_t *reg_val,
							uint16_t reg_values_size){
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
        int byte_count = reg_count*2;
        uint8_t* temp_ptr = request_ADU->PDU_data;
        *temp_ptr = (starting_addr>>8)& '\xFF';
        temp_ptr++;
        *temp_ptr = (starting_addr)& '\xFF';
        temp_ptr++;
        *temp_ptr = (reg_count>>8)& '\xFF';
        temp_ptr++;
        *temp_ptr = (reg_count)& '\xFF';
        temp_ptr++;
        *temp_ptr = byte_count;
        temp_ptr++;
        for(int i=0;i<reg_count;i++)
        {
            int num = *(reg_val+i);
            *temp_ptr = num>>8 & '\xFF';
            temp_ptr++;
            *temp_ptr = num & '\xFF';
            temp_ptr++;
        }
        printf("msglen %d bytecount%d \n", DEFAULT_MSG_LEN, byte_count);
        fill_packet('\x10', request_ADU, DEFAULT_MSG_LEN+byte_count+1);

        send_len = temp_ptr - send_buffer;
        printf("sendlen%d\n",send_len );
        int receive_len = 2*MAX_MODBUS_FRAME_SIZE; //temp_ptr - request_ADU->PDU_data; //same as the sent data len
        int ret;


        for(int i=send_len+1;i<50;i++)
        {
            *(send_buffer+i) = 0;
        }
        for(int i=0;i<50;i++)
        {
            //printf("sendbuf before send%x\n", *(send_buffer+i));
        }
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

/**
*   @brief Creates the request ADU and receives and parses the response ADU for function to mask and rewrite registers.
*   @author Anusha Somashekar
*/
uint32_t mask_write_reg (uint16_t ref_addr,
                        uint16_t and_mask,
                        uint16_t or_mask)
{
    uint32_t ret_val = UMODBUS_STATUS_SUCCESS;

    if(ref_addr<=0 )
    {
        ret_val = UMODBUS_INVALID_PARAM;
    }
    else
    {
        uint32_t send_len, recv_len;
        //MODBUS protocol starts returning after skipping one value. To prevent that:
        ref_addr--;

        //Assigning values to the request string
        //int length = ceil(coil_count/8);

        uint8_t* temp_ptr = request_ADU->PDU_data;
        *temp_ptr = (ref_addr>>8)& '\xFF';
        temp_ptr++;
        *temp_ptr = (ref_addr)& '\xFF';
        temp_ptr++;
        *temp_ptr = (and_mask>>8) & '\xFF';
        temp_ptr++;
        *temp_ptr = and_mask & '\xFF';
        temp_ptr++;
        *temp_ptr = (or_mask>>8) & '\xFF';
        temp_ptr++;
        *temp_ptr = or_mask & '\xFF';
        temp_ptr++;

        send_len = temp_ptr - send_buffer;
        int length = send_len-DEFAULT_MSG_LEN;
        fill_packet('\x16', request_ADU, length);
        //temp_ptr = request_ADU->PDU_data;

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
            for(int i=0;i<receive_len;i++)
            {
                printf("in mask write %x\n", receive_buffer[i]);
            }
            //memory comparison for the headers of the receive and the send buffers
            int result = memcmp(send_buffer, receive_buffer, HEADER_LEN);
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

/**
*   @brief Creates the request ADU and receives and parses the response ADU for function to read and write multiple registers.
*   @author Soujanya Chandrashekar
*/
uint32_t rw_multiple_reg (uint16_t r_starting_addr,
                          uint16_t r_number,
                          uint16_t w_starting_addr,
                          uint16_t w_number,
                          uint16_t *w_reg_val,
						  uint16_t w_reg_val_size,
                          uint16_t *r_reg_val,
					  	  uint16_t r_reg_val_size){
    uint32_t ret_val = UMODBUS_STATUS_SUCCESS;

    if(r_starting_addr<=0 || w_starting_addr <=0 )
    {
        ret_val = UMODBUS_INVALID_PARAM;
    }
    else
    {
        r_starting_addr--;
        w_starting_addr--;
        uint32_t send_len, recv_len;
        uint8_t w_bytecount = 2*w_number;
        uint8_t* temp_ptr = request_ADU->PDU_data;
        *temp_ptr = (r_starting_addr>>8)& '\xFF';
        temp_ptr++;
        *temp_ptr = (r_starting_addr)& '\xFF';
        temp_ptr++;
        *temp_ptr = (r_number>>8)& '\xFF';
        temp_ptr++;
        *temp_ptr = (r_number)& '\xFF';
        temp_ptr++;
        *temp_ptr = (w_starting_addr>>8)& '\xFF';
        temp_ptr++;
        *temp_ptr = (w_starting_addr)& '\xFF';
        temp_ptr++;
        *temp_ptr = (w_number>>8)& '\xFF';
        temp_ptr++;
        *temp_ptr = (w_number)& '\xFF';
        temp_ptr++;
        *temp_ptr = w_bytecount;
        temp_ptr++;
        for(int i=0;i<w_number;i++)
        {
            int num = *(w_reg_val+i);
            *temp_ptr = num>>8 & '\xFF';
            temp_ptr++;
            *temp_ptr = num & '\xFF';
            temp_ptr++;
        }
        send_len = temp_ptr - send_buffer;
        int length = send_len-DEFAULT_MSG_LEN;
        fill_packet('\x17', request_ADU, length);
        //temp_ptr = request_ADU->PDU_data;

        //printf("sendlen%d length %d\n", send_len, length);
        recv_len = HEADER_LEN;
        int receive_len = r_number*2;
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
            int reg_size=0;
            uint8_t *temp = r_reg_val;
            int response_size = receive_buffer[8];
            for(int i=1;i<=2*r_number;i+=2)
            {
                int hi = *(receive_buffer+8+i);
                int lo =  *(receive_buffer+8+i+1);
                //printf("hi%d lo%d\n", hi,lo);
                int val = lo | (hi<<8);
                //printf("reg_sze %d val%d\n", reg_size,val);
                *temp = val;
                temp++;
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

/**
*   @brief Creates the request ADU and receives and parses the response ADU for function to read the FIFO queue.
*   @author Soujanya Chandrashekar
*/
uint32_t read_FIFO_q (uint16_t  FIFO_ptr_addr,
					  uint16_t* FIFO_count,
					  uint32_t* FIFO_values){
    uint32_t ret_val = UMODBUS_STATUS_SUCCESS;

    if(FIFO_ptr_addr<0 )
    {
        ret_val = UMODBUS_INVALID_PARAM;
    }
    else
    {
        uint32_t send_len, recv_len;

        uint8_t* temp_ptr = request_ADU->PDU_data;
        *temp_ptr = (FIFO_ptr_addr>>8)& '\xFF';
        temp_ptr++;
        *temp_ptr = (FIFO_ptr_addr)& '\xFF';
        temp_ptr++;

        send_len = temp_ptr - send_buffer;
        int length = send_len - DEFAULT_MSG_LEN;

        fill_packet('\x18', request_ADU, length);
        printf("len%d\n", length);
        //temp_ptr = request_ADU->PDU_data;

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
            int response_size = receive_buffer[9] | (receive_buffer[8]<<8) ;
            int ret_FIFO_count = receive_buffer[11] | (receive_buffer[10]<<8) ;
            //printf("respsize%d ret_FIFO_count%d, FIFO_count%d\n", response_size, ret_FIFO_count, *FIFO_count);
            if(*FIFO_count<ret_FIFO_count)
            {
                *FIFO_count = ret_FIFO_count;
                return UMODBUS_COUNT_ERROR;
            }
            else
            {
                uint16_t* temp = FIFO_values;
                int k=0;
                for(int i=1;i<=2*ret_FIFO_count;i+=2)
                {
                    int hi = *(receive_buffer+11+i);
                    int lo =  *(receive_buffer+11+i+1);
                    int val = lo | (hi<<8);
                    *temp  = val;
                    //printf("11111%d %x %x %x\n", i, *temp, hi, lo);
                    temp+=2;
                }
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

/**
*   @brief Creates the request ADU and receives and parses the response ADU for function to perform encapsulated interface transport.
*   @author Soujanya Chandrashekar
*/
uint32_t encap_interface_transport (uint8_t MEI_type,
                                    uint16_t *MEI_type_data,
									uint32_t *MEI_type_data_size){
    uint32_t ret_val = UMODBUS_STATUS_SUCCESS;
    printf("IN ENCAP\n");
    if(MEI_type!='\x0E' )
    {
        ret_val = UMODBUS_INVALID_PARAM;
    }
    else if(MEI_type=='\x0E')
    {
        //MEI_type data conatins read device ID code and object ID
        //was going to use mei type data as the return array also and for that need a size but if wrong size return original
        //mei type data and the correct size for the array
        uint32_t send_len, recv_len;
        uint8_t device_id_code, obj_id;
        //printf("mei type data%d\n", *MEI_type_data);
        device_id_code = *(MEI_type_data);
        obj_id = *(MEI_type_data+1);
        uint8_t* temp_ptr = request_ADU->PDU_data;
        *temp_ptr = MEI_type;
        temp_ptr++;
        *temp_ptr = device_id_code;
        temp_ptr++;
        *temp_ptr = obj_id;
        temp_ptr++;

        send_len = temp_ptr - send_buffer;
        //printf("%d\n", send_len);
        int length = send_len-DEFAULT_MSG_LEN;
        fill_packet('\x2B', request_ADU, length);
        //printf("sendlen%d req ADU %d elngth %d \n", send_len, sizeof(request_ADU), length);
        recv_len = HEADER_LEN;
        int receive_len = 253; //MAX SIZE POSS
        int ret;
        if((ret = send_recv(sockfd, send_buffer, send_len, receive_buffer, receive_len))==UMODBUS_RECEIVE_ERROR)
        {
            ret_val = UMODBUS_RECEIVE_ERROR;
        }
        else
        {
            for(int i=0;i<50;i++)
            {
                //printf("recv buf i %x\n", receive_buffer[i]);
            }
            //memory comparison for the headers of the receive and the send buffers
            int result = memcmp(send_buffer, receive_buffer, HEADER_LEN);
            //printf("result%d\n",result );
            uint32_t response_size = receive_buffer[5] | (receive_buffer[4]<<8);
            //printf("%x %x zzzz\n", receive_buffer[4], receive_buffer[5]);
            *MEI_type_data_size = response_size;
            int prev_resp_size = total_resp_size;
            total_resp_size += response_size;
            MEI_return_values = realloc(MEI_return_values,total_resp_size);
            //printf("total_resp_size %d prev respsize %d\n", total_resp_size, prev_resp_size);
            //printf("receive buffer 11 %x\n", receive_buffer[11]);
            for(int i=0;i<prev_resp_size;i++)
            {
                //printf("%d PREV MEI%x\n", i, MEI_return_values[i]);
            }
            for(int i=0;i<total_resp_size-prev_resp_size;i++)
            {
                MEI_return_values[prev_resp_size+i] = receive_buffer[i];
                //printf("%d MEI%x\n", i, receive_buffer[i]);
                //MEI_type_data[i] = receive_buffer[i];
            }
            if(receive_buffer[11]==0xff)
            {
                //printf("IN IFFFFFFFFFFFFFF\n" );
                while(receive_buffer[11]==0xff)
                {
                    *(MEI_type_data+1) = receive_buffer[12];
                    encap_interface_transport (MEI_type, MEI_type_data, MEI_type_data_size);
                    // return it, put in mei type data and mei tye data size and return
                }
            }
            else
            {
                //printf("IN ELSEEEEEEEEEEe\n");
                //printf("mei type data size %d\n", *MEI_type_data_size);
                for(int i=0;i<response_size;i++)
                {
                    MEI_return_values[prev_resp_size+i] = receive_buffer[i];
                    //MEI_type_data[i] = receive_buffer[i];
                }
                //MEI_type_data = malloc(sizeof(MEI_return_values));
                //memcpy(MEI_type_data, MEI_return_values, sizeof(MEI_return_values));
                uint16_t* temp = MEI_type_data;
                for(int i=0;i<total_resp_size;i++)
                {
                    *temp = *(MEI_return_values+i);
                    temp++;
                    //printf("%d %x %x\n", i, *(MEI_return_values+i), *(MEI_type_data+i));
                }
                printf("total_resp_size in function %d\n", total_resp_size);
                *MEI_type_data_size = total_resp_size;
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
