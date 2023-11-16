#ifndef UTIL_H
#define UTIL_H

#include <stddef.h>

#define MAX(a, b) ((a) > (b) ? (a) : (b))
#define LEN(arr)  (sizeof(arr) / sizeof((arr)[0]))
#define BIT(n)    (1 << (n))

// NOLINTBEGIN(bugprone-macro-parentheses)
#define MEMBER_SIZE(type, member) sizeof(((type*)NULL)->member)
#define MEMBER_LENGTH(type, member) \
    (MEMBER_SIZE(type, member) / MEMBER_SIZE(type, member[0]))
// NOLINTEND(bugprone-macro-parentheses)

#define UTF8_MAX_BYTE_COUNT 4

enum pipe_fd_index {
    READ_END,
    WRITE_END,
    PIPE_FD_COUNT,
};

unsigned int gcd(unsigned int a, unsigned int b);
size_t truncate_utf8_string(char* const buffer, const size_t size,
                            const size_t char_limit);

#endif  // UTIL_H
