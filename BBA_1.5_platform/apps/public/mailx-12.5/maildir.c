/*
 * Heirloom mailx - a mail user agent derived from Berkeley Mail.
 *
 * Copyright (c) 2000-2004 Gunnar Ritter, Freiburg i. Br., Germany.
 */
/*
 * Copyright (c) 2004
 *	Gunnar Ritter.  All rights reserved.
 *
 * Redistribution and use in source and binary forms, with or without
 * modification, are permitted provided that the following conditions
 * are met:
 * 1. Redistributions of source code must retain the above copyright
 *    notice, this list of conditions and the following disclaimer.
 * 2. Redistributions in binary form must reproduce the above copyright
 *    notice, this list of conditions and the following disclaimer in the
 *    documentation and/or other materials provided with the distribution.
 * 3. All advertising materials mentioning features or use of this software
 *    must display the following acknowledgement:
 *	This product includes software developed by Gunnar Ritter
 *	and his contributors.
 * 4. Neither the name of Gunnar Ritter nor the names of his contributors
 *    may be used to endorse or promote products derived from this software
 *    without specific prior written permission.
 *
 * THIS SOFTWARE IS PROVIDED BY GUNNAR RITTER AND CONTRIBUTORS ``AS IS'' AND
 * ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE
 * IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE
 * ARE DISCLAIMED.  IN NO EVENT SHALL GUNNAR RITTER OR CONTRIBUTORS BE LIABLE
 * FOR ANY DIRECT, INDIRECT, INCIDENTAL, SPECIAL, EXEMPLARY, OR CONSEQUENTIAL
 * DAMAGES (INCLUDING, BUT NOT LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS
 * OR SERVICES; LOSS OF USE, DATA, OR PROFITS; OR BUSINESS INTERRUPTION)
 * HOWEVER CAUSED AND ON ANY THEORY OF LIABILITY, WHETHER IN CONTRACT, STRICT
 * LIABILITY, OR TORT (INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY
 * OUT OF THE USE OF THIS SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF
 * SUCH DAMAGE.
 */

#ifndef lint
#ifdef	DOSCCS
static char sccsid[] = "@(#)maildir.c	1.20 (gritter) 12/28/06";
#endif
#endif /* not lint */

#include "config.h"

#include "rcv.h"
#include "extern.h"
#include <time.h>
#include <dirent.h>
#include <unistd.h>
#include <sys/stat.h>
#include <errno.h>

/*
 * Mail -- a mail program
 *
 * Maildir folder support.
 */

static struct mditem {
	struct message	*md_data;
	unsigned	md_hash;
} *mdtable;
static long	mdprime;

static sigjmp_buf	maildirjmp;
static char *mkname(time_t t, enum mflag f, const char *pref);
static enum okay maildir_append1(const char *name, FILE *fp, off_t off1,
		long size, enum mflag flag);
static enum okay trycreate(const char *name);
static enum okay mkmaildir(const char *name);

static char *
mkname(time_t t, enum mflag f, const char *pref)
{
	static unsigned long	count;
	static pid_t	mypid;
	char	*cp;
	static char	*node;
	int	size, n, i;

	if (pref == NULL) {
		if (mypid == 0)
			mypid = getpid();
		if (node == NULL) {
			cp = nodename(0);
			n = size = 0;
			do {
				if (n < size + 8)
					node = srealloc(node, size += 20);
				switch (*cp) {
				case '/':
					node[n++] = '\\', node[n++] = '0',
					node[n++] = '5', node[n++] = '7';
					break;
				case ':':
					node[n++] = '\\', node[n++] = '0',
					node[n++] = '7', node[n++] = '2';
					break;
				default:
					node[n++] = *cp;
				}
			} while (*cp++);
		}
		size = 60 + strlen(node);
		cp = salloc(size);
		n = snprintf(cp, size, "%lu.%06lu_%06lu.%s:2,",
				(unsigned long)t,
				(unsigned long)mypid, ++count, node);
	} else {
		size = (n = strlen(pref)) + 13;
		cp = salloc(size);
		strcpy(cp, pref);
		for (i = n; i > 3; i--)
			if (cp[i-1] == ',' && cp[i-2] == '2' &&
					cp[i-3] == ':') {
				n = i;
				break;
			}
		if (i <= 3) {
			strcpy(&cp[n], ":2,");
			n += 3;
		}
	}
	if (n < size - 7) {
		if (f & MDRAFTED)
			cp[n++] = 'D';
		if (f & MFLAGGED)
			cp[n++] = 'F';
		if (f & MANSWERED)
			cp[n++] = 'R';
		if (f & MREAD)
			cp[n++] = 'S';
		if (f & MDELETED)
			cp[n++] = 'T';
		cp[n] = '\0';
	}
	return cp;
}

enum okay
maildir_append(const char *name, FILE *fp)
{
	char	*buf, *bp, *lp;
	size_t	bufsize, buflen, count;
	off_t	off1 = -1, offs;
	int	inhead = 1;
	int	flag = MNEW|MNEWEST;
	long	size = 0;
	enum okay	ok;

	if (mkmaildir(name) != OKAY)
		return STOP;
	buf = smalloc(bufsize = LINESIZE);
	buflen = 0;
	count = fsize(fp);
	offs = ftell(fp);
	do {
		bp = fgetline(&buf, &bufsize, &count, &buflen, fp, 1);
		if (bp == NULL || strncmp(buf, "From ", 5) == 0) {
			if (off1 != (off_t)-1) {
				ok = maildir_append1(name, fp, off1,
						size, flag);
				if (ok == STOP)
					return STOP;
				fseek(fp, offs+buflen, SEEK_SET);
			}
			off1 = offs + buflen;
			size = 0;
			inhead = 1;
			flag = MNEW;
		} else
			size += buflen;
		offs += buflen;
		if (bp && buf[0] == '\n')
			inhead = 0;
		else if (bp && inhead && ascncasecmp(buf, "status", 6) == 0) {
			lp = &buf[6];
			while (whitechar(*lp&0377))
				lp++;
			if (*lp == ':')
				while (*++lp != '\0')
					switch (*lp) {
					case 'R':
						flag |= MREAD;
						break;
					case 'O':
						flag &= ~MNEW;
						break;
					}
		} else if (bp && inhead &&
				ascncasecmp(buf, "x-status", 8) == 0) {
			lp = &buf[8];
			while (whitechar(*lp&0377))
				lp++;
			if (*lp == ':')
				while (*++lp != '\0')
					switch (*lp) {
					case 'F':
						flag |= MFLAGGED;
						break;
					case 'A':
						flag |= MANSWERED;
						break;
					case 'T':
						flag |= MDRAFTED;
						break;
					}
		}
	} while (bp != NULL);
	free(buf);
	return OKAY;
}

static enum okay
maildir_append1(const char *name, FILE *fp, off_t off1, long size,
		enum mflag flag)
{
	const int	attempts = 43200;
	struct stat	st;
	char	buf[4096];
	char	*fn, *tmp, *new;
	FILE	*op;
	long	n, z;
	int	i;
	time_t	now;

	for (i = 0; i < attempts; i++) {
		time(&now);
		fn = mkname(now, flag, NULL);
		tmp = salloc(n = strlen(name) + strlen(fn) + 6);
		snprintf(tmp, n, "%s/tmp/%s", name, fn);
		if (stat(tmp, &st) < 0 && errno == ENOENT)
			break;
		sleep(2);
	}
	if (i >= attempts) {
		DEBUG_ERROR(
			"Cannot create unique file name in \"%s/tmp\".\n",
			name);
		return STOP;
	}
	if ((op = Fopen(tmp, "w")) == NULL) {
		DEBUG_ERROR("Cannot write to \"%s\".\n", tmp);
		return STOP;
	}
	fseek(fp, off1, SEEK_SET);
	while (size > 0) {
		z = size > sizeof buf ? sizeof buf : size;
		if ((n = fread(buf, 1, z, fp)) != z ||
				fwrite(buf, 1, n, op) != n) {
			DEBUG_ERROR("Error writing to \"%s\".\n", tmp);
			Fclose(op);
			unlink(tmp);
			return STOP;
		}
		size -= n;
	}
	Fclose(op);
	new = salloc(n = strlen(name) + strlen(fn) + 6);
	snprintf(new, n, "%s/new/%s", name, fn);
	if (link(tmp, new) < 0) {
		DEBUG_ERROR("Cannot link \"%s\" to \"%s\".\n", tmp, new);
		return STOP;
	}
	if (unlink(tmp) < 0)
		DEBUG_ERROR("Cannot unlink \"%s\".\n", tmp);
	return OKAY;
}

static enum okay 
trycreate(const char *name)
{
	struct stat	st;

	if (stat(name, &st) == 0) {
		if (!S_ISDIR(st.st_mode)) {
			DEBUG_ERROR("\"%s\" is not a directory.\n", name);
			return STOP;
		}
	} else if (makedir(name) != OKAY) {
		DEBUG_ERROR("Cannot create directory \"%s\".\n", name);
		return STOP;
	} else
		imap_created_mailbox++;
	return OKAY;
}

static enum okay 
mkmaildir(const char *name)
{
	char	*np;
	size_t	sz;
	enum okay	ok = STOP;

	if (trycreate(name) == OKAY) {
		np = ac_alloc((sz = strlen(name)) + 5);
		strcpy(np, name);
		strcpy(&np[sz], "/tmp");
		if (trycreate(np) == OKAY) {
			strcpy(&np[sz], "/new");
			if (trycreate(np) == OKAY) {
				strcpy(&np[sz], "/cur");
				if (trycreate(np) == OKAY)
					ok = OKAY;
			}
		}
		ac_free(np);
	}
	return ok;
}

