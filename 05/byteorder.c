#include <stdio.h>

void byteorder() {
    union {
        __int16_t value;
        char union_bytes[sizeof(__int16_t)];
    } test;

    test.value = 0x0102;

    if ((test.union_bytes[0] == 1) && (test.union_bytes[1] == 2)) {
        printf("big endian \n");
    } else if (test.union_bytes[0] == 2 && test.union_bytes[1] == 1) {
        printf("little endian\n");
    } else {
        printf("unknown...\n");
    }
}

int main() {
    byteorder();
    return 0;
}
