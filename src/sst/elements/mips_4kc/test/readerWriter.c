typedef unsigned int uint32_t;

/* 

   Test of SIMD-like multi processor. For use with test_multi.py sst script.
   
   Processor 1 is the "reader." Processor 0 is the "writer."

   There is a 4 word mailbox that they share. mailbox[0] is used as a
   semaphore. All mailbox words are originally set to 0. 

   0. Each processor reads what it number is (GET_PROC_NUM_SYSCALL
   syscall) and then prints that (PRINT_INT) syscall.

   Reader:
   1. "reader" (p1) waits for mailbox[0] to be set to != 0

   2. It then reads and PRINT_INTs each value in the mailbox
   (0x1,0x1,0x2,0xdeadbeef)

   3. It sets mailbox[0] back to 0

   4. Waits for mailbox[0] != 0

   5. Reads and PRINTs each value in the mailbox (0x1,0x2,0x4,0x5)

   Writer:

   1. Sets the contents (mailbox[1..3]) to (0x1,0x2,0xdeadbeef)

   2. Sets the mailbox semaphore (mailbox[0]) to 1

   3. Waits for mailbox[0] == 0

   4. Sets the mailbox contents to (0x2,0x4,0x5)

   5. Sets the mailbox semaphore to 1.

 */

volatile uint32_t mailbox[4] = {0,0,0,0};
uint32_t scratch[32];

int main() {
    // find out who I am
    uint32_t proc_num;
    asm volatile ("li $v0, 11\n"    /* Set for 'GET_PROC_NUM_SYSCALL' syscall */
                  "syscall\n"
                  "move %0, $2" : "=r" (proc_num) : : "v0");

    // print proc_num
    asm volatile ("move $a0, %0\n"   /* Move 'proc_num' into $a0 */
                  "li $v0, 1\n"      /* Set for 'PRINT_INT' syscall */
                  "syscall" : : "r" (proc_num) : "v0");

    if (proc_num) {
        // "reader"
        while(mailbox[0] == 0) ;  // wait for data to be written
        
        // read & print contents
        for (int i = 0; i <= 3; ++i) {
            asm volatile ("move $a0, %0\n"   /* Move 'proc_num' into $a0 */
                          "li $v0, 1\n"      /* Set for 'PRINT_INT' syscall */
                          "syscall" : : "r" (mailbox[i]) : "v0");
        }            
        
        mailbox[0] = 0; // reset 

        while(mailbox[0] == 0) ;  // wait for data to be written again

        // read & print contents
        for (int i = 0; i <= 3; ++i) {
            asm volatile ("move $a0, %0\n"   /* Move 'proc_num' into $a0 */
                          "li $v0, 1\n"      /* Set for 'PRINT_INT' syscall */
                          "syscall" : : "r" (mailbox[i]) : "v0");
        }            

    } else {
        // "writer"
        mailbox[1] = 1;
        mailbox[2] = 2;
        mailbox[3] = 0xdeadbeef;
        mailbox[0] = 1;

        while(mailbox[0] == 1) ;  // wait for data to be read out

        mailbox[1] = 2;
        mailbox[2] = 4;
        mailbox[3] = 5;
        mailbox[0] = 1;
    }
    
    return 0;
}
