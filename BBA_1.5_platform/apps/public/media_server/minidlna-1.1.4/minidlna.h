/* 
 *
 * file		minidlna.h
 * brief	minidlna control implementation, modify from minidlna.c	
 *
 * author	Li Weijie
 * version	1.0.0
 * date		26Jun14
 *
 * history 	\arg  1.0.0, 19Jun14, Liweijie, Create the file.
 */

#ifndef _MINIDLNA_H_
#define _MINIDLNA_H_

#include <stdlib.h>
#include <unistd.h>
#include <string.h>
#include <stdio.h>
#include <ctype.h>
#include <sys/types.h>
#include <sys/socket.h>
#include <sys/wait.h>
#include <sys/file.h>
#include <sys/time.h>
#include <sys/param.h>
#include <sys/stat.h>
#include <netinet/in.h>
#include <arpa/inet.h>
#include <fcntl.h>
#include <time.h>
#include <signal.h>
#include <errno.h>
#include <pthread.h>
#include <limits.h>
#include <libgen.h>
#include <pwd.h>
#include <dirent.h>

#include "config.h"

#ifdef ENABLE_NLS
#include <locale.h>
#include <libintl.h>
#endif


#include "sql.h"
#include "upnphttp.h"
#include "upnpdescgen.h"
#include "minidlnapath.h"
#include "getifaddr.h"
#include "upnpsoap.h"
#include "options.h"
#include "utils.h"
#include "minissdp.h"
#include "minidlnatypes.h"
#include "process.h"
#include "upnpevents.h"
#include "scanner.h"
#include "inotify.h"
#include "log.h"
#include "tivo_beacon.h"
#include "tivo_utils.h"

#include <sys/vfs.h>/*add for get usb device space info*/

/**************************************************************************************************/
/*                                           DEFINES                                              */
/**************************************************************************************************/
#if SQLITE_VERSION_NUMBER < 3005001
# warning "Your SQLite3 library appears to be too old!  Please use 3.5.1 or newer."
# define sqlite3_threadsafe() 0
#endif


/* 
 * brief	maximum number of simultaneous connections, default 10.
 */
#define CONNECTIONS_NUM_MAX	(10)

/**************************************************************************************************/
/*                                           TYPES                                                */
/**************************************************************************************************/

/**************************************************************************************************/
/*                                           VARIABLES                                            */
/**************************************************************************************************/

/**************************************************************************************************/
/*                                           FUNCTIONS                                            */
/**************************************************************************************************/

/* 
 * fn		int open_db(sqlite3 **sq3) 
 * brief	open database with db_path,if it is not exist,create it
 * details	if db_path could't accecss, using default path to install(/var/run/minidlna/)
 *
 *
 * return	state of database
 * retval	0  found the database	
 *			1  the database is created		
 */
int open_db(sqlite3 **sq3);

/* 
 * fn		void check_db(sqlite3 *db, int new_db) 
 * brief	check database and start scanner
 * details	
 *
 *
 * return	void
 * retval	void 
 *
 * note	modify by liweijie, 2014-06-26	
 */
void check_db(sqlite3 *db, int new_db);


/* 
 * fn		int minidlna_startScannerThread()
 * brief	Start scan media files.
 * details	Before start scan media files, open DB and check if need update or rescan.
 *
 * return	
 * retval	0	usb is writtable.
 *			-1	no usb is writtable.
 * note		Written by zengdongbiao, 05May15.
 */
int minidlna_startScannerThread();



/* 
 * fn		void minidlnaStart() 
 * brief	start minidlna to work, open socket
 * details	
 *	
 *
 * return	void
 * retval	void
 *	
 */
void minidlnaStart();

/* 
 * fn		void minidlnaShutdown() 
 * brief	shutdown minidlna, close socket	
 *	
 *
 * return	void
 * retval	void
 *	
 */
void minidlnaShutdown();

/* 
 * fn		void minidlnaQuitScannerThread()
 * brief	function to quit scanner Thread 	
 *
 *
 * return		void
 * retval		void
 *
 * note	written by  22Jun14, liweijie	
 */
void minidlnaQuitScannerThread();

/* 
 * fn		void minidlnaStartInotifyThread()
 * brief	Function to start inotify thread.
 * details	This function must be called after scan thread started.
 *
 * return	
 * retval	
 *
 * note		written by zengdongbiao, 27Mar15.	
 */
void minidlnaStartInotifyThread();

/* 
 * fn		void minidlnaQuitInotifyThread()
 * brief	Function to quit inotify thread.
 * details	his function must be called before minidlnaQuitScannerThread() called.
 *
 * return	
 * retval	
 *
 * note		written by zengdongbiao, 27Mar15.
 */
void minidlnaQuitInotifyThread();


#endif /* End _MINIDLNA_H_ */

