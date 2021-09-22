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

/* $Id: //WIFI_SOC/MP/SDK_5_0_0_0/RT288x_SDK/source/user/snort-2.8.5.2/src/detection-plugins/sp_clientserver.h#1 $ */
/* ClientServer detection plugin header */

#ifndef __SP_CLIENTSERVER_H__
#define __SP_CLIENTSERVER_H__

#define ONLY_STREAM 0x01                                                                                                   
#define ONLY_FRAG 0x02                                                                                                     
#define IGNORE_STREAM 0x01                                                                                                 
#define IGNORE_FRAG 0x02                                                                                                   
                                                                                                                           
typedef struct _ClientServerData                                                                                           
{                                                                                                                          
    uint8_t from_server;                                                                                                  
    uint8_t from_client;                                                                                                  
    uint8_t ignore_reassembled; /* ignore reassembled sessions */                                                         
    uint8_t only_reassembled; /* ignore reassembled sessions */                                                           
    uint8_t stateless;                                                                                                    
    uint8_t established;                                                                                                  
    uint8_t unestablished;                                                                                                
} ClientServerData;        

void SetupClientServer(void);
int OtnFlowFromServer( OptTreeNode * otn );
int OtnFlowFromClient( OptTreeNode * otn );
int OtnFlowIgnoreReassembled( OptTreeNode * otn );
int OtnFlowOnlyReassembled( OptTreeNode * otn );
uint32_t FlowHash(void *d);
int FlowCompare(void *l, void *r);

#endif  /* __SP_CLIENTSERVER_H__ */
