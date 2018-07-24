/**
 *  @file    uModbusTCPMasterTest.c
 *  @author  Soujanya Chandrashekar
 *  @date    24/7/2018
 *
 *  @brief Testing program for MODBUS TCP client
 *
 *  @section DESCRIPTION
 *
 * This program takes input from the user, calls the functions defined
 * in the uModbusTCPMaster library and sends and receives data from
 * the given MODBUS device.
 */

#include "uModbusTCPMaster.h"
#include "uModbusTCPMaster.c"

int main(int argc, char* argv[])
{
    uint8_t* ip_address = malloc(20);
    uint16_t port_number;
    printf("Enter device IP address: ");
    scanf("%s", ip_address);
    printf("Enter port number to use: ");
    scanf(" %d", &port_number);

    network_init(ip_address, port_number);
    printf("\nEnter the slave address: ");
    scanf("%x", &slave_address);
    int n;
    while(1)
    {
        printf("Enter function code to use the given function\n");
        printf("1 - Read coils\n2 - Read discrete inputs\n3 - Read holding registers\n4 - Read input registers\n5 - write single coil\n");
        printf("6 - write single reg\n15 - Write multiple coils\n16 - Write multiple reg\n");
        printf("22 - Mask write register\n23 - Read write multiple registers\n24 - Read FIFO queue\n43 - Encap interface transpt\n");
        scanf("%d", &n);
        switch (n) {
            case 1:
            {
                uint16_t starting_addr;
                printf("Enter starting address: ");
                scanf(" %hd", &starting_addr);
                //printf("%x\n", starting_addr);
                uint16_t coil_count;
                printf("Enter coil count: ");
                scanf(" %hd", &coil_count);
                //printf("%d\n", coil_count);
                printf("Enter coil status size: ");
                uint8_t coils_status_size;
                scanf(" %hhd", &coils_status_size);
                uint8_t *coils_status = malloc(coils_status_size);
                read_coils(starting_addr, coil_count, coils_status,coils_status_size);
                int k = 0;
                for(int i=starting_addr;i<coil_count+starting_addr;i++)
                {
                    printf("%d %d\n", i, coils_status[k]);
                    k++;
                }
                free(coils_status);
                break;
            }
            case 2:
            {
                uint16_t starting_addr;
                printf("Enter starting address: ");
                scanf(" %hd", &starting_addr);
                //printf("%x\n", starting_addr);
                uint16_t inputs_count;
                printf("Enter inputs count: ");
                scanf(" %hd", &inputs_count);
                //printf("%d\n", coil_count);
                printf("Enter inputs status size: ");
                uint8_t inputs_status_size;
                scanf(" %hhd", &inputs_status_size);
                uint8_t *inputs_status = malloc(inputs_status_size);
                read_discrete_inputs(starting_addr, inputs_count, inputs_status,inputs_status_size);
                int k = 0;
                for(int i=starting_addr;i<inputs_count+starting_addr;i++)
                {
                    printf("%d %d\n", i, inputs_status[k]);
                    k++;
                }
                break;
            }
            case 3:
            {
                uint16_t starting_addr;
                printf("Enter starting address: ");
                scanf(" %hd", &starting_addr);
                uint16_t reg_count;
                printf("Enter register count: ");
                scanf(" %hd", &reg_count);
                printf("Enter registers values array size: ");
                uint8_t  reg_values_size;
                scanf(" %hhd", &reg_values_size);
                uint8_t *reg_values = malloc(reg_values_size);
                read_holding_reg(starting_addr, reg_count, reg_values,reg_values_size);
                int k = 0;
                for(int i=starting_addr;i<reg_count+starting_addr;i++)
                {
                    printf("%d %d\n", i, reg_values[k]);
                    k++;
                }
                break;
            }
            case 4:
            {
                uint16_t starting_addr;
                printf("Enter starting address: ");
                scanf(" %hd", &starting_addr);
                uint16_t reg_count;
                printf("Enter register count: ");
                scanf(" %hd", &reg_count);
                printf("Enter registers values array size: ");
                uint8_t  reg_values_size;
                scanf(" %hhd", &reg_values_size);
                uint8_t *reg_values = malloc(reg_values_size);
                read_input_reg(starting_addr, reg_count, reg_values,reg_values_size);
                int k = 0;
                for(int i=starting_addr;i<reg_count+starting_addr;i++)
                {
                    printf("%d %d\n", i, reg_values[k]);
                    k++;
                }
                break;
            }
            case 5:
            {
                uint16_t coil_addr;
                uint16_t coil_status;
                printf("Enter address of coil to be written to: ");
                scanf(" %hd", &coil_addr);
                printf("Enter the status of the coil (0 for OFF and 1 for ON)");
                scanf(" %hd", &coil_status);
                write_single_coil(coil_addr,coil_status);
                break;
            }
            case 6:
            {
                uint16_t reg_addr;
                uint16_t reg_val;
                printf("Enter address of register to be written to: ");
                scanf(" %hd", &reg_addr);
                printf("Enter the value of the register");
                scanf(" %hd", &reg_val);
                write_single_reg(reg_addr,reg_val);
                break;
            }
            case 15:
            {
                uint16_t starting_addr;
                uint16_t coils_count;
                uint16_t coils_status_size;
                printf("Enter starting address: ");
                scanf(" %hd", &starting_addr);
                printf("Enter number of coils to be written: ");
                scanf(" %hd", &coils_count);
                printf("Enter coils status array size: ");
                scanf(" %hd", &coils_status_size);
                uint8_t *coils_status = malloc(coils_status_size);
                printf("coils values to be written: \n");
                //printf("css%d\n", coils_status_size);
                //temp_ptr = coil_status;
                for(int i=1;i<=coils_status_size;i++)
                {
                    printf("%d: ", i);
                    scanf(" %d[^\n]", (coils_status+i-1));
                    printf("\n");
                }
                write_multiple_coils(starting_addr, coils_count, coils_status, coils_status_size);
                break;
            }
            case 16:
            {
                uint16_t starting_addr;
                uint16_t reg_count;
                uint16_t reg_val_size;
                printf("Enter starting address: ");
                scanf(" %hd", &starting_addr);
                printf("Enter number of reg to be written: ");
                scanf(" %hd", &reg_count);
                printf("Enter reg val array size: ");
                scanf(" %hd", &reg_val_size);
                uint16_t *reg_val = malloc(reg_val_size);
                printf("reg values to be written: \n");
                //printf("css%d\n", coils_status_size);
                //temp_ptr = coil_status;
                for(int i=1;i<=reg_count;i++)
                {
                    printf("%d: ", i);
                    scanf(" %d[^\n]", (reg_val+i-1));
                    printf("\n");
                }
                write_multiple_reg(starting_addr, reg_count, reg_val, reg_val_size);
                break;
            }
            case 22:
            {
                break;
            }
            case 23:
            {
                uint16_t r_starting_addr;
                printf("Enter read starting address: ");
                scanf(" %hd", &r_starting_addr);
                uint16_t r_number;
                printf("Enter read register count: ");
                scanf(" %hd", &r_number);
                printf("Enter read registers values array size: ");
                uint8_t  r_reg_val_size;
                scanf(" %hhd", &r_reg_val_size);
                uint8_t *r_reg_val = malloc(r_reg_val_size);

                uint16_t w_starting_addr;
                uint16_t w_number;
                uint16_t w_reg_val_size;
                printf("Enter write starting address: ");
                scanf(" %hd", &w_starting_addr);
                printf("Enter number of reg to be written: ");
                scanf(" %hd", &w_number);
                printf("Enter write reg val array size: ");
                scanf(" %hd", &w_reg_val_size);
                uint16_t *w_reg_val = malloc(w_reg_val_size);
                printf("reg values to be written: \n");
                for(int i=1;i<=w_number;i++)
                {
                    printf("%d: ", i);
                    scanf(" %d[^\n]", (w_reg_val+i-1));
                    printf("\n");
                }
                rw_multiple_reg (r_starting_addr, r_number, w_starting_addr,
                     w_number, w_reg_val, w_reg_val_size, r_reg_val, r_reg_val_size);
                int k =0;
                 for(int i=r_starting_addr;i<r_number+r_starting_addr;i++)
                 {
                     printf("%d %d\n", i, r_reg_val[k]);
                     k++;
                 }
                break;
            }
            case 24:
            {
                uint16_t FIFO_ptr_addr;
                printf("Enter FIFO pointer address: ");
                scanf(" %hd", &FIFO_ptr_addr);
                uint16_t *FIFO_count = malloc(sizeof(uint8_t));
                printf("Enter expected size of FIFO register list:");
                scanf("%hd[^\n]", FIFO_count);
                uint32_t *FIFO_values = malloc(FIFO_count);
                uint32_t ret = read_FIFO_q (FIFO_ptr_addr, FIFO_count, FIFO_values);
                //printf("%d\n", ret);
                if(ret==UMODBUS_STATUS_SUCCESS)
                {
                    for(int i=0;i<*FIFO_count;i++)
                    {
                        printf("%d %d\n",i,*(FIFO_values+i));
                    }
                }
                else
                {
                    printf("%d\n", *FIFO_count);
                }
                break;
            }
            case 43:
            {
                uint32_t encap_interface_transport (uint8_t MEI_type,
                                                    uint16_t *MEI_type_data,
                									uint32_t *MEI_type_data_size);


                break;
            }
        }
    }
return 0;
}
