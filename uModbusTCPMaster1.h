int msg_len = 12;
uint32_t sockfd;
int slave_address;

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
    if(sockfd==0)
        return UMODBUS_CONNECTION_ERROR;

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
    starting_addr--;
    uint8_t* buf = malloc(14);
    buf[0] = '\x00';
    buf[1] = '\x01';
    buf[2] = '\x00';
    buf[3] = '\x00';
    buf[4] = '\x00';
    buf[5] = '\x06';
    buf[6] = slave_address;
    buf[7] = '\x01';
    buf[8] = (starting_addr>>8)& '\xFF';
    buf[9] = (starting_addr)& '\xFF';
    buf[10] = (coil_count>>8)& '\xFF';
    buf[11] = (coil_count)& '\xFF';
    buf[12] = '\x00';
    uint32_t send_ret_val;

    if((send_error_check(sockfd, buf, send_ret_val)==UMODBUS_SEND_ERROR))
        return UMODBUS_SEND_ERROR;
    //printf("sent\n");
    uint8_t* reply = malloc(200);
    uint32_t recv_ret_val;
    if(recv_error_check(sockfd,reply,&recv_ret_val)==UMODBUS_RECEIVE_ERROR)
        return UMODBUS_RECEIVE_ERROR;
    //printf("received\n");
    int x = 1, k=0;
    for(int i=1;i<=reply[8];i++)
    {
        int b = (reply[8+i]);
        for(int j=1;j<=8;j++)
        {
            char p = ((j==8)?'\n':' ');
            int hi = (b)&1;
            coils_status[k] = hi;
            k++;
            b = b>>1;
        }
    }
}

uint32_t read_discrete_inputs (uint16_t starting_addr,
                               uint16_t inputs_count,
                               uint8_t *inputs_status,
                               uint8_t  inputs_status_size)
{
    starting_addr--;
    uint8_t* buf = malloc(14);
    buf[0] = '\x00';
    buf[1] = '\x01';
    buf[2] = '\x00';
    buf[3] = '\x00';
    buf[4] = '\x00';
    buf[5] = '\x06';
    buf[6] = slave_address;
    buf[7] = '\x02';
    buf[8] = (starting_addr>>8)& '\xFF';
    buf[9] = (starting_addr)& '\xFF';
    buf[10] = (inputs_count>>8)& '\xFF';
    buf[11] = (inputs_count)& '\xFF';
    buf[12] = '\x00';
    uint32_t send_ret_val;

    if((send_error_check(sockfd, buf, send_ret_val)==UMODBUS_SEND_ERROR))
        return UMODBUS_SEND_ERROR;
    //printf("sent\n");
    uint8_t* reply = malloc(200);
    uint32_t recv_ret_val;
    if(recv_error_check(sockfd,reply,&recv_ret_val)==UMODBUS_RECEIVE_ERROR)
        return UMODBUS_RECEIVE_ERROR;
    //printf("received\n");
    int x = 1, k=0;
    for(int i=1;i<=reply[8];i++)
    {
        int b = (reply[8+i]);
        //printf("%d\n", b);
        for(int j=1;j<=8;j++)
        {
            char p = ((j==8)?'\n':' ');
            int hi = (b)&1;
            printf("%d\n", hi);
            inputs_status[k] = hi;
            k++;
            b = b>>1;
        }
        x+=8;
    }
}

uint32_t read_holding_reg (uint16_t starting_addr,
                               uint16_t reg_count,
                               uint8_t *reg_values,
                               uint8_t  reg_values_size)
{
    starting_addr--;
    uint8_t* buf = malloc(14);
    buf[0] = '\x00';
    buf[1] = '\x01';
    buf[2] = '\x00';
    buf[3] = '\x00';
    buf[4] = '\x00';
    buf[5] = '\x06';
    buf[6] = slave_address;
    buf[7] = '\x03';
    buf[8] = (starting_addr>>8)& '\xFF';
    buf[9] = (starting_addr)& '\xFF';
    buf[10] = (reg_count>>8)& '\xFF';
    buf[11] = (reg_count)& '\xFF';
    buf[12] = '\x00';
    uint32_t send_ret_val;

    if((send_error_check(sockfd, buf, send_ret_val)==UMODBUS_SEND_ERROR))
        return UMODBUS_SEND_ERROR;
    //printf("sent\n");
    uint8_t* reply = malloc(200);
    uint32_t recv_ret_val;
    if(recv_error_check(sockfd,reply,&recv_ret_val)==UMODBUS_RECEIVE_ERROR)
        return UMODBUS_RECEIVE_ERROR;
    //printf("received\n");
    int x = 1, k=0;
    for(int i=1;i<=reply[8];i+=2)
    {
        int hi = (reply[8+i]);
        int lo = (reply[8+i+1]);
        int res = lo | (hi<<8);
        reg_values[k] = res;
        k++;
    }
}

uint32_t read_input_reg (uint16_t starting_addr,
                               uint16_t reg_count,
                               uint8_t *reg_values,
                               uint8_t  reg_values_size)
{
    starting_addr--;
    uint8_t* buf = malloc(14);
    buf[0] = '\x00';
    buf[1] = '\x01';
    buf[2] = '\x00';
    buf[3] = '\x00';
    buf[4] = '\x00';
    buf[5] = '\x06';
    buf[6] = slave_address;
    buf[7] = '\x04';
    buf[8] = (starting_addr>>8)& '\xFF';
    buf[9] = (starting_addr)& '\xFF';
    buf[10] = (reg_count>>8)& '\xFF';
    buf[11] = (reg_count)& '\xFF';
    buf[12] = '\x00';
    uint32_t send_ret_val;

    if((send_error_check(sockfd, buf, send_ret_val)==UMODBUS_SEND_ERROR))
        return UMODBUS_SEND_ERROR;
    //printf("sent\n");
    uint8_t* reply = malloc(200);
    uint32_t recv_ret_val;
    if(recv_error_check(sockfd,reply,&recv_ret_val)==UMODBUS_RECEIVE_ERROR)
        return UMODBUS_RECEIVE_ERROR;
    //printf("received\n");
    int x = 1, k=0;
    for(int i=1;i<=reply[8];i+=2)
    {
        int hi = (reply[8+i]);
        int lo = (reply[8+i+1]);
        int res = lo | (hi<<8);
        reg_values[k] = res;
        k++;
    }
}

uint32_t write_single_coil (uint16_t coil_addr, uint16_t coil_status)
{
    coil_addr--;
    uint8_t* buf = malloc(14);
    buf[0] = '\x00';
    buf[1] = '\x01';
    buf[2] = '\x00';
    buf[3] = '\x00';
    buf[4] = '\x00';
    buf[5] = '\x06';
    buf[6] = slave_address;
    buf[7] = '\x05';
    buf[8] = (coil_addr>>8)& '\xFF';
    buf[9] = (coil_addr)& '\xFF';

    if(coil_status == 1)
    {
        buf[10] = '\xFF';
        buf[11] = '\x00';
    }
    else
    {
        buf[10] = '\x00';
        buf[11] = '\x00';
    }

    buf[12] = '\x00';
    uint32_t send_ret_val;

    if((send_error_check(sockfd, buf, send_ret_val)==UMODBUS_SEND_ERROR))
        return UMODBUS_SEND_ERROR;
    //printf("sent\n");
    uint8_t* reply = malloc(200);
    uint32_t recv_ret_val;
    if(recv_error_check(sockfd,reply,&recv_ret_val)==UMODBUS_RECEIVE_ERROR)
        return UMODBUS_RECEIVE_ERROR;
    //printf("received\n");
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

    if((send_error_check(sockfd, buf, send_ret_val)==UMODBUS_SEND_ERROR))
        return UMODBUS_SEND_ERROR;
    //printf("sent\n");
    uint8_t* reply = malloc(200);
    uint32_t recv_ret_val;
    if(recv_error_check(sockfd,reply,&recv_ret_val)==UMODBUS_RECEIVE_ERROR)
        return UMODBUS_RECEIVE_ERROR;
    //printf("received\n");
}

uint32_t write_multiple_coils (uint16_t starting_addr,
                              uint16_t coils_count,
                              uint8_t *coils_status,
						  	  uint16_t coils_status_size)
{
    starting_addr--;
    uint8_t* buf = malloc(14);
    buf[0] = '\x00';
    buf[1] = '\x01';
    buf[2] = '\x00';
    buf[3] = '\x00';
    buf[4] = '\x00';
    buf[5] = '\x06';
    buf[6] = slave_address;
    buf[7] = '\x0F';
    buf[8] = (starting_addr>>8)& '\xFF';
    buf[9] = (starting_addr)& '\xFF';
    buf[10] = (coils_count>>8)& '\xFF';
    buf[11] = (coils_count)& '\xFF';
    buf[12] = '\x00';
    uint32_t send_ret_val;

    if((send_error_check(sockfd, buf, send_ret_val)==UMODBUS_SEND_ERROR))
        return UMODBUS_SEND_ERROR;
    //printf("sent\n");
    uint8_t* reply = malloc(200);
    uint32_t recv_ret_val;
    if(recv_error_check(sockfd,reply,&recv_ret_val)==UMODBUS_RECEIVE_ERROR)
        return UMODBUS_RECEIVE_ERROR;
    //printf("received\n");
}
