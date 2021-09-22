/*
** Copyright (C) 2002-2009 Sourcefire, Inc.
** Copyright (C) 1998-2002 Martin Roesch <roesch@sourcefire.com>
**
** This program is free software; you can redistribute it and/or modify
** it under the terms of the GNU General Public License Version 2 as
** published by the Free Software Foundation.  You may not use, modify or
** distribute this program under any other version of the GNU General
** Public License.
**
** This program is distributed in the hope that it will be useful,
** but WITHOUT ANY WARRANTY; without even the implied warranty of
** MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
** GNU General Public License for more details.
**
** You should have received a copy of the GNU General Public License
** along with this program; if not, write to the Free Software
** Foundation, Inc., 59 Temple Place - Suite 330, Boston, MA 02111-1307, USA.
*/

/* $Id: //WIFI_SOC/MP/SDK_5_0_0_0/RT288x_SDK/source/user/snort-2.8.5.2/src/detection-plugins/sp_byte_check.h#1 $ */

#ifndef __SP_BYTE_CHECK_H__
#define __SP_BYTE_CHECK_H__

#include "sf_engine/sf_snort_plugin_api.h"
#include "decode.h"

#define BT_LESS_THAN            CHECK_LT
#define BT_EQUALS               CHECK_EQ
#define BT_GREATER_THAN         CHECK_GT
#define BT_AND                  CHECK_AND
#define BT_XOR                  CHECK_XOR
#define BT_GREATER_THAN_EQUAL   CHECK_GTE
#define BT_LESS_THAN_EQUAL      CHECK_LTE
#define BT_CHECK_ALL            CHECK_ALL
#define BT_CHECK_ATLEASTONE     CHECK_ATLEASTONE
#define BT_CHECK_NONE           CHECK_NONE

#define BIG    0
#define LITTLE 1


typedef struct _ByteTestData
{
    uint32_t bytes_to_compare; /* number of bytes to compare */
    uint32_t cmp_value;
    uint32_t operator;
    int32_t offset;
    uint8_t not_flag;
    uint8_t relative_flag;
    uint8_t data_string_convert_flag;
    uint8_t endianess;
    uint32_t base;
} ByteTestData;

void SetupByteTest(void);
uint32_t ByteTestHash(void *d);
int ByteTestCompare(void *l, void *r);
int ByteTest(void *, Packet *);
#endif  /* __SP_BYTE_CHECK_H__ */
