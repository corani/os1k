#include "common.h"

// defined in kernel.c
void putchar(char ch);

void *memcpy(void *dst, const void *src, size_t n) {
    uint8_t *d = (uint8_t *) dst;
    const uint8_t *s = (const uint8_t *) src;
    while (n--) *d++ = *s++;

    return dst;
}

void *memset(void *buf, char c, size_t n) {
    uint8_t *p = (uint8_t *) buf;
    while (n--) *p++ = c;

    return buf;
}

char *strcpy(char *dst, const char *src) {
    char *p = dst;
    while ((*p++ = *src++));
    *p = '\0';

    return dst;
}

int strcmp(const char *s1, const char *s2) {
    while (*s1 && *s1 == *s2) s1++, s2++;
    // `unsigned char *` to conform to POSIX
    return *(unsigned char *)s1 - *(unsigned char *)s2;
}

void printf(const char *fmt, ...) {
    va_list vargs;
    va_start(vargs, fmt);

    while (*fmt) {
        if (*fmt != '%') {
            putchar(*fmt);
            fmt++;
            continue;
        }

        // skip '%'
        fmt++;
        switch(*fmt) {
            case '\0':
                // trailing '%'
                putchar('%');
                goto end;
            case '%':
                // escaped '%'
                putchar('%');
                break;
            case 's': {
                // null-terminated string
                const char *s = va_arg(vargs, const char*);
                while (*s) putchar(*s++);
            } break;
            case 'd': {
                // decimal integer
                int value = va_arg(vargs, int);
                if (value < 0) {
                    putchar('-');
                    value = -value;
                }

                // find maximum divisor
                int divisor = 1;
                while (value / divisor >= 10) divisor *= 10;

                // print digits
                while (divisor > 0) {
                    putchar('0' + value / divisor);
                    value %= divisor;
                    divisor /= 10;
                }
            } break;
            case 'x': {
                // hexadecimal integer
                int value = va_arg(vargs, int);
                for (int i = 7; i >= 0; i--) {
                    int nibble = (value >> (i * 4)) & 0xf;
                    putchar("0123456789abcdef"[nibble]);
                }
            } break;
        }

        fmt++;
    }

end:
    va_end(vargs);
}
