/**
 * @file moca_types.h
 * @author MStar Semiconductor, Inc.
 * @date 2 April 2014
 * @brief Data type declaretion used by libmoca2.
 *
 */

#ifndef _MOCA_TYPES_H_
#define _MOCA_TYPES_H_

#include "moca_Pragmas.h"

typedef int                 int32;
typedef unsigned int        uint32;
typedef short               int16;
typedef unsigned short      uint16;
//typedef char                int8;
typedef signed char                int8;
typedef unsigned char       uint8;

typedef long long           int64;
typedef void                (*fHandler)(uint32);

#if !defined(SYSTEMC)   // !defined(__cplusplus)
//typedef unsigned long long    uint64;
typedef struct uint64 {
    uint32 lo;
    uint32 hi;
} _moca_packed_attribute_ uint64;
#endif

typedef struct guid64 {
    union {
        struct {
            uint32 hi;
            uint32 lo;
        };
        uint16     hword[4];
        uint8      bytes[8];
    };
} _moca_packed_attribute_ guid64;

#if !defined(__cplusplus) || !defined(_WIN32)
typedef unsigned int        BOOL;
#endif

#define TRUE                1
#define FALSE               0

#ifndef NULL
#define NULL                0
#endif

#endif  // _MOCA_TYPES_H_

