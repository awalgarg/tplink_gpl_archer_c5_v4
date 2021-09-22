/*  Copyright(c) 2009-2011 Shenzhen TP-LINK Technologies Co.Ltd.
 *
 * file		cmsip_assert.h
 * brief		This file define the macro of assert and print, to add customer information,
 *				for DEBUG.
 * details	
 *
 * author	Yu Chuwei
 * version	1.0.0
 * date		21Sept11
 *
 * warning	
 *
 * history \arg	
 */
 
#ifndef __CMSIP_ASSERT_H__
/* 
 * brief	Macro to prevent include for many time
 */
#define __CMSIP_ASSERT_H__

/**************************************************************************************************/
/*                                           DEFINES                                              */
/**************************************************************************************************/

/* 
 * brief	This Macro enable DEBUG. If you enable DEBUG, the program will assert some parameters of a
 * 		function or some variable, and print some information to help the developer work.
 *			If you want to release this software, you must DELETE this define.
 */

/* 
 * brief	The switch to enable DEBUG or not
 */
/*
#define CMSIP_DEBUG 1
*/

#ifdef CMSIP_DEBUG
#include <assert.h>
#include <errno.h>
#include <string.h>

/* 
 * brief	The Macro to assert a expression.If the result of the expression is TRUE, this macro will
 *			do nothing. If the result is FALSE, the macro will report this case and abort.
 */
#define CMSIP_ASSERT(expr) (assert(expr))

/* 
 * brief	Print current file name,function name,line number, and the information specified in 
 *			paramters. 
 *			Note: This macro uses gcc's extend attribute.So, for other compiler, it may be wrong.
 */
#define CMSIP_PRINT(format,args...) \
	do											\
	{										 	\
			printf("--%s,%s,%d:" format "\n", __FILE__, __FUNCTION__, __LINE__, ##args);	\
	} while (0)		

/* 
 * brief	Print the string specified in paramter.
 */
#define CMSIP_PRINT_PJSTR(ptr,len)	\
	do											\
	{										\
			char tmp[128];					\
			memcpy(tmp, ptr, len);		\
			tmp[len] = 0;						\
			printf("---%s,%s,%d:%s\n", __FILE__, __FUNCTION__, __LINE__, tmp);	\
	} while (0)
#else	/*	CMSIP_DEBUG	*/
/* 
 * brief	When no DEBUG, do nothing
 */
#define CMSIP_ASSERT(expr)

/* 
 * brief	When no DEBUG, do nothing
 */
#define CMSIP_PRINT(format,args...)

/* 
 * brief	When no DEBUG, do nothing
 */
#define CMSIP_PRINT_PJSTR(ptr,len)
#endif	/*	CMSIP_DEBUG	*/
/**************************************************************************************************/
/*                                           TYPES                                                */
/**************************************************************************************************/

/**************************************************************************************************/
/*                                           VARIABLES                                            */
/**************************************************************************************************/

/**************************************************************************************************/
/*                                           FUNCTIONS                                            */
/**************************************************************************************************/

#endif	/* __CMSIP_ASSERT_H__ */
