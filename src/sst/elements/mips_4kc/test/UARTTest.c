#define SIDE 12

typedef unsigned int uint32_t;

typedef struct {
  uint32_t tx_send;
  uint32_t rx_avail;
  uint32_t tx_buffer;
  uint32_t rx_buffer;
} UART_t;

volatile UART_t *uart = (UART_t*)0x90000000;

int main() {
  // counter variables
  volatile uint32_t x = 0;
  volatile uint32_t xx = 0;
  uint32_t read_out = 0;
  uint32_t proc_num;
  asm volatile ("li $v0, 11\n"      /* Set for 'GET_PROC_NUM_SYSCALL' syscall */
		"syscall\n"
		"move %0, $2" : "=r" (proc_num) : : "v0");
  
  // print proc_num
  asm volatile ("move $a0, %0\n"   /* Move 'proc_num' into $a0 */
		"li $v0, 1\n"      /* Set for 'PRINT_INT' syscall */
		"syscall" : : "r" (proc_num) : "v0");
  
  
  // put data in send buffer
  uart->tx_buffer = (0xdead + proc_num);
  // tell UART to send
  uart->tx_send = 1;
  
  // wait
  for (x = 0; x < 5000; ++x) {
    xx++;
  }
  
  if (uart->rx_avail) {
    read_out = uart->rx_buffer;
  }

    asm volatile ("move $a0, %0\n"   /* Move 'b' into $a0 */
		  "li $v0, 1\n"      /* Set for 'PRINT_INT' syscall */
		  "syscall" : : "r" (read_out));

  return 0;
}
