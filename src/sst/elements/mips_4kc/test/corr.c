int main() {
  int a = 1;
  int b = 1;
  int res = a | b;


  asm volatile ("move $a0, %0\n"   /* Move 'b' into $a0 */
		"li $v0, 1\n"      /* Set for 'PRINT_INT' syscall */
		"syscall" : : "r" (res));

  return 0;
}
