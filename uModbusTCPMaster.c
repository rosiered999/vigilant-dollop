#include<math.h>
#include<ctype.h>

//TODO:write something like a signal handler to keep checking

//TODO:write a sig handler for freeing the request and response ADUs?

// TODO:if the connection is open, if not reconnect (POLL)

/*TODO:packed structure in send and recv research; struct with all pointers if no packed
https://stackoverflow.com/questions/8568432/is-gccs-attribute-packed-pragma-pack-unsafe*/

//TODO:pased headerlen from which byte len is there size of len, default; define as macros

//TODO:length in header matches bytes ret

//never make a pointer to a packed struct https://stackoverflow.com/questions/1756811/does-gccs-attribute-packed-retain-the-original-ordering/7956942#7956942
/* PACKED STRUCT NOTES
Yes, __attribute__((packed)) is potentially unsafe on some systems. The symptom probably won't show up on an x86, which just makes the problem more insidious;
 testing on x86 systems won't reveal the problem. (On the x86, misaligned accesses are handled in hardware; if you dereference an int* pointer
      that points to an odd address, it will be a little slower than if it were properly aligned, but you'll get the correct result.)
On some other systems, such as SPARC, attempting to access a misaligned int object causes a bus error, crashing the program.
https://stackoverflow.com/questions/8568432/is-gccs-attribute-packed-pragma-pack-unsafe
This is a very bad idea. Binary data should always be sent in a way that:
Handles different endianness
Handles different padding
Handles differences in the byte-sizes of intrinsic types
Don't ever write a whole struct in a binary way, not to a file, not to a socket.
Always write each field separately, and read them the same way.
https://stackoverflow.com/questions/1577161/passing-a-structure-through-sockets-in-c
What malloc returns depends on the implementation of malloc, and the architecture. As others have already said, you are guaranteed to get at
 LEAST the requested amount of memory, or NULL.
https://stackoverflow.com/questions/430163/why-does-malloc-allocate-a-different-number-of-bytes-than-requested
*/

//remove length from struct, data single yte array; send base will be packet pointers; change data[1] but pointer stuff same
// in init put send packet pointer to send and recv to recv buff
//mm=em compariosn for header len
struct packet {
    //GCC Specific
    //
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
#define MAX_MODBUS_FRAME_SIZE 260

//Global Definitions
uint8_t *memory_pool;
uint8_t *send_buffer;
uint8_t *receive_buffer;
struct packet *request_ADU; //I only need this inside the function, so no pointer also pointers not recommmended
struct packet *response_ADU;
uint8_t is_network_init = 0;
uint32_t sockfd;
uint32_t slave_address;
uint32_t transaction_id = 0; //pairing req and response, synchronous,
//fill header function, slave,

void fill_packet(uint8_t function_code, struct packet *request_ADU) //fill header pass a packet pointer; packet pte to send buf in function
//fill data fields in function
{
    request_ADU->transaction_id_hi = (transaction_id>>8)& '\xFF';
    request_ADU->transaction_id_lo = (transaction_id)& '\xFF';
    transaction_id++;
    request_ADU->protocol_id_hi = '\x00';
    request_ADU->protocol_id_lo = '\x00';
    request_ADU->length_hi = '\x00';
    request_ADU->length_lo = '\x06';
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

uint32_t send_recv(uint32_t sockfd,
                   /*uint32_t* buf,*/
                   uint8_t *send_buffer,
                   uint32_t* send_len,
                   uint8_t *receive_buffer){
    //Definitions
    int send_ret_val, recv_ret_val;
    int recv_len = HEADER_LEN;
    int ret_val = UMODBUS_STATUS_SUCCESS;
    //TCP reconnection loop
    if(send_len!=0 && ((send_ret_val=send(sockfd,send_buffer,send_len,0)) < 0)) //or strlen(request_ADU != last_pos_request)
    {
        ret_val = UMODBUS_SEND_ERROR;
    }
    else if(send_len==0 || ((send_ret_val=send(sockfd,send_buffer,send_len,0)) > 0))
    {
        if ((recv_ret_val = recv(sockfd, receive_buffer, recv_len, 0)) <0)
        {
           perror("recv");
           ret_val = UMODBUS_RECEIVE_ERROR;
        }
        else
        {
            //printf("receiving...\n");
            //response_ADU[recv_ret_val] = '\0';
            printf("recv ret val in send rev%d\n", recv_ret_val);
            ret_val = UMODBUS_STATUS_SUCCESS;
        }
    }
    int response_size = receive_buffer[8];
    int ret;
    if((ret=recv(sockfd, (receive_buffer+recv_len),response_size,0))<0)
    {
        ret_val = UMODBUS_RECEIVE_ERROR;
    }
    return ret_val;
}

/*uint32_t read_coils (uint16_t starting_addr,
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

        send_len = (temp_ptr - request_ADU);
        recv_len = HEADER_LEN;
        printf("sendlen%d\n", send_len);
        for(int i=0;i<send_len;i++)
        {
            printf("%x\n", *(request_ADU+i));
        }

        int ret;
        if((ret = send_recv(sockfd, request_ADU, send_len, response_ADU))==UMODBUS_RECEIVE_ERROR)
        {
            ret_val = UMODBUS_RECEIVE_ERROR;
        }
        else
        {
            send_len = 0;
            int coils_status_index=0;
            //Filling the response string
            int response_size = response_ADU[8];
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
    memset(total_ADU,0,2*MAX_MODBUS_FRAME_SIZE);
    request_ADU = total_ADU;
    response_ADU = total_ADU+MAX_MODBUS_FRAME_SIZE;
    return ret_val;
}*/

uint32_t read_coils1 (uint16_t starting_addr,
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
        int length = 4;

        uint8_t* temp_ptr = request_ADU->PDU_data;
        *temp_ptr = (starting_addr>>8)& '\xFF';
        temp_ptr++;
        *temp_ptr = (starting_addr)& '\xFF';
        temp_ptr++;
        *temp_ptr = (coil_count>>8)& '\xFF';
        temp_ptr++;
        *temp_ptr = (coil_count)& '\xFF';
        //temp_ptr++;

        fill_packet('\x01', request_ADU);

        printf("%x %x %x %x %x %x %x %x \n",
        request_ADU->transaction_id_hi, request_ADU->transaction_id_lo,
        request_ADU->protocol_id_hi, request_ADU->protocol_id_lo,
        request_ADU->length_hi, request_ADU->length_lo,
        request_ADU->slave_address, request_ADU->function_code);

        temp_ptr = request_ADU->PDU_data;

        send_len = sizeof(request_ADU)+length;
        recv_len = HEADER_LEN;
        //printf("sendlen%d\n", send_len);

        int ret;
        if((ret = send_recv(sockfd, send_buffer, send_len, receive_buffer))==UMODBUS_RECEIVE_ERROR)
        {
            ret_val = UMODBUS_RECEIVE_ERROR;
        }
        else
        {
            send_len = 0;
            /*printf("%x %x %x %x %x %x %x %x\n",
            response_ADU->transaction_id_hi, response_ADU->transaction_id_lo,
            response_ADU->protocol_id_hi, response_ADU->protocol_id_lo,
            response_ADU->length_hi, response_ADU->length_lo,
            response_ADU->slave_address, response_ADU->function_code);
            */

            for(int i=0;i<4;i++)
            {
                //printf("%x\n", *(receive_buffer+8+i));
            }
            send_len = 0;
            int coils_status_index=0;
            //Filling the response string
            int response_size = receive_buffer[8];
            for(int i=1;i<=response_size;i++)
            {
                int temp_byte = (receive_buffer[8+i]);
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
    memset(memory_pool,0,2*MAX_MODBUS_FRAME_SIZE);
    request_ADU = memory_pool;
    response_ADU = memory_pool+MAX_MODBUS_FRAME_SIZE;
    return ret_val;
}
