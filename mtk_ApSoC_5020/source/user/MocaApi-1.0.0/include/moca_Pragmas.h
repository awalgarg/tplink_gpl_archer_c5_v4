/**
 * @file moca_Pragmas.h
 * @author MStar Semiconductor, Inc.
 * @date 2 April 2014
 * @brief Pramas.
 *
 */

#ifndef _MOCA_PRAGMAS_H_
#define _MOCA_PRAGMAS_H_

#if defined(_WIN32)  || defined(SYSTEMC)
# define _moca_packed_attribute_
# define _moca_packed_aligned_(A)
# define inline __inline
#else
# define _moca_packed_attribute_        __attribute__ ((packed)) __attribute__ ((aligned (4)))
# define _moca_packed_aligned_(A)       __attribute__ ((packed)) __attribute__ ((aligned (A)))
#endif

#endif  // _MOCA_PRAGMAS_H_
