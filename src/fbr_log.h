#ifndef FABRIC_LOG_H
#define FABRIC_LOG_H

#include "fbr_macros.h"

#include <stdio.h>

#define FBR_LOG_TYPE_SPECIFIER(x) _Generic((x), \
    _Bool: "%d", \
    unsigned char: "%hhu", \
    char: "%c", \
    signed char: "%hd", \
    short int: "%hi", \
    unsigned short int: "%hu", \
    int: "%d", \
    unsigned int: "%u", \
    long int: "%l", \
    unsigned long int: "%lu", \
    long long int: "%lli",\
    unsigned long long int: "%llu", \
    float: "%f", \
    double: "%lf", \
    long double: "%Lf", \
    char *: "%s", \
    const char *: "%s", \
    void *: "%p", \
    int *: "%p", \
    default: "%%")

#define FBR_FILE_NO_PATH (strrchr(__FILE__, '/') ? strrchr(__FILE__, '/') + 1 : __FILE__)
// https://renenyffenegger.ch/notes/development/languages/C-C-plus-plus/preprocessor/macros/__VA_ARGS__/count-arguments
#define ELEVENTH_ARGUMENT(a1, a2, a3, a4, a5, a6, a7, a8, a9, a10, a11, ...) a11
#define COUNT_ARGUMENTS(...) ELEVENTH_ARGUMENT(dummy, ## __VA_ARGS__, 9, 8, 7, 6, 5, 4, 3, 2, 1, 0)
#define FBR_LOG_DEBUG_1(f0, f1, i0) \
    printf("(%s:%d) ", f0, f1), \
    printf("%s: ", #i0), \
    printf(FBR_LOG_TYPE_SPECIFIER(i0), i0), \
    printf("\n")
#define FBR_LOG_DEBUG_2(f0, f1, i0, i1) \
    printf("(%s:%d) ", f0, f1), \
    printf("%s: ", #i0), \
    printf(FBR_LOG_TYPE_SPECIFIER(i0), i0),\
    printf(" | %s: ", #i1), \
    printf(FBR_LOG_TYPE_SPECIFIER(i1), i1), \
    printf("\n")
#define FBR_LOG_DEBUG_3(f0, f1, i0, i1, i2) \
    printf("(%s:%d) ", f0, f1), \
    printf(" | %s: ", #i0), \
    printf(FBR_LOG_TYPE_SPECIFIER(i0), i0),\
    printf(" | %s: ", #i1), \
    printf(FBR_LOG_TYPE_SPECIFIER(i1), i1), \
    printf(" | %s: ", #i2), \
    printf(FBR_LOG_TYPE_SPECIFIER(i2), i2), \
    printf("\n")
#define FBR_LOG_DEBUG_4(f0, f1, i0, i1, i2, i3) \
    printf("(%s:%d) ", f0, f1), \
    printf("%s: ", #i0), \
    printf(FBR_LOG_TYPE_SPECIFIER(i0), i0),\
    printf(" | %s: ", #i1), \
    printf(FBR_LOG_TYPE_SPECIFIER(i1), i1), \
    printf(" | %s: ", #i2), \
    printf(FBR_LOG_TYPE_SPECIFIER(i2), i2), \
    printf(" | %s: ", #i3), \
    printf(FBR_LOG_TYPE_SPECIFIER(i3), i3), \
    printf("\n")
#define FBR_LOG_DEBUG_5(f0, f1, i0, i1, i2, i3, i4) \
    printf("(%s:%d) ", f0, f1), \
    printf("%s: ", #i0), \
    printf(FBR_LOG_TYPE_SPECIFIER(i0), i0),\
    printf(" | %s: ", #i1), \
    printf(FBR_LOG_TYPE_SPECIFIER(i1), i1), \
    printf(" | %s: ", #i2), \
    printf(FBR_LOG_TYPE_SPECIFIER(i2), i2), \
    printf(" | %s: ", #i3), \
    printf(FBR_LOG_TYPE_SPECIFIER(i3), i3), \
    printf(" | %s: ", #i4), \
    printf(FBR_LOG_TYPE_SPECIFIER(i4), i4), \
    printf("\n")
#define FBR_LOG_DEBUG(...) EXPAND_CONCAT(FBR_LOG_DEBUG_, COUNT_ARGUMENTS(__VA_ARGS__))(FBR_FILE_NO_PATH, __LINE__, ##__VA_ARGS__)
#define FBR_LOG_MESSAGE_0(f0, f1, m) \
    printf("(%s:%d) ", f0, f1), \
    printf(FBR_LOG_TYPE_SPECIFIER(m), m), \
    printf("\n")
#define FBR_LOG_MESSAGE_1(f0, f1, m, i0) \
    printf("(%s:%d) ", f0, f1), \
    printf(FBR_LOG_TYPE_SPECIFIER(m), m), \
    printf(" | %s: ", #i0), \
    printf(FBR_LOG_TYPE_SPECIFIER(i0), i0), \
    printf("\n")
#define FBR_LOG_MESSAGE_2(f0, f1, m, i0, i1) \
    printf("(%s:%d) ", f0, f1), \
    printf(FBR_LOG_TYPE_SPECIFIER(m), m), \
    printf(" | %s: ", #i0), \
    printf(FBR_LOG_TYPE_SPECIFIER(i0), i0),      \
    printf(" | %s: ", #i1), \
    printf(FBR_LOG_TYPE_SPECIFIER(i1), i1), \
    printf("\n")
#define FBR_LOG_MESSAGE(m, ...) EXPAND_CONCAT(FBR_LOG_MESSAGE_, COUNT_ARGUMENTS(__VA_ARGS__))(FBR_FILE_NO_PATH, __LINE__, m, ##__VA_ARGS__)
#define FBR_LOG_ERROR_1(f0, f1, f2, e) \
    printf("###ERRROR### %s:%s:%d: ", f0, f1, f2), \
    printf(FBR_LOG_TYPE_SPECIFIER(e), e), \
    printf("\n")
#define FBR_LOG_ERROR(e) FBR_LOG_ERROR_1(FBR_FILE_NO_PATH, __FUNCTION__, __LINE__, e)

#endif //FABRIC_LOG_H
