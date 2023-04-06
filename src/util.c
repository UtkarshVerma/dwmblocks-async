#include "util.h"

#include <unistd.h>

int gcd(int a, int b) {
    int temp;
    while (b > 0) {
        temp = a % b;
        a = b;
        b = temp;
    }
    return a;
}

void closePipe(int pipe[2]) {
    close(pipe[0]);
    close(pipe[1]);
}

void trimUTF8(char* buffer, unsigned int size) {
    int length = (size - 1) / 4;
    int count = 0, j = 0;
    char ch = buffer[j];
    while (ch != '\0' && ch != '\n' && count < length) {
        // Skip continuation bytes, if any
        int skip = 1;
        while ((ch & 0xc0) > 0x80) {
            ch <<= 1;
            skip++;
        }

        j += skip;
        ch = buffer[j];
        count++;
    }

    // Trim trailing newline and spaces
    buffer[j] = ' ';
    while (j >= 0 && buffer[j] == ' ') j--;
    buffer[j + 1] = '\0';
}
