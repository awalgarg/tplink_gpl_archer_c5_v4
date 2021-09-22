/* MiniDLNA project
 *
 * http://sourceforge.net/projects/minidlna/
 *
 * MiniDLNA media server
 * Copyright (C) 2008-2012  Justin Maggard
 *
 * This file is part of MiniDLNA.
 *
 * MiniDLNA is free software; you can redistribute it and/or modify
 * it under the terms of the GNU General Public License version 2 as
 * published by the Free Software Foundation.
 *
 * MiniDLNA is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with MiniDLNA. If not, see <http://www.gnu.org/licenses/>.
 *
 * Portions of the code from the MiniUPnP project:
 *
 * Copyright (c) 2006-2007, Thomas Bernard
 * All rights reserved.
 *
 * Redistribution and use in source and binary forms, with or without
 * modification, are permitted provided that the following conditions are met:
 *     * Redistributions of source code must retain the above copyright
 *       notice, this list of conditions and the following disclaimer.
 *     * Redistributions in binary form must reproduce the above copyright
 *       notice, this list of conditions and the following disclaimer in the
 *       documentation and/or other materials provided with the distribution.
 *     * The name of the author may not be used to endorse or promote products
 *       derived from this software without specific prior written permission.
 *
 * THIS SOFTWARE IS PROVIDED BY THE COPYRIGHT HOLDERS AND CONTRIBUTORS "AS IS"
 * AND ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE
 * IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE
 * ARE DISCLAIMED. IN NO EVENT SHALL THE COPYRIGHT OWNER OR CONTRIBUTORS BE
 * LIABLE FOR ANY DIRECT, INDIRECT, INCIDENTAL, SPECIAL, EXEMPLARY, OR
 * CONSEQUENTIAL DAMAGES (INCLUDING, BUT NOT LIMITED TO, PROCUREMENT OF
 * SUBSTITUTE GOODS OR SERVICES; LOSS OF USE, DATA, OR PROFITS; OR BUSINESS
 * INTERRUPTION) HOWEVER CAUSED AND ON ANY THEORY OF LIABILITY, WHETHER IN
 * CONTRACT, STRICT LIABILITY, OR TORT (INCLUDING NEGLIGENCE OR OTHERWISE)
 * ARISING IN ANY WAY OUT OF THE USE OF THIS SOFTWARE, EVEN IF ADVISED OF THE
 * POSSIBILITY OF SUCH DAMAGE.
 */

#include "minidlna.h"
#include "minidlna_tplink.h"
#include "upnpglobalvars.h"
#include <os_msg.h>

#ifdef USE_UPDATE
#include "updateDatabase.h"
#endif

/**************************************************************************************************/
/*                                           DEFINES                                              */
/**************************************************************************************************/

/* 
 * brief	Minimum space(KB) to store DB cache. For 10000 files, 40 M space is enough.
 */
#define MINIMUM_SPACE_FOR_DB	(40 * 1024) 

/**************************************************************************************************/
/*                                           TYPES                                                */
/**************************************************************************************************/

/**************************************************************************************************/
/*                                           EXTERN_PROTOTYPES                                    */
/**************************************************************************************************/
extern CMSG_FD g_usFd;
extern CMSG_BUFF g_usBuff;

/**************************************************************************************************/
/*                                           LOCAL_PROTOTYPES                                     */
/**************************************************************************************************/

/* 
 * fn		static int OpenAndConfHTTPSocket(unsigned short port) 
 * brief	setup the socket used to handle incoming HTTP connections
 *
 * param[in]	port	http socket to open, default 8200
 *
 * return	socket handle
 * retval	> 0 open socket success
 *			< 0	open socket fail
 */
static int OpenAndConfHTTPSocket(unsigned short port);

/* 
 * fn		static void sigterm(int sig) 
 * brief	handler for the SIGTERM signal (kill),SIGINT is also handled
 *
 * return	void
 * retval	void 
 *	
 */
static void sigterm(int sig);


/* 
 * fn		static void set_startup_time(void) 
 * brief	record the startup time
 *
 * return	void
 * retval	void 
 *		
 */
static void set_startup_time(void);

 
/* 
 * fn		static void getfriendlyname(char *buf, int len) 
 * brief	get friendly name
 * details	
 *
 * param[in]	len 	length of buf
 * param[out]	buf		point to friendly name string
 *
 * return	void
 * retval	void
 *	
 */
static void getfriendlyname(char *buf, int len);

/* 
 * fn		static int writepidfile(const char *fname, int pid, uid_t uid) 
 * brief	write pid file
 *
 * param[in]	fname	pointer to pif file name string
 * param[in]	pid		pid to write
 * param[in]    uid
 *
 * return	state of write
 * retval	0		success
 *			-1		fail
 *		
 */
static int writepidfile(const char *fname, int pid, uid_t uid);

/* 
 * fn		static int init(int argc, char **argv) 
 * brief	init base minidlna data
 * details	init phase:
 *			1) read configuration file
 * 			2) read command line arguments
 * 			3) daemonize
 * 			4) check and write pid file
 * 			5) set startup time stamp
 * 			6) compute presentation URL
 * 			7) set signal handlers 
 *	
 *
 * return	state of init
 * retval	0 	success
 *			!0  fail
 * note		
 */
static int init(int argc, char **argv);


/**************************************************************************************************/
/*                                           VARIABLES                                            */
/**************************************************************************************************/
//static int sudp = -1;/* remove from minidlna1.1.4, zengdongbiao, 24Apr15.  */
static int shttpl = -1;
static int smonitor = -1;
//static int snotify[MAX_LAN_ADDR];/* remove from minidlna1.1.4, zengdongbiao, 24Apr15.  */
LIST_HEAD(httplisthead, upnphttp) upnphttphead;
static pthread_t scanner_tid = 0;	 /*added by LY to change scanner work as thread instead of process*/
static pthread_t inotify_thread = 0;
static struct timeval lastnotifytime = {0, 0};
static int max_fd = -1;
static fd_set readset;	/* for select() */
static fd_set writeset;
static int startFlag = 1;


#ifdef TIVO_SUPPORT
static	uint8_t beacon_interval = 5;
static	int sbeacon = -1;
static	struct sockaddr_in tivo_bcast;
static 	struct timeval lastbeacontime = {0, 0};
#endif

/* 
 * brief	Rescan flag. 1: rescan; 0: update. 
 */
static int l_rescanFlag = 0;

/* 
 * brief	1: usb is writtable; 0: usb is unwrittable.
 */
static int l_usbIswrittable = 0;


/**************************************************************************************************/
/*                                           LOCAL_FUNCTIONS                                      */
/**************************************************************************************************/

/* 
 * fn		static int OpenAndConfHTTPSocket(unsigned short port) 
 * brief	setup the socket used to handle incoming HTTP connections
 *
 * param[in]	port	http socket to open, default 8200
 *
 * return	socket handle
 * retval	> 0 open socket success
 *			< 0	open socket fail
 */
static int OpenAndConfHTTPSocket(unsigned short port)
{
	int s;
	int i = 1;
	struct sockaddr_in listenname;

	/* Initialize client type cache */
	memset(&clients, 0, sizeof(clients));

	s = socket(PF_INET, SOCK_STREAM, 0);
	if (s < 0)
	{
		DPRINTF(E_ERROR, L_GENERAL, "socket(http): %s\n", strerror(errno));
		return -1;
	}

	if (setsockopt(s, SOL_SOCKET, SO_REUSEADDR, &i, sizeof(i)) < 0)
		DPRINTF(E_WARN, L_GENERAL, "setsockopt(http, SO_REUSEADDR): %s\n", strerror(errno));

	memset(&listenname, 0, sizeof(struct sockaddr_in));
	listenname.sin_family = AF_INET;
	listenname.sin_port = htons(port);
	listenname.sin_addr.s_addr = htonl(INADDR_ANY);

	if (bind(s, (struct sockaddr *)&listenname, sizeof(struct sockaddr_in)) < 0)
	{
		DPRINTF(E_ERROR, L_GENERAL, "bind(http): %s\n", strerror(errno));
		close(s);
		return -1;
	}

	if (listen(s, 6) < 0)
	{
		DPRINTF(E_ERROR, L_GENERAL, "listen(http): %s\n", strerror(errno));
		close(s);
		return -1;
	}

	return s;
}

/* 
 * fn		static void sigterm(int sig) 
 * brief	handler for the SIGTERM signal (kill),SIGINT is also handled
 *
 * return	void
 * retval	void 
 *	
 */
static void sigterm(int sig)
{
	/*int save_errno = errno;*/
	signal(sig, SIG_IGN);	/* Ignore this signal while we are quitting */

	DPRINTF(E_WARN, L_GENERAL, "received signal %d, good-bye\n", sig);

	quitting = 1;
	inotify_quitting = 1;
	/*errno = save_errno;*/
}

static void
sigusr1(int sig)
{
	signal(sig, sigusr1);
	DPRINTF(E_WARN, L_GENERAL, "received signal %d, clear cache\n", sig);

	memset(&clients, '\0', sizeof(clients));
}

static void
sighup(int sig)
{
	signal(sig, sighup);
	DPRINTF(E_WARN, L_GENERAL, "received signal %d, re-read\n", sig);

	reload_ifaces(1);
}

/* 
 * fn		static void set_startup_time(void) 
 * brief	record the startup time
 *
 * return	void
 * retval	void 
 *		
 */
static void set_startup_time(void)
{
	startup_time = time(NULL);
}



/* 
 * fn		static void getfriendlyname(char *buf, int len) 
 * brief	get friendly name
 * details	
 *
 * param[in]	len 	length of buf
 * param[out]	buf		point to friendly name string
 *
 * return	void
 * retval	void
 *	
 */
static void getfriendlyname(char *buf, int len)
{
	char *p = NULL;
	char hn[256];
	int off;

	if (gethostname(hn, sizeof(hn)) == 0)
	{
		strncpyt(buf, hn, len);
		p = strchr(buf, '.');
		if (p)
			*p = '\0';
	}
	else
		strcpy(buf, "Unknown");

	off = strlen(buf);
	off += snprintf(buf+off, len-off, ": ");
#ifdef READYNAS
	FILE *info;
	char ibuf[64], *key, *val;
	snprintf(buf+off, len-off, "ReadyNAS");
	info = fopen("/proc/sys/dev/boot/info", "r");
	if (!info)
		return;
	while ((val = fgets(ibuf, 64, info)) != NULL)
	{
		key = strsep(&val, ": \t");
		val = trim(val);
		if (strcmp(key, "model") == 0)
		{
			snprintf(buf+off, len-off, "%s", val);
			key = strchr(val, ' ');
			if (key)
			{
				strncpyt(modelnumber, key+1, MODELNUMBER_MAX_LEN);
				*key = '\0';
			}
			snprintf(modelname, MODELNAME_MAX_LEN,
				"Windows Media Connect compatible (%s)", val);
		}
		else if (strcmp(key, "serial") == 0)
		{
			strncpyt(serialnumber, val, SERIALNUMBER_MAX_LEN);
			if (serialnumber[0] == '\0')
			{
				char mac_str[13];
				if (getsyshwaddr(mac_str, sizeof(mac_str)) == 0)
					strcpy(serialnumber, mac_str);
				else
					strcpy(serialnumber, "0");
			}
			break;
		}
	}
	fclose(info);
#if PNPX
	memcpy(pnpx_hwid+4, "01F2", 4);
	if (strcmp(modelnumber, "NVX") == 0)
		memcpy(pnpx_hwid+17, "0101", 4);
	else if (strcmp(modelnumber, "Pro") == 0 ||
	         strcmp(modelnumber, "Pro 6") == 0 ||
	         strncmp(modelnumber, "Ultra 6", 7) == 0)
		memcpy(pnpx_hwid+17, "0102", 4);
	else if (strcmp(modelnumber, "Pro 2") == 0 ||
	         strncmp(modelnumber, "Ultra 2", 7) == 0)
		memcpy(pnpx_hwid+17, "0103", 4);
	else if (strcmp(modelnumber, "Pro 4") == 0 ||
	         strncmp(modelnumber, "Ultra 4", 7) == 0)
		memcpy(pnpx_hwid+17, "0104", 4);
	else if (strcmp(modelnumber+1, "100") == 0)
		memcpy(pnpx_hwid+17, "0105", 4);
	else if (strcmp(modelnumber+1, "200") == 0)
		memcpy(pnpx_hwid+17, "0106", 4);
	/* 0107 = Stora */
	else if (strcmp(modelnumber, "Duo v2") == 0)
		memcpy(pnpx_hwid+17, "0108", 4);
	else if (strcmp(modelnumber, "NV+ v2") == 0)
		memcpy(pnpx_hwid+17, "0109", 4);
#endif
#else
	char * logname;
	logname = getenv("LOGNAME");
#ifndef STATIC // Disable for static linking
	if (!logname)
	{
		struct passwd * pwent;
		pwent = getpwuid(getuid());
		if (pwent)
			logname = pwent->pw_name;
	}
#endif
	snprintf(buf+off, len-off, "%s", logname?logname:"Unknown");
#endif
}

/* 
 * fn		static int writepidfile(const char *fname, int pid, uid_t uid) 
 * brief	write pid file
 *
 * param[in]	fname	pointer to pif file name string
 * param[in]	pid		pid to write
 * param[in]    uid
 *
 * return	state of write
 * retval	0		success
 *			-1		fail
 *		
 */
static int writepidfile(const char *fname, int pid, uid_t uid)
{
	FILE *pidfile;
	struct stat st;
	char path[PATH_MAX], *dir;
	int ret = 0;

	if(!fname || *fname == '\0')
		return -1;

	/* Create parent directory if it doesn't already exist */
	strncpyt(path, fname, sizeof(path));
	dir = dirname(path);
	if (stat(dir, &st) == 0)
	{
		if (!S_ISDIR(st.st_mode))
		{
			DPRINTF(E_ERROR, L_GENERAL, "Pidfile path is not a directory: %s\n",
			        fname);
			return -1;
		}
	}
	else
	{
		if (make_dir(dir, S_IRWXU|S_IRGRP|S_IXGRP|S_IROTH|S_IXOTH) != 0)
		{
			DPRINTF(E_ERROR, L_GENERAL, "Unable to create pidfile directory: %s\n",
			        fname);
			return -1;
		}
		if (uid > 0)
		{
			if (chown(dir, uid, -1) != 0)
				DPRINTF(E_WARN, L_GENERAL, "Unable to change pidfile %s ownership: %s\n",
					dir, strerror(errno));
		}
	}
	
	pidfile = fopen(fname, "w");
	if (!pidfile)
	{
		DPRINTF(E_ERROR, L_GENERAL, "Unable to open pidfile for writing %s: %s\n",
		        fname, strerror(errno));
		return -1;
	}

	if (fprintf(pidfile, "%d\n", pid) <= 0)
	{
		DPRINTF(E_ERROR, L_GENERAL, 
			"Unable to write to pidfile %s: %s\n", fname, strerror(errno));
		ret = -1;
	}
	if (uid > 0)
	{
		if (fchown(fileno(pidfile), uid, -1) != 0)
			DPRINTF(E_WARN, L_GENERAL, "Unable to change pidfile %s ownership: %s\n",
				fname, strerror(errno));
	}

	fclose(pidfile);

	return ret;
}

static int strtobool(const char *str)
{
	return ((strcasecmp(str, "yes") == 0) ||
		(strcasecmp(str, "true") == 0) ||
		(atoi(str) == 1));
}

static void init_nls(void)
{
#ifdef ENABLE_NLS
	setlocale(LC_MESSAGES, "");
	setlocale(LC_CTYPE, "en_US.utf8");
	DPRINTF(E_DEBUG, L_GENERAL, "Using locale dir %s\n", bindtextdomain("minidlna", getenv("TEXTDOMAINDIR")));
	textdomain("minidlna");
#endif
}

/* 
 * fn		static int init(int argc, char **argv) 
 * brief	init base minidlna data
 * details	init phase:
 *			1) read configuration file
 * 			2) read command line arguments
 * 			3) daemonize
 * 			4) check and write pid file
 * 			5) set startup time stamp
 * 			6) compute presentation URL
 * 			7) set signal handlers 
 *	
 *
 * return	state of init
 * retval	0 	success
 *			!0  fail
 * note		
 */
static int init(int argc, char **argv)
{
	int i;
	int pid;
	int debug_flag = 0;
	int verbose_flag = 0;
	int options_flag = 0;
	struct sigaction sa;
	const char * presurl = NULL;
	const char * optionsfile = "/etc/minidlna.conf";
	char mac_str[13];
	char *string, *word;
	char *path;
	char buf[PATH_MAX];
	char log_str[75] = "general,artwork,database,inotify,scanner,metadata,http,ssdp,tivo=error";
	char *log_level = NULL;
	struct media_dir_s *media_dir;
	int ifaces = 0;
	media_types types;
	uid_t uid = 0;
    int count;
    int genable;

	/* first check if "-f" option is used */
	for (i=2; i<argc; i++)
	{
		if (strcmp(argv[i-1], "-f") == 0)
		{
			optionsfile = argv[i];
			options_flag = 1;
			break;
		}
	}

	/* set up uuid based on mac address */
	if (getsyshwaddr(mac_str, sizeof(mac_str)) < 0)
	{
		DPRINTF(E_OFF, L_GENERAL, "No MAC address found.  Falling back to generic UUID.\n");
		strcpy(mac_str, "554e4b4e4f57");
	}
	strcpy(uuidvalue+5, "4d696e69-444c-164e-9d41-");
	strncat(uuidvalue, mac_str, 12);

	getfriendlyname(friendly_name, FRIENDLYNAME_MAX_LEN);
	
	runtime_vars.port = 8200;
	runtime_vars.notify_interval = 895;	/* seconds between SSDP announces */
	runtime_vars.max_connections = CONNECTIONS_NUM_MAX; //50;
	runtime_vars.root_container = NULL;
	runtime_vars.ifaces[0] = NULL;

	/* read options file first since
	 * command line arguments have final say */
	if (readoptionsfile(optionsfile) < 0)
	{
		/* only error if file exists or using -f */
		if(access(optionsfile, F_OK) == 0 || options_flag)
			DPRINTF(E_FATAL, L_GENERAL, "Error reading configuration file %s\n", optionsfile);
	}

	for (i=0; i<num_options; i++)
	{
		switch (ary_options[i].id)
		{
		case UPNPIFNAME:
			for (string = ary_options[i].value; (word = strtok(string, ",")); string = NULL)
			{
				if (ifaces >= MAX_LAN_ADDR)
				{
					DPRINTF(E_ERROR, L_GENERAL, "Too many interfaces (max: %d), ignoring %s\n",
						MAX_LAN_ADDR, word);
					break;
				}
				runtime_vars.ifaces[ifaces++] = word;
			}
			break;
		case UPNPPORT:
			runtime_vars.port = atoi(ary_options[i].value);
			break;
		case UPNPPRESENTATIONURL:
			presurl = ary_options[i].value;
			break;
		case UPNPNOTIFY_INTERVAL:
			runtime_vars.notify_interval = atoi(ary_options[i].value);
			break;
		case UPNPSERIAL:
			strncpyt(serialnumber, ary_options[i].value, SERIALNUMBER_MAX_LEN);
			break;				
		case UPNPMODEL_NAME:
			strncpyt(modelname, ary_options[i].value, MODELNAME_MAX_LEN);
			break;
		case UPNPMODEL_NUMBER:
			strncpyt(modelnumber, ary_options[i].value, MODELNUMBER_MAX_LEN);
			break;
		case UPNPFRIENDLYNAME:
			strncpyt(friendly_name, ary_options[i].value, FRIENDLYNAME_MAX_LEN);
			/* If friendly_name contains special chars(such as '&'), we would replace them, add by zengdongbiao, 07Apr15.  */
			minidlnaModifyFriendlyName(friendly_name, FRIENDLYNAME_MAX_LEN);
			/* End add by zengdongbiao */
			break;
		case UPNPMEDIADIR:
			types = ALL_MEDIA;
            genable = 0;
			path = ary_options[i].value;
			word = strchr(path, ',');
			if (word && (access(path, F_OK) != 0))
			{
				/* types = 0; */
                count = 0;
				while (*path)
				{
					if (*path == ',')
					{
                        count++;
                        if (count >= 2)
                        {
						path++;
						break;
					}
					}
					else if (*path == 'A' || *path == 'a')
						types |= TYPE_AUDIO;
					else if (*path == 'V' || *path == 'v')
						types |= TYPE_VIDEO;
					else if (*path == 'P' || *path == 'p')
						types |= TYPE_IMAGES;
                    else if (*path == 'G' || *path == 'g')
                        genable = 1;
                    else if (*path == '/')
                        break;
					else
						DPRINTF(E_FATAL, L_GENERAL, "Media directory entry not understood [%s]\n",
							ary_options[i].value);
					path++;
				}
			}
			path = realpath(path, buf);
			if (!path || access(path, F_OK) != 0)
			{
				DPRINTF(E_ERROR, L_GENERAL, "Media directory \"%s\" not accessible [%s]\n",
					ary_options[i].value, strerror(errno));
				break;
			}
			media_dir = calloc(1, sizeof(struct media_dir_s));
			media_dir->path = strdup(path);
			media_dir->types = types;
            media_dir->genable = genable;
			if (media_dirs)
			{
				struct media_dir_s *all_dirs = media_dirs;
				while( all_dirs->next )
					all_dirs = all_dirs->next;
				all_dirs->next = media_dir;
			}
			else
				media_dirs = media_dir;
			break;
		case UPNPALBUMART_NAMES:
			for (string = ary_options[i].value; (word = strtok(string, "/")); string = NULL)
			{
				struct album_art_name_s * this_name = calloc(1, sizeof(struct album_art_name_s));
				int len = strlen(word);
				if (word[len-1] == '*')
				{
					word[len-1] = '\0';
					this_name->wildcard = 1;
				}
				this_name->name = strdup(word);
				if (album_art_names)
				{
					struct album_art_name_s * all_names = album_art_names;
					while( all_names->next )
						all_names = all_names->next;
					all_names->next = this_name;
				}
				else
					album_art_names = this_name;
			}
			break;
		case UPNPDBDIR:
			path = realpath(ary_options[i].value, buf);
			if (!path)
				path = (ary_options[i].value);
			make_dir(path, S_ISVTX|S_IRWXU|S_IRWXG|S_IRWXO);
			if (access(path, F_OK) != 0)
				DPRINTF(E_FATAL, L_GENERAL, "Database path not accessible! [%s]\n", path);
			strncpyt(db_path, path, PATH_MAX);
			break;
		case UPNPLOGDIR:
			path = realpath(ary_options[i].value, buf);
			if (!path)
				path = (ary_options[i].value);
			make_dir(path, S_ISVTX|S_IRWXU|S_IRWXG|S_IRWXO);
			if (access(path, F_OK) != 0)
				DPRINTF(E_FATAL, L_GENERAL, "Log path not accessible! [%s]\n", path);
			strncpyt(log_path, path, PATH_MAX);
			break;
		case UPNPLOGLEVEL:
			log_level = ary_options[i].value;
			break;
		case UPNPINOTIFY:
			if (!strtobool(ary_options[i].value))
				CLEARFLAG(INOTIFY_MASK);
			break;
		case ENABLE_TIVO:
			if (strtobool(ary_options[i].value))
				SETFLAG(TIVO_MASK);
			break;
		case ENABLE_DLNA_STRICT:
			if (strtobool(ary_options[i].value))
				SETFLAG(DLNA_STRICT_MASK);
			break;
		case ROOT_CONTAINER:
			switch (ary_options[i].value[0]) {
			case '.':
				runtime_vars.root_container = NULL;
				break;
			case 'B':
			case 'b':
				runtime_vars.root_container = BROWSEDIR_ID;
				break;
			case 'M':
			case 'm':
				runtime_vars.root_container = MUSIC_ID;
				break;
			case 'V':
			case 'v':
				runtime_vars.root_container = VIDEO_ID;
				break;
			case 'P':
			case 'p':
				runtime_vars.root_container = IMAGE_ID;
				break;
			default:
				runtime_vars.root_container = ary_options[i].value;
				DPRINTF(E_WARN, L_GENERAL, "Using arbitrary root container [%s]\n",
					ary_options[i].value);
				break;
			}
			break;
		case UPNPMINISSDPDSOCKET:
			minissdpdsocketpath = ary_options[i].value;
			break;
		case UPNPUUID:
			strcpy(uuidvalue+5, ary_options[i].value);
			break;
		case USER_ACCOUNT:
			uid = strtoul(ary_options[i].value, &string, 0);
			if (*string)
			{
				/* Symbolic username given, not UID. */
				struct passwd *entry = getpwnam(ary_options[i].value);
				if (!entry)
					DPRINTF(E_FATAL, L_GENERAL, "Bad user '%s'.\n", argv[i]);
				uid = entry->pw_uid;
			}
			break;
		case FORCE_SORT_CRITERIA:
			force_sort_criteria = ary_options[i].value;
			break;
		case MAX_CONNECTIONS:
			runtime_vars.max_connections = atoi(ary_options[i].value);
			break;
		case MERGE_MEDIA_DIRS:
			if (strtobool(ary_options[i].value))
				SETFLAG(MERGE_MEDIA_DIRS_MASK);
			break;
		default:
			DPRINTF(E_ERROR, L_GENERAL, "Unknown option in file %s\n",
			        optionsfile);
		}
	}
	if (log_path[0] == '\0')
	{
		if (db_path[0] == '\0')
			strncpyt(log_path, DEFAULT_LOG_PATH, PATH_MAX);
		else
			strncpyt(log_path, db_path, PATH_MAX);
	}
	if (db_path[0] == '\0')
		strncpyt(db_path, DEFAULT_DB_PATH, PATH_MAX);

	/* command line arguments processing */
	for (i=1; i<argc; i++)
	{
		if (argv[i][0] != '-')
		{
			DPRINTF(E_FATAL, L_GENERAL, "Unknown option: %s\n", argv[i]);
		}
		else if (strcmp(argv[i], "--help") == 0)
		{
			runtime_vars.port = -1;
			break;
		}
		else 
			switch(argv[i][1])
		{
		case 't':
			if (i+1 < argc)
				runtime_vars.notify_interval = atoi(argv[++i]);
			else
				DPRINTF(E_FATAL, L_GENERAL, "Option -%c takes one argument.\n", argv[i][1]);
			break;
		case 's':
			if (i+1 < argc)
				strncpyt(serialnumber, argv[++i], SERIALNUMBER_MAX_LEN);
			else
				DPRINTF(E_FATAL, L_GENERAL, "Option -%c takes one argument.\n", argv[i][1]);
			break;
		case 'm':
			if (i+1 < argc)
				strncpyt(modelnumber, argv[++i], MODELNUMBER_MAX_LEN);
			else
				DPRINTF(E_FATAL, L_GENERAL, "Option -%c takes one argument.\n", argv[i][1]);
			break;
		case 'p':
			if (i+1 < argc)
				runtime_vars.port = atoi(argv[++i]);
			else
				DPRINTF(E_FATAL, L_GENERAL, "Option -%c takes one argument.\n", argv[i][1]);
			break;
		case 'P':
			if (i+1 < argc)
			{
				if (argv[++i][0] != '/')
					DPRINTF(E_FATAL, L_GENERAL, "Option -%c requires an absolute filename.\n", argv[i-1][1]);
				else
					pidfilename = argv[i];
			}
			else
				DPRINTF(E_FATAL, L_GENERAL, "Option -%c takes one argument.\n", argv[i][1]);
			break;
		case 'd':
			debug_flag = 1;
		case 'v':
			verbose_flag = 1;
			break;
		case 'L':
			SETFLAG(NO_PLAYLIST_MASK);
			break;
		case 'w':
			if (i+1 < argc)
				presurl = argv[++i];
			else
				DPRINTF(E_FATAL, L_GENERAL, "Option -%c takes one argument.\n", argv[i][1]);
			break;
		case 'i':
			if (i+1 < argc)
			{
				i++;
				if (ifaces >= MAX_LAN_ADDR)
				{
					DPRINTF(E_ERROR, L_GENERAL, "Too many interfaces (max: %d), ignoring %s\n",
						MAX_LAN_ADDR, argv[i]);
					break;
				}
				runtime_vars.ifaces[ifaces++] = argv[i];
			}
			else
				DPRINTF(E_FATAL, L_GENERAL, "Option -%c takes one argument.\n", argv[i][1]);
			break;
		case 'f':
			i++;	/* discarding, the config file is already read */
			break;
		case 'h':
			runtime_vars.port = -1; // triggers help display
			break;
		case 'R':
#ifndef USE_ALBUM_RESPONSE
			snprintf(buf, sizeof(buf), "rm -rf %s/files.db %s/art_cache", db_path, db_path);
			if (system(buf) != 0)
				DPRINTF(E_FATAL, L_GENERAL, "Failed to clean old file cache. EXITING\n");
#endif /* USE_ALBUM_RESPONSE */
			break;
		case 'u':
			if (i+1 != argc)
			{
				i++;
				uid = strtoul(argv[i], &string, 0);
				if (*string)
				{
					/* Symbolic username given, not UID. */
					struct passwd *entry = getpwnam(argv[i]);
					if (!entry)
						DPRINTF(E_FATAL, L_GENERAL, "Bad user '%s'.\n", argv[i]);
					uid = entry->pw_uid;
				}
			}
			else
				DPRINTF(E_FATAL, L_GENERAL, "Option -%c takes one argument.\n", argv[i][1]);
			break;
#ifdef __linux__
		case 'S':
			SETFLAG(SYSTEMD_MASK);
			break;
#endif
		case 'V':
			printf("Version " MINIDLNA_VERSION "\n");
			exit(0);
			break;
		default:
			DPRINTF(E_ERROR, L_GENERAL, "Unknown option: %s\n", argv[i]);
			runtime_vars.port = -1; // triggers help display
		}
	}

	if (runtime_vars.port <= 0)
	{
		printf("Usage:\n\t"
			"%s [-d] [-v] [-f config_file] [-p port]\n"
			"\t\t[-i network_interface] [-u uid_to_run_as]\n"
			"\t\t[-t notify_interval] [-P pid_filename]\n"
			"\t\t[-s serial] [-m model_number]\n"
#ifdef __linux__
			"\t\t[-w url] [-R] [-L] [-S] [-V] [-h]\n"
#else
			"\t\t[-w url] [-R] [-L] [-V] [-h]\n"
#endif
			"\nNotes:\n\tNotify interval is in seconds. Default is 895 seconds.\n"
			"\tDefault pid file is %s.\n"
			"\tWith -d minidlna will run in debug mode (not daemonize).\n"
			"\t-w sets the presentation url. Default is http address on port 80\n"
			"\t-v enables verbose output\n"
			"\t-h displays this text\n"
			"\t-R forces a full rescan\n"
			"\t-L do not create playlists\n"
#ifdef __linux__
			"\t-S changes behaviour for systemd\n"
#endif
			"\t-V print the version number\n",
		        argv[0], pidfilename);
		return 1;
	}

	if (verbose_flag)
	{
		strcpy(log_str+65, "debug");
		log_level = log_str;
	}
	else if (!log_level)
		log_level = log_str;

	/* Set the default log file path to NULL (stdout) */
	path = NULL;
	if (debug_flag)
	{
		pid = getpid();
		strcpy(log_str+65, "maxdebug");
		log_level = log_str;
	}
	else if (GETFLAG(SYSTEMD_MASK))
	{
		pid = getpid();
	}
	else
	{
		pid = process_daemonize();
		#ifdef READYNAS
		unlink("/ramfs/.upnp-av_scan");
		path = "/var/log/upnp-av.log";
		#else
		if (access(db_path, F_OK) != 0)
			make_dir(db_path, S_ISVTX|S_IRWXU|S_IRWXG|S_IRWXO);
		snprintf(buf, sizeof(buf), "%s/minidlna.log", log_path);
		path = buf;
		#endif
	}

	/* set maxdebug */
	//strcpy(log_str+65, "maxdebug");
	log_level = log_str;
	
	/* set log output to stdout */
	log_init(NULL, log_level);

	int failnums = 0;

	for (failnums; failnums < 10; failnums++)
	{
		if (process_check_if_running(pidfilename) >= 0)
		{
			break;
		}

		sleep(1);
	}

	if (failnums >= 10)
	{
		DPRINTF(E_ERROR, L_GENERAL, SERVER_NAME " is already running. EXITING.\n");
		return 1;
	}	

	set_startup_time();

	/* presentation url */
	if (presurl)
		strncpyt(presentationurl, presurl, PRESENTATIONURL_MAX_LEN);
	else
		strcpy(presentationurl, "/");

	/* set signal handlers */
	memset(&sa, 0, sizeof(struct sigaction));
	sa.sa_handler = sigterm;
	if (sigaction(SIGTERM, &sa, NULL))
		DPRINTF(E_FATAL, L_GENERAL, "Failed to set %s handler. EXITING.\n", "SIGTERM");
	if (sigaction(SIGINT, &sa, NULL))
		DPRINTF(E_FATAL, L_GENERAL, "Failed to set %s handler. EXITING.\n", "SIGINT");
	if (signal(SIGPIPE, SIG_IGN) == SIG_ERR)
		DPRINTF(E_FATAL, L_GENERAL, "Failed to set %s handler. EXITING.\n", "SIGPIPE");
	if (signal(SIGHUP, &sighup) == SIG_ERR)
		DPRINTF(E_FATAL, L_GENERAL, "Failed to set %s handler. EXITING.\n", "SIGHUP");
	signal(SIGUSR1, &sigusr1);
	sa.sa_handler = process_handle_child_termination;
	if (sigaction(SIGCHLD, &sa, NULL))
		DPRINTF(E_FATAL, L_GENERAL, "Failed to set %s handler. EXITING.\n", "SIGCHLD");

	if (writepidfile(pidfilename, pid, uid) != 0)
		pidfilename = NULL;

	if (uid > 0)
	{
		struct stat st;
		if (stat(db_path, &st) == 0 && st.st_uid != uid && chown(db_path, uid, -1) != 0)
			DPRINTF(E_ERROR, L_GENERAL, "Unable to set db_path [%s] ownership to %d: %s\n",
				db_path, uid, strerror(errno));
	}

	if (uid > 0 && setuid(uid) == -1)
		DPRINTF(E_FATAL, L_GENERAL, "Failed to switch to uid '%d'. [%s] EXITING.\n",
		        uid, strerror(errno));

	children = calloc(runtime_vars.max_connections, sizeof(struct child));
	if (!children)
	{
		DPRINTF(E_ERROR, L_GENERAL, "Allocation failed\n");
		return 1;
	}
	
	return 0;
}


/* 
 * fn		static unsigned long long getFreeSpaceSize(char *path)
 * brief	Get free space size of volume path.
 * details	
 *
 * param[in]	path 	volume path.	
 *
 * return	The total size(KB) of free space.
 * retval	
 *
 * note		
 */
static unsigned long long getFreeSpaceSize(const char *path)
{
	struct statfs diskInfo;
	unsigned long long blockSize = 0;
	unsigned long long availSpace = 0;
	if (path == NULL)
		return -1;

	if (statfs(path, &diskInfo) < 0)
	{
		DPRINTF(E_WARN, L_GENERAL, "statfs %s error: %s\n", path, strerror(errno));
		return -1;
	}
	blockSize = diskInfo.f_bsize;
	availSpace = blockSize * diskInfo.f_bfree;
	//DPRINTF(E_WARN, L_GENERAL, "Free space of path[%s]: %llu kB\n", path, (availSpace>>10));
	
	return (availSpace>>10);/* KB  */
}

/* 
 * fn		static void removeOldCache()
 * brief	Remove old database and art_cache of minidlna.
 * details	Find a usable db name from g_databaseNames.
 *
 * return	void	
 *
 * note		Written by zengdongbiao, 22Apr15.
 */
static void removeOldCache(const char *dbPath)
{
	char cmd[PATH_MAX*2];
	int tryTime = 0;
		
	for (tryTime = 0; g_databaseNames[tryTime] != NULL; tryTime++)
	{
		g_dbNameStr = (char *)g_databaseNames[tryTime];
		snprintf(cmd, sizeof(cmd), "rm -rf %s/%s", dbPath, g_dbNameStr);

		if (system(cmd) == 0)
		{
			DPRINTF(E_WARN, L_GENERAL, "clean old database[%s/%s] successfully!\n", 
																dbPath, g_dbNameStr);
			break;
		}
	}

	/* Clean old databases failed.  */
	if (g_databaseNames[tryTime] == NULL)
	{
		DPRINTF(E_FATAL, L_GENERAL, "Failed to clean old database!  times: %d\n", tryTime);
	}	
	
#ifndef USE_ALBUM_RESPONSE
	/* Clean the old album art pictures created by minidlna.  */
	snprintf(cmd, sizeof(cmd), "rm -rf %s/art_cache", dbPath);
	system(cmd);
#endif /* USE_ALBUM_RESPONSE */
	
}


/* 
 * fn		static int findDBPath(int *pfoundDB)
 * brief	Find path to store sqlite database.
 * details	
 *
 * param[out]	*pfoundDB	return 1 when old files.db is found.
 *
 * return	
 * retval	0	db path is found.
 *			-1	db path not found.
 * note		Written by zengdongbiao, 11May15.
 */
static int findDBPath(int *pfoundDB)
{
	char path[PATH_MAX]={0};
	char volumePath[PATH_MAX]={0};
	char cmd[PATH_MAX]={0};
	DIR *pDir = NULL;
	struct dirent *pResult = NULL;
	struct dirent entryItem;
	int foundDB = 0;
	unsigned long long freeSizeCur = 0;
	unsigned long long freeSizeLast = 0;
	struct stat dbFile;
	int idx = 0;
	int volumeNum= 0;/* number of volumes  */
	unsigned long dbSize = 0;
		
	/*use readdir_r instead of scandir, because you cannot predict the number of files in one directory, 
 	*and use scandir may casue memory problems */
	pDir = opendir(USB_DISK_DIRECTORY);
	if (NULL == pDir)
	{
		DPRINTF(E_ERROR, L_SCANNER, _("Scanning dir NULL\n"));
		return -1;
	}

	while (!readdir_r(pDir, &entryItem, &pResult) && (NULL != pResult))
	{
		/* escape dir . and usbvm and files */
		if (entryItem.d_type != DT_DIR)
		{
			continue;
		}
			
		if (entryItem.d_name[0] == '.')
		{
			continue;
		}

		if (strcmp(entryItem.d_name, USBVM) == 0)
		{
			continue;
		}

		/*Get free space size of volume. */
		memset(volumePath, 0, sizeof(volumePath));
		snprintf(volumePath, sizeof(volumePath), "%s/%s", USB_DISK_DIRECTORY, entryItem.d_name);
		freeSizeCur = getFreeSpaceSize(volumePath);
		if (freeSizeCur < 0)
		{
			DPRINTF(E_WARN, L_GENERAL, "The volume[%s] is not accessable\n", volumePath);
			continue;
		}
		volumeNum++;

		dbSize  = 0;

		/* check if db file existed and can be used*/
		for (idx = 0; g_databaseNames[idx] != NULL; idx++)
		{
			memset(path, 0, sizeof(path));
			snprintf(path, sizeof(path), "%s/%s/%s/%s", USB_DISK_DIRECTORY,
						entryItem.d_name, USB_DISK_MEDIA_SERVER_FILE_NAME, g_databaseNames[idx]);
			
			/* check if db file existed and can be used*/
			if ( stat(path, &dbFile) == 0 )
			{
				dbSize = ((unsigned long)(dbFile.st_size))>>10; /* KB */
				DPRINTF(E_WARN, L_GENERAL, "db: %s, Size: %lu\n", path, dbSize);
				
				if ( (freeSizeCur + dbSize) >= MINIMUM_SPACE_FOR_DB)/* old db_path has enough free space */
				{
					snprintf(db_path, PATH_MAX, "%s/%s/%s", USB_DISK_DIRECTORY, 
								entryItem.d_name, USB_DISK_MEDIA_SERVER_FILE_NAME);
					foundDB = 1;
				}
				else/* no enough free space.  */
				{
					snprintf(volumePath, sizeof(volumePath), "%s/%s/%s", USB_DISK_DIRECTORY, 
										entryItem.d_name, USB_DISK_MEDIA_SERVER_FILE_NAME);
#ifndef	USE_ALBUM_RESPONSE

					/* Clean the old cache. */
					snprintf(cmd, sizeof(cmd), "rm -rf %s/%s %s/art_cache", 
												volumePath, g_databaseNames[idx], volumePath);
					system(cmd);
#endif /* USE_ALBUM_RESPONSE */
				}

				break;/* no need to continue  */
			}
			
		}

		if (foundDB == 1)
		{
			break;
		}
			
		/*  if not found database, use first path which has enough free space.
		 *	if all volume has no enough free space, use the path which has the max free size.
		 */
		if ( (freeSizeLast < MINIMUM_SPACE_FOR_DB) && ((freeSizeCur + dbSize) >= freeSizeLast) )
		{
			snprintf(db_path, PATH_MAX, "%s/%s/%s", USB_DISK_DIRECTORY, entryItem.d_name, 
							USB_DISK_MEDIA_SERVER_FILE_NAME);

			freeSizeLast = freeSizeCur + dbSize;
		}

	}

	/* close dir, add by liweijie, 2014-06-27 */
	if (NULL != pDir)
	{
		if (-1 == closedir(pDir))
		{
			DPRINTF(E_ERROR, L_GENERAL, _("Close Dir fail, error: %d!\n"), errno);
		}
	}

	*pfoundDB = foundDB;

	return ((volumeNum >= 1) ? 0 : -1);
}


/* 
 * fn		static int findDBName()
 * brief	Find useable database name.
 * details	
 *
 * param[in]	
 * param[out]	
 *
 * return	
 * retval	0	Find database name successfully.
 *			-1	Find database name failed.
 * note		Written by zengdongbiao, 11May15.
 */
static int findDBName()
{
	char path[PATH_MAX]={0};
	struct stat dbFile;
	int idx = 0;
	
	for (idx = 0; g_databaseNames[idx] != NULL; idx++)
	{
		memset(path, 0, sizeof(path));
		snprintf(path, sizeof(path), "%s/%s", db_path, g_databaseNames[idx]);
		
		/* Check if has old chache. */
		if (access(path, F_OK) == 0)
		{
			/* check if db file existed and can be used*/
			if ( stat(path, &dbFile) != 0 )
			{
				DPRINTF(E_WARN, L_GENERAL, "db name cannot be used: %s\n", g_databaseNames[idx]);
				continue;
			}
		}

		break;
	}
			
	g_dbNameStr = (char *)g_databaseNames[idx];	
	DPRINTF(E_WARN, L_GENERAL, "use db name: %s\n", g_dbNameStr);

	return ((g_dbNameStr != NULL) ? 0 : (-1));
}
/**************************************************************************************************/
/*                                           PUBLIC_FUNCTIONS                                     */
/**************************************************************************************************/

/* 
 * fn		int main(int argc, char **argv)
 * brief	main function, process HTTP,SSDP requests or configure requests
 * details	
 *
 * return	
 * retval	
 *
 */
int main(int argc, char **argv)
{
	int ret, i;
	struct upnphttp *e = 0;
	struct upnphttp *next;

	struct timeval timeout, timeofday;
	time_t lastupdatetime = 0;
	int last_changecnt = 0;

	/* Init the g_dbNameStr to "files.db".  */
	g_dbNameStr = (char *)g_databaseNames[0];

	for (i = 0; i < L_MAX; i++)
		log_level[i] = E_WARN;
	
	init_nls();

	/* clear configure recv buff*/
	memset(&g_usBuff, 0 , sizeof(CMSG_BUFF));
	
	msg_init(&g_usFd);
	msg_srvInit(CMSG_ID_DLNA_MEDIA_SERVER, &g_usFd);
	
	ret = init(argc, argv);
	if (ret != 0)
		return 1;

	DPRINTF(E_WARN, L_GENERAL, "Starting " SERVER_NAME " version " MINIDLNA_VERSION ".\n");
	if (sqlite3_libversion_number() < 3005001)
	{
		DPRINTF(E_WARN, L_GENERAL, "SQLite library is old.  Please use version 3.5.1 or newer.\n");
	}

	if (TRUE != minidlnaProcessInitInfo())
	{
		return -1;
	}
	
	/* accroding the init info, decide wether start minidlna, */
	//minidlnaStart();
	MINIDLNA_DEBUG("dlna start end!\n");
	
	/* main loop */
	while (!quitting)
	{

		/* Check if we need to send SSDP NOTIFY messages and do it if needed */
		if (gettimeofday(&timeofday, 0) < 0)
		{
			DPRINTF(E_ERROR, L_GENERAL, "gettimeofday(): %s\n", strerror(errno));
			timeout.tv_sec = runtime_vars.notify_interval;
			timeout.tv_usec = 0;
		}
		else
		{
			DPRINTF(E_DEBUG, L_GENERAL, "timeofday: %d, lastnotifytime:%d, start Flag:%d\n",  
							(int)timeofday.tv_sec, (int)lastnotifytime.tv_sec, startFlag);
			/* the comparison is not very precise but who cares ? */
			if ((timeofday.tv_sec >= (lastnotifytime.tv_sec + runtime_vars.notify_interval)) || (1 == startFlag))
			{
				DPRINTF(E_DEBUG, L_SSDP, "Sending SSDP notifies\n");
				for (i = 0; i < n_lan_addr; i++)
				{
					SendSSDPNotifies(lan_addr[i].snotify, lan_addr[i].str,
						runtime_vars.port, runtime_vars.notify_interval);
				}
				memcpy(&lastnotifytime, &timeofday, sizeof(struct timeval));
				timeout.tv_sec = runtime_vars.notify_interval;
				timeout.tv_usec = 0;
				startFlag = 0;
			}
			else
			{
				timeout.tv_sec = lastnotifytime.tv_sec + runtime_vars.notify_interval
				                 - timeofday.tv_sec;
				if (timeofday.tv_usec > lastnotifytime.tv_usec)
				{
					timeout.tv_usec = 1000000 + lastnotifytime.tv_usec
					                  - timeofday.tv_usec;
					timeout.tv_sec--;
				}
				else
					timeout.tv_usec = lastnotifytime.tv_usec - timeofday.tv_usec;
			}
#ifdef TIVO_SUPPORT
			if (sbeacon >= 0)
			{
				if (timeofday.tv_sec >= (lastbeacontime.tv_sec + beacon_interval))
				{
   					sendBeaconMessage(sbeacon, &tivo_bcast, sizeof(struct sockaddr_in), 1);
					memcpy(&lastbeacontime, &timeofday, sizeof(struct timeval));
					if (timeout.tv_sec > beacon_interval)
					{
						timeout.tv_sec = beacon_interval;
						timeout.tv_usec = 0;
					}
					/* Beacons should be sent every 5 seconds or so for the first minute,
					 * then every minute or so thereafter. */
					if (beacon_interval == 5 && (timeofday.tv_sec - startup_time) > 60)
						beacon_interval = 60;
				}
				else if (timeout.tv_sec > (lastbeacontime.tv_sec + beacon_interval + 1 - timeofday.tv_sec))
					timeout.tv_sec = lastbeacontime.tv_sec + beacon_interval - timeofday.tv_sec;
			}
#endif
		}

		/* if scanning completed, reclaiming the resource of scanning pthread.  */
		if (0 == get_scanning_value())
		{
			if(scanner_tid)
			{	
				pthread_join(scanner_tid, NULL); 
				scanner_tid = 0;
				updateID++;
			}
		}

		/* select open sockets (SSDP, HTTP listen, and all HTTP soap sockets) */
		FD_ZERO(&readset);
#if 1 //DEBUG_UPDATE_TO_1.1.4
		max_fd = -1;
#endif	
		if (sssdp >= 0) 
		{
			FD_SET(sssdp, &readset);
			max_fd = MAX(max_fd, sssdp);
		}
		
		if (shttpl >= 0) 
		{
			FD_SET(shttpl, &readset);
			max_fd = MAX(max_fd, shttpl);
		}

		/* set configure request fd, add by liweijie, 2014-06-20 */
		memset(&g_usBuff, 0 , sizeof(CMSG_BUFF));

		if (g_usFd.fd >= 0)
		{
   			FD_SET(g_usFd.fd, &readset);
			max_fd = MAX(max_fd, g_usFd.fd);
		}
		
#ifdef TIVO_SUPPORT
		if (sbeacon >= 0) 
		{
			FD_SET(sbeacon, &readset);
			max_fd = MAX(max_fd, sbeacon);
		}
#endif
		
		if (smonitor >= 0) 
		{
			FD_SET(smonitor, &readset);
			max_fd = MAX(max_fd, smonitor);
		}

		i = 0;	/* active HTTP connections count */
		for (e = upnphttphead.lh_first; e != NULL; e = e->entries.le_next)
		{
			if ((e->socket >= 0) && (e->state <= 2))
			{
				FD_SET(e->socket, &readset);
				max_fd = MAX(max_fd, e->socket);
				i++;
			}
		}
		
		FD_ZERO(&writeset);

		upnpevents_selectfds(&readset, &writeset, &max_fd);

		/* Check if the SIGTERM have been occured before select */
		if(quitting) 
		{
			goto shutdown;
		}
		//DPRINTF(E_DEBUG, L_GENERAL, "Enter FD select\n");
		ret = select(max_fd+1, &readset, &writeset, 0, &timeout);
		if (ret < 0)
		{
			if(quitting) goto shutdown;
			if(errno == EINTR) continue;
			DPRINTF(E_ERROR, L_GENERAL, "select(all): %s\n", strerror(errno));
			DPRINTF(E_FATAL, L_GENERAL, "Failed to select open sockets. EXITING\n");
		}

		/* process configure request, add by liweijie, 2014-06-20 */
		if(g_usFd.fd >= 0 && FD_ISSET(g_usFd.fd, &readset))
		{
			minidlnaProcessConfigureRequest();
		}
		
		upnpevents_processfds(&readset, &writeset);
		/* process SSDP packets */
		if (sssdp >= 0 && FD_ISSET(sssdp, &readset))
		{
			ProcessSSDPRequest(sssdp, (unsigned short)runtime_vars.port);
		}
		
#ifdef TIVO_SUPPORT
		if (sbeacon >= 0 && FD_ISSET(sbeacon, &readset))
		{
			DPRINTF(E_WARN, L_GENERAL, "Received sbeacon UDP Packet\n");
			ProcessTiVoBeacon(sbeacon);
		}
#endif
		if (smonitor >= 0 && FD_ISSET(smonitor, &readset))
		{
			ProcessMonitorEvent(smonitor);
		}
			
		DPRINTF(E_DEBUG, L_GENERAL, "create notify event...\n");
		/* increment SystemUpdateID if the content database has changed,
		 * and if there is an active HTTP connection, at most once every 2 seconds */
		if (i && (timeofday.tv_sec >= (lastupdatetime + 2)))
		{
			if (scanning || sqlite3_total_changes(db) != last_changecnt)
			{
				updateID++;
				last_changecnt = sqlite3_total_changes(db);
				upnp_event_var_change_notify(EContentDirectory);
				lastupdatetime = timeofday.tv_sec;
			}
		}

		DPRINTF(E_DEBUG, L_GENERAL, "processs active http connections\n");
		/* process active HTTP connections */
		for (e = upnphttphead.lh_first; e != NULL; e = e->entries.le_next)
		{
			if ((e->socket >= 0) && (e->state <= 2) && (FD_ISSET(e->socket, &readset)))
			{
				Process_upnphttp(e);
			}
		}

		DPRINTF(E_DEBUG, L_GENERAL, "processs incoming http connections\n");
		/* process incoming HTTP connections */
		if (shttpl >= 0 && FD_ISSET(shttpl, &readset))
		{
			int shttp;
			socklen_t clientnamelen;
			struct sockaddr_in clientname;
			clientnamelen = sizeof(struct sockaddr_in);
			shttp = accept(shttpl, (struct sockaddr *)&clientname, &clientnamelen);
			if (shttp<0)
			{
				DPRINTF(E_ERROR, L_GENERAL, "accept(http): %s, shttpl: %d\n", strerror(errno), shttpl);
			}
			else
			{
				struct upnphttp * tmp = 0;
				DPRINTF(E_DEBUG, L_GENERAL, "HTTP connection from %s:%d\n",
					inet_ntoa(clientname.sin_addr),
					ntohs(clientname.sin_port) );
				/*if (fcntl(shttp, F_SETFL, O_NONBLOCK) < 0) {
					DPRINTF(E_ERROR, L_GENERAL, "fcntl F_SETFL, O_NONBLOCK\n");
				}*/
				/* Create a new upnphttp object and add it to
				 * the active upnphttp object list */
				tmp = New_upnphttp(shttp);
				if (tmp)
				{
					tmp->clientaddr = clientname.sin_addr;
					LIST_INSERT_HEAD(&upnphttphead, tmp, entries);
				}
				else
				{
					DPRINTF(E_ERROR, L_GENERAL, "New_upnphttp() failed\n");
					close(shttp);
				}
			}
		}

		DPRINTF(E_DEBUG, L_GENERAL, "delete finished http connections\n");
		/* delete finished HTTP connections */
		for (e = upnphttphead.lh_first; e != NULL; e = next)
		{
			next = e->entries.le_next;
			if(e->state >= 100)
			{
				LIST_REMOVE(e, entries);
				Delete_upnphttp(e);
			}
		}		
	}

shutdown:

	minidlnaShutdown();

	log_close();
	freeoptions();
	free_media_dirs_realative();	

	free(children);

	exit(EXIT_SUCCESS);
}


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
void minidlnaStart()
{
	MINIDLNA_DEBUG("start minidlna....\n");
	int ret = -1;
	
	LIST_INIT(&upnphttphead);

	/* Initialize children cache, Add by zengdongbiao, 19May15. */ 
	memset(children, 0, runtime_vars.max_connections * sizeof(struct child));
	
	MINIDLNA_DEBUG("open db and check db....\n");

	/* initalize the sqlite3 */
	sqlite3_initialize();

	/* Open sqlite database and rescan media files. */
	ret = minidlna_startScannerThread();
	if (ret == 0)
	{
		minidlnaStartInotifyThread();
	}

	smonitor = OpenAndConfMonitorSocket();
	MINIDLNA_DEBUG("open smonitor socket[%d] ....\n", smonitor);

	sssdp = OpenAndConfSSDPReceiveSocket();
	if (sssdp < 0)
	{
		DPRINTF(E_WARN, L_GENERAL, "Failed to open socket for receiving SSDP. Trying to use MiniSSDPd\n");
		if (SubmitServicesToMiniSSDPD(lan_addr[0].str, runtime_vars.port) < 0)
			DPRINTF(E_FATAL, L_GENERAL, "Failed to connect to MiniSSDPd. EXITING");
	}
	else
	{
		MINIDLNA_DEBUG("open ssdp receive socket[%d] ....\n", sssdp);
	}

	/* open socket for HTTP connections. Listen on the 1st LAN address */
	shttpl = OpenAndConfHTTPSocket(runtime_vars.port);
	if (shttpl < 0)
	{
		DPRINTF(E_FATAL, L_GENERAL, "Failed to open socket for HTTP. EXITING\n");
	}
	else
	{
		DPRINTF(E_WARN, L_GENERAL, "HTTP[%d] listening on port %d\n", shttpl, runtime_vars.port);
	}	

#ifdef TIVO_SUPPORT
	if (GETFLAG(TIVO_MASK))
	{
		DPRINTF(E_WARN, L_GENERAL, "TiVo support is enabled.\n");
		/* Add TiVo-specific randomize function to sqlite */
		ret = sqlite3_create_function(db, "tivorandom", 1, SQLITE_UTF8, NULL, &TiVoRandomSeedFunc, NULL, NULL);
		if (ret != SQLITE_OK)
			DPRINTF(E_ERROR, L_TIVO, "ERROR: Failed to add sqlite randomize function for TiVo!\n");
		/* open socket for sending Tivo notifications */
		sbeacon = OpenAndConfTivoBeaconSocket();
		if(sbeacon < 0)
			DPRINTF(E_FATAL, L_GENERAL, "Failed to open sockets for sending Tivo beacon notify "
		                "messages. EXITING\n");
		tivo_bcast.sin_family = AF_INET;
		tivo_bcast.sin_addr.s_addr = htonl(getBcastAddress());
		tivo_bcast.sin_port = htons(2190);
	}
#endif

	/*set fd select zero, add by liweijie ,2014-06-24 */
	FD_ZERO(&readset);
	FD_ZERO(&writeset);

	/* set startFlag to true,make it send ssdp:alive when minidlna start, add by liweijie, 2014-07-08 */
	startFlag = 1;

	reload_ifaces(0);
	lastnotifytime.tv_sec = time(NULL) + runtime_vars.notify_interval;

	/* Remove from minidlna1.1.4, zengdongbiao, 24Apr15.  */
	//SendSSDPGoodbye(snotify, n_lan_addr);
	
	MINIDLNA_DEBUG("start minidlna completed!\n");
}


/* 
 * fn		void minidlnaShutdown() 
 * brief	shutdown minidlna, close socket	
 *	
 *
 * return	void
 * retval	void
 *	
 */
void minidlnaShutdown()
{
	MINIDLNA_DEBUG("shutdown minidlna....\n");
	int i;
	struct upnphttp *e = 0;

	/* Quit inotify thread before quit scanner thread.  */
	minidlnaQuitInotifyThread();
	
	MINIDLNA_DEBUG("kill scanner....\n");
	/* kill the scanner */
	minidlnaQuitScannerThread();

	/* kill other child processes */
	process_reap_children();
	
	MINIDLNA_DEBUG("close http socket...\n");
	/* close out open sockets */
	while (upnphttphead.lh_first != NULL)
	{
		e = upnphttphead.lh_first;
		LIST_REMOVE(e, entries);
		Delete_upnphttp(e);
	}

	MINIDLNA_DEBUG("close sssdp shttpl socket...\n");
	if (sssdp >= 0)
	{
		close(sssdp);
		sssdp = -1;
	}
	if (shttpl >= 0)
	{
		close(shttpl);
		shttpl = -1;
	}
#ifdef TIVO_SUPPORT
	if (sbeacon >= 0)
	{
		close(sbeacon);
		sbeacon = -1;
	}
#endif

	/*  Fix the bug, add by zengdongbiao, 30Apr15. */
	if (smonitor >= 0)
	{
		close(smonitor);
		smonitor = -1;
	}
	/* End add by zengdongbiao.  */

	for (i = 0; i < n_lan_addr; i++)
	{
		SendSSDPGoodbyes(lan_addr[i].snotify);
		DPRINTF(E_WARN, L_GENERAL, "goint to close snotify: %d\n", lan_addr[i].snotify);
		close(lan_addr[i].snotify);
		lan_addr[i].snotify = -1;
	}

	MINIDLNA_DEBUG("close sqlite db...\n");
	//sql_exec(db, "UPDATE SETTINGS set VALUE = '%u' where KEY = 'UPDATE_ID'", updateID);
	int ret = 0;
	ret = sqlite3_close(db);
	if (SQLITE_OK != ret)
	{
		MINIDLNA_DEBUG("close sqlite failed, %d\n", ret);
	}else{
		MINIDLNA_DEBUG("close sqlite OK\n");
	}

	ret = sqlite3_shutdown();
	if (SQLITE_OK != ret)
	{
		MINIDLNA_DEBUG("shutdown sqlite failed, %d\n", ret);
	}else{
		MINIDLNA_DEBUG("shutdown sqlite OK\n");
	}
	/*set fd select zero, add by liweijie ,2014-06-24 */
	FD_ZERO(&readset);
	FD_ZERO(&writeset);	
	
	upnpevents_removeSubscribers();
	MINIDLNA_DEBUG("shutdown minidlna completed!\n");
}

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
int open_db(sqlite3 **sq3)
{
	char path[PATH_MAX] = {0};
	int new_db = 0;

	snprintf(path, sizeof(path), "%s/%s", db_path, g_dbNameStr);
	if ( access(path, F_OK) != 0)
	{
		new_db = 1;
		make_dir(db_path, S_ISVTX|S_IRWXU|S_IRWXG|S_IRWXO);
	}

	MINIDLNA_DEBUG("db_path: %s\n", db_path);
	MINIDLNA_DEBUG("path: %s\n", path);

	if (sqlite3_open(path, &db) != SQLITE_OK)
	{
		DPRINTF(E_ERROR, L_GENERAL, "ERROR: Failed to open sqlite database[%s]!\n", path);
		return -1;
	}
	
	if (sq3)
		*sq3 = db;
	sqlite3_busy_timeout(db, 5000);
	
	/* limit the heap memory used of sqlite, add by liweijie, 2014-08-01 */
	sqlite3_soft_heap_limit64(1 << 20); /* 1 * 1024 * 1024 */

	if ((sql_exec(db, "pragma page_size = 4096") != SQLITE_OK)
		|| (sql_exec(db, "pragma journal_mode = OFF") != SQLITE_OK)
		|| (sql_exec(db, "pragma synchronous = OFF;") != SQLITE_OK)
		|| (sql_exec(db, "pragma default_cache_size = 8192;") != SQLITE_OK)
		|| (sql_exec(db, "pragma temp_store = 2;") != SQLITE_OK)) /* set temp store in memory */
	{
		return -2;
	}

	return new_db;
}

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
void check_db(sqlite3 *db, int new_db)
{
	int ret = -1;

	DPRINTF(E_WARN, L_GENERAL, "new_db: %d\n", new_db);

#ifdef USE_UPDATE
	l_rescanFlag = new_db;
	MINIDLNA_DEBUG("check db %d ....\n", new_db);
	if (!new_db)
	{
		/* Check if the last update had been completed, if not, rescan.*/
		char *watchDirNumStr = NULL;
		watchDirNumStr = sql_get_text_field(db, "SELECT VALUE from SETTINGS where KEY = 'watch_dir_numbers'");
		if(watchDirNumStr)
		{
			sqlite3_free(watchDirNumStr);
		}
		else
		{
			ret = -1;
			DPRINTF(E_WARN, L_GENERAL, "Database is distorted [%s/%s]\n", db_path, g_dbNameStr);
			goto rescan;
		}
		
		/* 如果需要移除的共享条目中，文件数超过总文件数的2/3，选择重新扫描(比更新速度快)。 */
		ret = update_CheckShareFolder(1);
		if (ret == -1)
		{
			DPRINTF(E_WARN, L_GENERAL, "Rescan is faster than updating\n");
			goto rescan;
		}

	}

	ret = db_upgrade(db);
	if (ret != 0)
	{	
#endif /* USE_UPDATE */
	
rescan:
		l_rescanFlag = 1;
		
		if (ret < 0)
		{
			DPRINTF(E_WARN, L_GENERAL, "Creating new database at %s/%s\n", db_path, g_dbNameStr);
		}
		else
		{
			DPRINTF(E_WARN, L_GENERAL, "Database version mismatch; need to recreate...\n");
		}
		
		/*modify by liukun 2016.1.10*/
		/*the new build db do not need to remove and re open_db*/
		if (1 != new_db)
		{
			sqlite3_close(db);
			
			/* Remove old db and old cache */
			removeOldCache(db_path);

			open_db(&db);
		}
		/*end modify by liukun*/
		
		if (CreateDatabase() != 0)
		{
			DPRINTF(E_FATAL, L_GENERAL, "ERROR: Failed to create sqlite database!  Exiting...\n");
			return;
		}
		
#ifdef USE_UPDATE
	}
#endif /* USE_UPDATE */

	/* check if it's under scanner. */
	if (scanner_tid)
	{
		DPRINTF(E_ERROR, L_GENERAL, "Some thing is wrong, scanner pthread still exist, exist it ....\n");
		minidlnaQuitScannerThread();
	}

	/* No USB device writtable, usually when the USB was unmount, so donot rescan. */
	if (!l_usbIswrittable)
	{
		return;
	}

	set_scanning_value(1);
	set_allow_scanning_value(1);

#ifdef USE_UPDATE
	DPRINTF(E_WARN, L_GENERAL, "%s\n", 
			l_rescanFlag ? "Start scan media files ...." : "Start update database ....");
#else
	DPRINTF(E_WARN, L_GENERAL, "Start scan media files ....\n");
#endif
	
#if USE_SCANNER_THREAD
	pthread_create(&scanner_tid, NULL, start_scanner, (void *)(&l_rescanFlag));
#else
	start_scanner((void *)(&l_rescanFlag));
#endif

}

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
int minidlna_startScannerThread()
{	
	int foundDB = 0;
	int ret = 0;

	l_usbIswrittable = 1;

	/* Set g_dbNameStr to "files.db"  */
	g_dbNameStr = (char *)g_databaseNames[0];

	ret = findDBPath(&foundDB);
	if (ret == 0)
	{
		ret = findDBName();
	}
	
	if (ret < 0)/* No USB device writtable, usually when the USB was unmount */
	{
		l_usbIswrittable = 0;
		DPRINTF(E_WARN, L_GENERAL, "No USB device was writtable, "
									"going to using files.db in /var/run/minidlna\n");
		snprintf(db_path, PATH_MAX, "/var/run/minidlna");
	}

	ret = open_db(NULL); 

	/* 若数据库文件发生损坏，可能会发生sqlite3_open成功但无法sql_exec的情况，  */
	if (ret == -2)
	{
		DPRINTF(E_ERROR, L_GENERAL, "ERROR: Failed to sql_exec db file(%s/%s), goto rm it!\n", 
									db_path, g_dbNameStr);
		sqlite3_close(db);
		removeOldCache(db_path);
		ret = open_db(NULL);	
	}

	/* 若依然无法成功打开db文件，则表明磁盘可能已经被拔出。*/
	if (ret < 0)
	{
		if (-1 == ret)
		{
			sqlite3_close(db);
		}
		/* The usb have been unmounted or full. */
		DPRINTF(E_WARN, L_GENERAL, "Failed to open_db[%s], "
									"going to using files.db in /var/run/minidlna\n", db_path);
		l_usbIswrittable = 0;
		snprintf(db_path, PATH_MAX, "/var/run/minidlna");
		/* Remove old cache in /var/run/minidlna */
		removeOldCache(db_path);
		ret = open_db(NULL);	
	}
	if (ret == 0)
	{
		updateID = sql_get_int_field(db, "SELECT VALUE from SETTINGS where KEY = 'UPDATE_ID'");
		if (updateID == -1)
		{
			ret = -1;
		}
	}

	check_db(db, ret);

	return (l_usbIswrittable ? 0 : (-1));
}


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
void minidlnaQuitScannerThread()
{
	//MINIDLNA_DEBUG("Exit scanner thread.....\n");
	
	/* wait util thread exist */
	if(scanner_tid)
	{
		MINIDLNA_DEBUG("Exit scanner thread[%u].....\n", (unsigned int)scanner_tid);
		set_allow_scanning_value(0);
		
		pthread_join(scanner_tid, NULL);
		
		scanner_tid = 0;	
	}
	//MINIDLNA_DEBUG("scanner thread already exit\n");
}


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
void minidlnaStartInotifyThread()
{
#ifdef HAVE_INOTIFY
	if( GETFLAG(INOTIFY_MASK) )
	{	
		/* check if it's under inotify. */
		if (inotify_thread)
		{
			DPRINTF(E_ERROR, L_GENERAL, "Some thing is wrong, inotify pthread still exist, exist it ....\n");
			minidlnaQuitInotifyThread();
		}
	
		MINIDLNA_DEBUG("start inotify thread....\n");
		/* set inotify_quitting start, add by liweijie, 2014-06-24*/
		inotify_quitting = 0; 

		if (!sqlite3_threadsafe() || sqlite3_libversion_number() < 3005001)
		{
			DPRINTF(E_ERROR, L_GENERAL, "SQLite library is not threadsafe!  "
			                            "Inotify will be disabled.\n");
		}
		else if (pthread_create(&inotify_thread, NULL, start_inotify, NULL) != 0)
		{
			DPRINTF(E_FATAL, L_GENERAL, "ERROR: pthread_create() failed for start_inotify.\n");
		}

	}
#endif

}


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
void minidlnaQuitInotifyThread()
{
#ifdef HAVE_INOTIFY
	
	inotify_quitting = 1;
	
	if (inotify_thread)
	{
		MINIDLNA_DEBUG("close inotify thread[%u] .........\n", (unsigned int)inotify_thread);
		pthread_join(inotify_thread, NULL);
		inotify_thread = 0;
	}
#endif
}

