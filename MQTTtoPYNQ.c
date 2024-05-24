#include <libpynq.h>
#include <stdio.h>
#include <stdlib.h>
#include <stdint.h>
#include <string.h>
#include <unistd.h>
#include <fcntl.h>
#include <termios.h>

#define UART_STRING "/dev/ttyPS0"

// struct received_data{
//   uint32_t payload_length_recv;
//   uint8_t * payload_recv;
// }
//
//
// int receive_data(struct data){
//
// }




int main(void) {
  pynq_init();
  uart_init(UART0);
  uart_reset_fifos(UART0);
  switchbox_set_pin(IO_AR0, SWB_UART0_RX);
  switchbox_set_pin(IO_AR1, SWB_UART0_TX);

  uint8_t array[4] = {0};

  while(1){
  if(uart_has_data(UART0) == 1){
  for(uint32_t i = 0; i < 4; i++){
    array[i] = uart_recv(UART0);
  }
 
 
  uint32_t payload_size = *((uint32_t *)array);
  printf("Payload size is: %d\n", payload_size);

  uint8_t * payload = malloc(sizeof(char) * payload_size);

  for(uint32_t i = 0; i < payload_size; i++)
  {
    payload[i] = uart_recv(UART0);
    printf("%d\n", payload[i]);
  }

  free(payload);

  }
  }


  pynq_destroy();
  return EXIT_SUCCESS;
}
