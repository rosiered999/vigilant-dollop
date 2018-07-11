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

//Exceptions
//Function code unknown by server
#define UMODBUS_EXCEPTION_ILLEGAL_FUNCTION 0x01
//Invalid data address
#define UMODBUS_EXCEPTION_ILLEGAL_DATA_ADDR 0x02
//Invalid data value
#define UMODBUS_EXCEPTION_ILLEGAL_DATA_VAL 0x03
//The server failed during the execution
#define UMODBUS_EXCEPTION_SLAVE_DEVICE_FAILURE 0x04
//Server received request but response will take time
#define UMODBUS_EXCEPTION_ACK 0x05
//Server did not accept request as it is engaged in processing another request
#define UMODBUS_EXCEPTION_SLAVE_BUSY 0x06
//The slave cannot perform the request
#define UMODBUS_EXCEPTION_NAK 0x07
//Slave attempted to read extended memory or record file, but parity error
#define UMODBUS_EXCEPTION_MEMORY_PARITY 0x08
//Gateway could not allocate path => gateway misconfigured or overloaded
#define UMODBUS_EXCEPTION_NO_GATEWAY_PATH 0x0A
//No response obtained from target device
#define UMODBUS_EXCEPTION_GATEWAY_TARGET_NO_RESPONSE 0x0B

//On Success
#define UMODBUS_STATUS_SUCCESS 0

#define UMODBUS_GETADDRINFO_ERROR 1001
#define UMODBUS_CONNECTION_ERROR 1002
#define UMODBUS_SEND_ERROR 1003
#define UMODBUS_RECEIVE_ERROR 1004
#define UMODBUS_PTR_NULL 1005
#define UMODBUS_SIZE_INVALID 1006
#define UMODBUS_INVALID_DATA 1007


uint8_t slave_address;

struct File_Record{
    //required for one of the function codes below
    uint16_t reference_type;
    uint16_t file_number;
    uint16_t record_num;
    uint16_t record_len;
    uint16_t file_resp_len;
    uint16_t record_data[];
};

/**
    * Function to setup the network.
    * @param ip_address is the string containing the IP of the device.
    * @param port_number is the string containing the port(usually 502) at which the device is listening.
    * @return UMODBUS_STATUS_SUCCESS when success.
              UMODBUS_GETADDRINFO_ERROR when an Internet address to connect to is not found
              UMODBUS_CONNECTION_ERROR when client was unable to connect.
*/
uint32_t network_init (uint8_t *ip_address,
                       uint16_t port_number);

/**
    * Function to send data to the server.
    * @param sockfd is the file descriptor for the socket.
    * @param buf is the string to be sent to the server.
    * @param send_ret_val is the variable in which to save the number of bytes sent.
    * @return UMODBUS_STATUS_SUCCESS when success.
              UMODBUS_SEND_ERROR when client was unable to send the data to server.
*/
uint32_t send_error_check (int32_t sockfd,
                          uint8_t* buf,
                          uint32_t* send_ret_val);

/**
    * Function to receive data from the server.
    * @param sockfd is the file descriptor for the socket.
    * @param reply is the string in which the received data from the server is to be stored.
    * @param recv_ret_val is the variable in which to save the number of bytes received.
    * @return UMODBUS_STATUS_SUCCESS when success.
              UMODBUS_RECEIVE_ERROR when client was unable to receive the data from server.
*/
uint32_t recv_error_check (int32_t sockfd,
                          uint8_t* reply,
                          uint32_t* recv_ret_val);

/**
    * Function to read coils.
    * @param starting_addr is the address onwards the coils will be read
    * @param coil_count is number of coils to be read
    * @param coils_status is the buffer to hold the status of the coils.
    * @param coils_status_size is the size of the coils status buffer. The size must be atleast coils_count/8 + 1 bytes.
    * @return UMODBUS_STATUS_SUCCESS when success.
              UMODBUS_EXCEPTION_ILLEGAL_FUNCTION when the function code is unknown by server
              UMODBUS_EXCEPTION_ILLEGAL_DATA_ADDR when the data address is invalid
              UMODBUS_EXCEPTION_ILLEGAL_DATA_VAL when the data value is invalid
              UMODBUS_EXCEPTION_SLAVE_DEVICE_FAILURE if the server fails during execution.
              UMODBUS_PTR_NULL if coils_status is a null pointer
              UMODBUS_SIZE_INVALID if coils_status_size is less than coils_count/8 + 1 bytes.
*/
uint32_t read_coils (uint16_t starting_addr,
                     uint16_t coil_count,
                     uint8_t *coils_status,
                     uint8_t  coils_status_size);

 /**
     * Function to read discrete inputs.
     * @param starting_addr is the address onwards the discrete inputs will be read
     * @param inputs_count is number of discrete inputs to be read
     * @param byte_count is the output variable to hold the number of bytes returned.
     * @param input_status is the output variable to hold the status of the inputs.
     * @return UMODBUS_STATUS_SUCCESS when success.
               UMODBUS_EXCEPTION_ILLEGAL_FUNCTION when the function code is unknown by server
               UMODBUS_EXCEPTION_ILLEGAL_DATA_ADDR when the data address is invalid
               UMODBUS_EXCEPTION_ILLEGAL_DATA_VAL when the data value is invalid
               UMODBUS_EXCEPTION_SLAVE_DEVICE_FAILURE if the server fails during execution.
               UMODBUS_PTR_NULL if input_status or byte_count is a null pointer
 */
 uint32_t read_discrete_inputs (uint8_t starting_addr,
                               uint8_t inputs_count,
                               uint8_t *byte_count,
                               uint8_t *input_status);

/**
    * Function to read holding registers.
    * @param starting_addr is the address onwards the holding registers will be read
    * @param reg_count is number of holding registers to be read
    * @param byte_count is the output variable to hold the number of bytes returned.
    * @param reg_value is the output variable to hold the status of the inputs.
    * @return UMODBUS_STATUS_SUCCESS when success.
              UMODBUS_EXCEPTION_ILLEGAL_FUNCTION when the function code is unknown by server
              UMODBUS_EXCEPTION_ILLEGAL_DATA_ADDR when the data address is invalid
              UMODBUS_EXCEPTION_ILLEGAL_DATA_VAL when the data value is invalid
              UMODBUS_EXCEPTION_SLAVE_DEVICE_FAILURE if the server fails during execution.
              UMODBUS_PTR_NULL if reg_value or byte_count is a null pointer
*/
uint32_t read_holding_reg (uint16_t starting_addr,
                          uint16_t reg_count,
                          uint8_t byte_count,
                          uint8_t *reg_value);

/**
    * Function to read input registers.
    * @param starting_addr is the address onwards the input registers will be read
    * @param ip_reg_count is number of input registers to be read
    * @param byte_count is the output variable to hold the number of bytes returned.
    * @param inp_reg_value is the output variable to hold the status of the inputs.
    * @return UMODBUS_STATUS_SUCCESS when success.
              UMODBUS_EXCEPTION_ILLEGAL_FUNCTION when the function code is unknown by server
              UMODBUS_EXCEPTION_ILLEGAL_DATA_ADDR when the data address is invalid
              UMODBUS_EXCEPTION_ILLEGAL_DATA_VAL when the data value is invalid
              UMODBUS_EXCEPTION_SLAVE_DEVICE_FAILURE if the server fails during execution.
              UMODBUS_PTR_NULL if reg_value or byte_count is a null pointer
*/
uint32_t read_ip_reg (uint16_t starting_addr,
                     uint16_t ip_reg_count,
                     uint8_t byte_count,
                     uint8_t *inp_reg_value);

/**
    * Function to write a single coil.
    * @param op_addr is the address of the coil to be forced.
    * @param op_val is the value with which the coil is to be forced. Has to be 0XFF00 (ON) or 0X0000 (OFF).
    * @return UMODBUS_STATUS_SUCCESS when success.
              UMODBUS_EXCEPTION_ILLEGAL_FUNCTION when the function code is unknown by server
              UMODBUS_EXCEPTION_ILLEGAL_DATA_ADDR when the data address is invalid
              UMODBUS_EXCEPTION_ILLEGAL_DATA_VAL when the data value is invalid
              UMODBUS_EXCEPTION_SLAVE_DEVICE_FAILURE if the server fails during execution.
              UMODBUS_PTR_NULL if reg_value or byte_count is a null pointer
              UMODBUS_INVALID_DATA if op_val is not 0XFF00 (ON) or 0X0000 (OFF)
*/
uint32_t write_single_coil (uint16_t op_addr, uint16_t op_val);

/**
    * Function to write a single register.
    * @param reg_addr is the address to be written to.
    * @param reg_val is the value to be written.
    * @return UMODBUS_STATUS_SUCCESS when success.
              UMODBUS_EXCEPTION_ILLEGAL_FUNCTION when the function code is unknown by server
              UMODBUS_EXCEPTION_ILLEGAL_DATA_ADDR when the data address is invalid
              UMODBUS_EXCEPTION_ILLEGAL_DATA_VAL when the data value is invalid
              UMODBUS_EXCEPTION_SLAVE_DEVICE_FAILURE if the server fails during execution.
              UMODBUS_PTR_NULL if reg_value or byte_count is a null pointer
*/
uint32_t write_single_reg (uint16_t reg_addr, uint16_t reg_val);

/**
    * Function to write multiple coils.
    * @param starting_addr is the address onwards the coils will be read
    * @param quantity_op is the number of outputs to be written.
    * @param byte_count is the number of bytes the outputs to be written will take.
    * @param op_val is the set of values with which the coils are to be forced. Has to be 0XFF00 (ON) or 0X0000 (OFF).
    * @return UMODBUS_STATUS_SUCCESS when success.
              UMODBUS_EXCEPTION_ILLEGAL_FUNCTION when the function code is unknown by server
              UMODBUS_EXCEPTION_ILLEGAL_DATA_ADDR when the data address is invalid
              UMODBUS_EXCEPTION_ILLEGAL_DATA_VAL when the data value is invalid
              UMODBUS_EXCEPTION_SLAVE_DEVICE_FAILURE if the server fails during execution.
              UMODBUS_PTR_NULL if reg_value or byte_count is a null pointer
              UMODBUS_INVALID_DATA if op_val is not 0XFF00 (ON) or 0X0000 (OFF)
*/
uint32_t write_multiple_coils (uint16_t starting_addr,
                              uint16_t quantity_op,
                              uint16_t byte_count,
                              uint16_t op_val[]);

/**
    * Function to write multiple registers in the remote device
    * @param starting_addr is the address to start writing from.
    * @param quantity_reg is the number of registers to be written to.
    * @param byte_count is the number of bytes that the required registers take up.
    * @param reg_val is the values to be written.
    * @return UMODBUS_STATUS_SUCCESS when success.
              UMODBUS_EXCEPTION_ILLEGAL_FUNCTION when the function code is unknown by server
              UMODBUS_EXCEPTION_ILLEGAL_DATA_ADDR when the data address is invalid
              UMODBUS_EXCEPTION_ILLEGAL_DATA_VAL when the data value is invalid
              UMODBUS_EXCEPTION_SLAVE_DEVICE_FAILURE if the server fails during execution.
              UMODBUS_EXCEPTION_MEMORY_PARITY if slave attempted to read extended memory or record file, but parity error
*/
uint32_t write_multiple_reg (uint16_t starting_addr,
                            uint16_t quantity_reg,
                            uint16_t byte_count,
                            uint16_t reg_val[]);

/**
    * Function to read file records in the remote device
    * @param byte_count is the number of bytes input.
    * @param records is the values to be read and returned.
    * @param data_len is the length of response data
    * @return UMODBUS_STATUS_SUCCESS when success.
              UMODBUS_EXCEPTION_ILLEGAL_FUNCTION when the function code is unknown by server
              UMODBUS_EXCEPTION_ILLEGAL_DATA_ADDR when the data address is invalid
              UMODBUS_EXCEPTION_ILLEGAL_DATA_VAL when the data value is invalid
              UMODBUS_EXCEPTION_SLAVE_DEVICE_FAILURE if the server fails during execution.
              UMODBUS_EXCEPTION_MEMORY_PARITY if slave attempted to read extended memory or record file, but parity error
              UMODBUS_PTR_NULL if records or data_len is a null pointer
*/
uint32_t read_file_rec (uint16_t byte_count,
                       struct File_Record *records[],
                       uint32_t *data_len);

/**
    * Function to write file records in the remote device
    * @param request_data_len is the length of data requested.
    * @param records is the list of records with their required data given.
    * @param data_len is the length of the response data.
    * @return UMODBUS_STATUS_SUCCESS when success.
              UMODBUS_EXCEPTION_ILLEGAL_FUNCTION when the function code is unknown by server
              UMODBUS_EXCEPTION_ILLEGAL_DATA_ADDR when the data address is invalid
              UMODBUS_EXCEPTION_ILLEGAL_DATA_VAL when the data value is invalid
              UMODBUS_EXCEPTION_SLAVE_DEVICE_FAILURE if the server fails during execution.
              UMODBUS_EXCEPTION_MEMORY_PARITY if slave attempted to read extended memory or record file, but parity error
              UMODBUS_PTR_NULL if records or data_len is a null pointer
*/
uint32_t write_file_rec (uint16_t request_data_len,
                        struct File_Record *records[],
                        uint32_t* data_len);

/**
    * Function to modify the contents of a given holding register.
    * @param ref_addr is the address to be written to.
    * @param and_mask is the value with which the given holding registers value will be ANDed.
    * @param or_mask is the value with which the given holding registers value will be ORed.
    * @return UMODBUS_STATUS_SUCCESS when success.
              UMODBUS_EXCEPTION_ILLEGAL_FUNCTION when the function code is unknown by server
              UMODBUS_EXCEPTION_ILLEGAL_DATA_ADDR when the data address is invalid
              UMODBUS_EXCEPTION_ILLEGAL_DATA_VAL when the data value is invalid
              UMODBUS_EXCEPTION_SLAVE_DEVICE_FAILURE if the server fails during execution.
*/
uint32_t mask_write_reg (uint16_t ref_addr,
                        uint16_t and_mask,
                        uint16_t or_mask);

/**
    * Function to read and write multiple file records in the remote device
    * @param r_starting_addr is the address to be read from.
    * @param read_quantity is the number of holding registers to be read
    * @param w_starting_addr is the address to be written to
    * @param write_quantity is the number of holding registers to be written to
    * @param w_byte_count is the number of bytes to be written
    * @param w_reg_val is the data to be written.
    * @param r_byte_count is the number of bytes read
    * @param read_reg_val is the values read.
    * @return UMODBUS_STATUS_SUCCESS when success.
              UMODBUS_EXCEPTION_ILLEGAL_FUNCTION when the function code is unknown by server
              UMODBUS_EXCEPTION_ILLEGAL_DATA_ADDR when the data address is invalid
              UMODBUS_EXCEPTION_ILLEGAL_DATA_VAL when the data value is invalid
              UMODBUS_EXCEPTION_SLAVE_DEVICE_FAILURE if the server fails during execution.
              UMODBUS_EXCEPTION_MEMORY_PARITY if slave attempted to read extended memory or record file, but parity error
              UMODBUS_PTR_NULL if byte_count or read_reg_val is a null pointer
*/
uint32_t rw_multiple_reg (uint16_t r_starting_addr,
                         uint16_t read_quantity,
                         uint16_t w_starting_addr,
                         uint16_t write_quantity,
                         uint16_t w_byte_count,
                         uint16_t w_reg_val[],
                         uint32_t* r_byte_count,
                         uint16_t *read_reg_val);

/**
    * Function read FIFO queue of the register
    * @param FIFO_ptr_addr is the address of the FIFO queue required.
    * @param byte_count is the number of bytes to follow, including the queue count bytes and the value register bytes
    * @param FIFO_count is the count of the registers in the queue, must be <= 31.
    * @param FIFO_values are the values of the registers in the queue
    * @return UMODBUS_STATUS_SUCCESS when success.
              UMODBUS_EXCEPTION_ILLEGAL_FUNCTION when the function code is unknown by server
              UMODBUS_EXCEPTION_ILLEGAL_DATA_ADDR when the count registers is not <= 31.
              UMODBUS_EXCEPTION_ILLEGAL_DATA_VAL when the data value is invalid
              UMODBUS_EXCEPTION_SLAVE_DEVICE_FAILURE if the server fails during execution.
              UMODBUS_PTR_NULL if the address of the FIFO queue is null.
*/
uint32_t read_FIFO_q (uint16_t FIFO_ptr_addr,
                     uint32_t* byte_count,
                     uint32_t* FIFO_count,
                     uint32_t* FIFO_values);

/**
    * Function mechanism for tunneling service requests and method invocations, as well as their returns, inside MODBUS PDUs
    * @param MEI_type is a fixed MODBUS assigned number used to send a method invocation to the given interface.
    * @param MEI_type_data is data specific to the MEI type
    * @return UMODBUS_STATUS_SUCCESS when success.
              UMODBUS_EXCEPTION_ILLEGAL_FUNCTION when the function code is unknown by server
              UMODBUS_EXCEPTION_ILLEGAL_DATA_ADDR when the data address is invalid
              UMODBUS_EXCEPTION_ILLEGAL_DATA_VAL when the data value is invalid
              UMODBUS_EXCEPTION_SLAVE_DEVICE_FAILURE if the server fails during execution.
*/
uint32_t encap_interface_transport (uint16_t MEI_type, uint16_t MEI_type_data);
