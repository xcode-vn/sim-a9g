/*
* author: lvdinh
* https://xcode.vn
* 2019/11/16
* v0.0.1
*/

#ifndef _UTILS_C_
#define _UTILS_C_
#include <ctype.h>
#include <stdio.h>

char* concatStrs(const char *s1, const char *s2)
{
    const size_t len1 = strlen(s1);
    const size_t len2 = strlen(s2);
    char *result = malloc(len1 + len2 + 1); // +1 for the null-terminator
    // in real code you would check for errors in malloc here
    memcpy(result, s1, len1);
    memcpy(result + len1, s2, len2 + 1); // +1 to copy the null-terminator
    return result;
}

#endif
