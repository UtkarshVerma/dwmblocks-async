#pragma once

#define LEN(arr)  (sizeof(arr) / sizeof(arr[0]))
#define MAX(a, b) (a > b ? a : b)

int gcd(int, int);
void closePipe(int[2]);
void trimUTF8(char*, unsigned int);
