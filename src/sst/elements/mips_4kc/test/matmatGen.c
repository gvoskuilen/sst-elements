#include <string.h>
#include <stdlib.h>
#include <stdio.h>

#define SIDE 12

uint32_t first[SIDE][SIDE];
uint32_t second[SIDE][SIDE];
uint32_t golden[SIDE][SIDE];

void printM(uint32_t mat[SIDE][SIDE], char *name) {
    int i,j;
    printf("uint32_t %s[SIDE][SIDE] =  {\n", name);

    for(i = 0; i < SIDE; ++i) {
        printf("    { ");
        for(j = 0; j < SIDE; ++j) {
            printf("%u", mat[i][j]);
            if (j != SIDE-1) {
                printf(", ");
            }
        }
        printf("}");
        if (i != SIDE-1) {
            printf(",\n");
        }
    }
    printf("};\n");
}

int main() {
    int i,j,k;
    unsigned long sum = 0;
    srand(123);
    
    for(i = 0; i < SIDE; ++i) {
        for(j = 0; j < SIDE; ++j) {
            first[i][j] = (rand());
            second[i][j] = (rand());
        }
    }

    
    //MM
    for ( i = 0 ; i < SIDE ; i++ ) {
        for ( j = 0 ; j < SIDE ; j++ ) {
            for (k = 0 ; k < SIDE ; k++ ) {
                sum = sum + first[i][k]*second[k][j];
            }
            
            golden[i][j] = sum;
            sum = 0;
        }
    }

    printM(first,"firstM");
    printM(second,"secondM");
    printM(golden,"goldenM");

    return 0;
}
