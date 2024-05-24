#include <libpynq.h>
#include <stdio.h>
#include <fcntl.h>
#include <unistd.h>


int main()
{
  switchbox_init();
  switchbox_set_pin(IO_AR0, SWB_UART0_RX);
  switchbox_set_pin(IO_AR1, SWB_UART0_TX);

  uart_init(UART0);
  uart_reset_fifos(UART0);

  while (1)
  {

    uint8_t payload[] = "Hello";
    uint32_t payload_size = sizeof(payload);
    uint8_t *byte_length = (uint8_t*)&payload_size;

    printf("Sending %d bytes\n",payload_size);
    for(uint32_t i = 0; i<4;  i++)
    {
      uart_send(UART0, byte_length[i]);
      printf("%d\n", byte_length[i]);
    }
    for(uint32_t i = 0; i < payload_size; i++)
    {
      uart_send(UART0, payload[i]);
      printf("%d\n", payload[i]);
    }
    sleep_msec(5000);
  }
  uart_reset_fifos(UART0);
  uart_destroy(UART0);
  pynq_destroy();
  return EXIT_SUCCESS;
}
