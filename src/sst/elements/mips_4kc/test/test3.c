int b = 10;
int data[500];


int main() {
  int a;

  for(a = 0; a < 100; ++a) {
    data[a] = b;
  }

  for(a = 0; a < 100; ++a) {
    b += data[a];
  }

  asm volatile ("move $a0, %0\n"   /* Move 'b' into $a0 */
		"li $v0, 1\n"      /* Set for 'PRINT_INT' syscall */
		"syscall" : : "r" (b));

  return b;
}
