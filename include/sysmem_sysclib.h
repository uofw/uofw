/* Copyright (C) 2011, 2012 The uOFW team
   See the file COPYING for copying permission.
*/

#include <stdarg.h>

#include "common_header.h"

#define CTYPE_DOWNCASE_LETTER 0x01
#define CTYPE_UPCASE_LETTER   0x02
#define CTYPE_CIPHER          0x04
#define CTYPE_TRANSPARENT     0x08
#define CTYPE_PUNCTUATION     0x10
#define CTYPE_CTRL            0x20
#define CTYPE_HEX_CIPHER      0x40

#define CTYPE_LETTER (CTYPE_DOWNCASE_LETTER | CTYPE_UPCASE_LETTER)

typedef void (*prnt_callback)(void *ctx, int ch);

int memset(void *s, int c, int n);

int bcmp(void *s1, void *s2, int n);
int bcopy(void *src, void *dst, int n);
int bzero(void *s, int n);
int toupper(int c);
int tolower(int c);
int look_ctype_table(int c);
char *index(char *s, int c);
u64 __udivmoddi4(u64 arg01, u64 arg23, u64 *v);
u64 __udivdi3(u64 arg01, u64 arg23);
u64 __umoddi3(u64 arg01, u64 arg23);
const void *memchr(const void *s, int c, int n);
int memcmp(const void *s1, const void *s2, int n);
void *memcpy(void *dst, const void *src, u32 n);
void *memmove(void *dst, const void *src, int n);
int prnt(prnt_callback cb, void *ctx, const char *fmt, va_list args);
int sprintf(char *str, const char *format, ...);
int snprintf(char *str, u32 size, const char *format, ...);
void sprintf_char(int *ctx, int c);
void snprintf_char(int *ctx, int c);
char *strcat(char *dst, const char *src);
char *strchr(const char *s, char c);
int strcmp(const char *s1, const char *s2);
char *strcpy(char *dest, const char *src);
int strtol(const char *nptr, char **endptr, int base);
u32 strtoul(char *nptr, char **endptr, int base);
int strncmp(const char *s1, const char *s2, int n);
char *strncpy(char *dest, const char *src, int n);
u32 strnlen(const char *s, int maxlen);
u32 strlen(const char *s);
char *strrchr(char *s, int c);
char *strpbrk(char *s, const char *accept);
char *strstr(char *haystack, const char *needle);

