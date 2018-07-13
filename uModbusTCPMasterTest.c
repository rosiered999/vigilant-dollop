#include "uModbusTCPMaster.h"
#include "uModbusTCPMaster1.h"

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
        printf("6 - write single reg\n15 - Write multiple coils\n16 - Write multiple reg\n20 - Read file record\n21 - Write file record\n");
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
                printf("Enter the register value: ");
                scanf(" %hd", &reg_val);
                write_single_reg(reg_addr,reg_val);
                break;
            }
            case 15:
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
                printf("Enter coil status (0 for OFF and 1 for ON): \n");
                for(int i=starting_addr;i<coil_count+starting_addr;i++)
                {
                    printf("%d: ",i );
                    scanf(" %d", &coils_status[i]);
                //    k++;
                }
                write_multiple_coils(starting_addr, coil_count, coils_status,coils_status_size);
                break;
            }
        }
    }
}
