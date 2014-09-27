#ifndef DEFINE_UTILS_HEADER
#define DEFINE_UTILS_HEADER

#include <stdbool.h>

static inline bool streq(const char *s1, const char *s2)
{
    while (*s1 && *s2 && *s1 == *s2){
        s1++;
        s2++;
    }
    return *s1 == *s2;
}

#define min(a, b) ((a) < (b)) ? (a) : (b)
#define max(a, b) ((a) > (b)) ? (a) : (b)

#define STRIP_JUNK "\r\n\t "
#define strip(s) rstrip(lstrip(s, STRIP_JUNK), STRIP_JUNK)

/*! left strip: forward head pointer while in junk */
static inline char *lstrip(char *str, const char *junk)
{
    while (*str != '\0' && strchr(junk, *str))
        str++;
    return str;
}

/*! right strip: replace tail with '\0' while in junk */
static inline char *rstrip(char *str, const char *junk)
{
    size_t i = strlen(str);
    while (i > 0 && strchr(junk, str[i-1])){
        i--;
        str[i] = '\0';
    }
    return str;
}

#endif
