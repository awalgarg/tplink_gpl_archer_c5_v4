/*
 * Heirloom mailx - a mail user agent derived from Berkeley Mail.
 *
 * Copyright (c) 2000-2004 Gunnar Ritter, Freiburg i. Br., Germany.
 */
/*
 * Copyright (c) 1980, 1993
 *	The Regents of the University of California.  All rights reserved.
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
 *	This product includes software developed by the University of
 *	California, Berkeley and its contributors.
 * 4. Neither the name of the University nor the names of its contributors
 *    may be used to endorse or promote products derived from this software
 *    without specific prior written permission.
 *
 * THIS SOFTWARE IS PROVIDED BY THE REGENTS AND CONTRIBUTORS ``AS IS'' AND
 * ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE
 * IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE
 * ARE DISCLAIMED.  IN NO EVENT SHALL THE REGENTS OR CONTRIBUTORS BE LIABLE
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
static char sccsid[] = "@(#)fio.c	2.76 (gritter) 9/16/09";
#endif
#endif /* not lint */

#include "rcv.h"
#include <sys/stat.h>
#include <sys/file.h>
#include <sys/wait.h>
#ifdef	HAVE_WORDEXP
#include <wordexp.h>
#endif	/* HAVE_WORDEXP */
#include <unistd.h>

#include <openssl/ssl.h>
#include <openssl/err.h>
#include <openssl/x509v3.h>
#include <openssl/x509.h>
#include <openssl/rand.h>
#include <sys/socket.h>
#include <netdb.h>
#include <netinet/in.h>
#ifdef	HAVE_ARPA_INET_H
#include <arpa/inet.h>
#endif	/* HAVE_ARPA_INET_H */

#include <errno.h>
#include "extern.h"

#include <mail.h>

/*
 * Mail -- a mail program
 *
 * File I/O.
 */
static char *globname(char *name);
static size_t length_of_line(const char *line, size_t linesize);
static char *fgetline_byone(char **line, size_t *linesize, size_t *llen,
		FILE *fp, int appendnl, size_t n);
static enum okay get_header(struct message *mp);

/*
 * Drop the passed line onto the passed output buffer.
 * If a write error occurs, return -1, else the count of
 * characters written, including the newline.
 */
int
putline(FILE *obuf, char *linebuf, size_t count)
{
	fwrite(linebuf, sizeof *linebuf, count, obuf);
	putc('\n', obuf);
	if (ferror(obuf))
		return (-1);
	return (count + 1);
}


static char line[LINESIZE] = {0};

/*
 * Read up a line from the specified input into the line
 * buffer.  Return the number of characters read.  Do not
 * include the newline at the end.
 *
 * n is the number of characters already read.
 */
int
readline_restart(FILE *ibuf, char **linebuf, size_t *linesize, size_t n)
{
	long sz;

	clearerr(ibuf);
	/*
	 * Interrupts will cause trouble if we are inside a stdio call. As
	 * this is only relevant if input comes from a terminal, we can simply
	 * bypass it by read() then.
	 */
	fileno(ibuf);
	
	/*
	 * Not reading from standard input or standard input not
	 * a terminal. We read one char at a time as it is the
	 * only way to get lines with embedded NUL characters in
	 * standard stdio.
	 */
	if (NULL == fgets(line, LINESIZE, ibuf))
		return -1;

	*linebuf = line;
	*linesize = strlen(line);
	n = strlen(line);
	
	if (n > 0 && (*linebuf)[n - 1] == '\n')
		(*linebuf)[--n] = '\0';
	return n;
}

/*
 * Return a file buffer all ready to read up the
 * passed message pointer.
 */
FILE *
setinput(struct mailbox *mp, struct message *m, enum needspec need)
{
	enum okay ok = STOP;

	switch (need) {
	case NEED_HEADER:
		if (m->m_have & HAVE_HEADER)
			ok = OKAY;
		else
			ok = get_header(m);
		break;
	case NEED_BODY:
		if (m->m_have & HAVE_BODY)
			ok = OKAY;
		else
			ok = get_body(m);
		break;
	case NEED_UNSPEC:
		ok = OKAY;
		break;
	}
	if (ok != OKAY)
		return NULL;
	fflush(mp->mb_otf);
	if (fseek(mp->mb_itf, (long)mailx_positionof(m->m_block,
					m->m_offset), SEEK_SET) < 0) {
		perror("fseek");
		panic(catgets(catd, CATSET, 77, "temporary file seek"));
	}
	return (mp->mb_itf);
}

struct message *
setdot(struct message *mp)
{
	if (dot != mp) {
		prevdot = dot;
		did_print_dot = 0;
	}
	dot = mp;
	uncollapse1(dot, 0);
	return dot;
}

/*
 * Delete a file, but only if the file is a plain file.
 */
int
rm(char *name)
{
	struct stat sb;

	if (stat(name, &sb) < 0)
		return(-1);
	if (!S_ISREG(sb.st_mode)) {
		errno = EISDIR;
		return(-1);
	}
	return(unlink(name));
}

static int sigdepth;		/* depth of holdsigs() */
static sigset_t nset, oset;
/*
 * Hold signals SIGHUP, SIGINT, and SIGQUIT.
 */
void
holdsigs(void)
{

	if (sigdepth++ == 0) {
		sigemptyset(&nset);
		sigaddset(&nset, SIGHUP);
		sigaddset(&nset, SIGINT);
		sigaddset(&nset, SIGQUIT);
		sigprocmask(SIG_BLOCK, &nset, &oset);
	}
}

/*
 * Release signals SIGHUP, SIGINT, and SIGQUIT.
 */
void
relsesigs(void)
{

	if (--sigdepth == 0)
		sigprocmask(SIG_SETMASK, &oset, (sigset_t *)NULL);
}

/*
 * Determine the size of the file possessed by
 * the passed buffer.
 */
off_t
fsize(FILE *iob)
{
	struct stat sbuf;

	if (fstat(fileno(iob), &sbuf) < 0)
		return 0;
	return sbuf.st_size;
}

/*
 * Evaluate the string given as a new mailbox name.
 * Supported meta characters:
 *	%	for my system mail box
 *	%user	for user's system mail box
 *	#	for previous file
 *	&	invoker's mbox file
 *	+file	file in folder directory
 *	any shell meta character
 * Return the file name as a dynamic string.
 */
char *
expand(char *name)
{
	char xname[PATHSIZE];
	char foldbuf[PATHSIZE];
	struct shortcut *sh;

	/*
	 * The order of evaluation is "%" and "#" expand into constants.
	 * "&" can expand into "+".  "+" can expand into shell meta characters.
	 * Shell meta characters expand into constants.
	 * This way, we make no recursive expansion.
	 */
	if ((sh = get_shortcut(name)) != NULL)
		name = sh->sh_long;
next:
	switch (*name) {
	case '%':
		if (name[1] == ':' && name[2]) {
			name = &name[2];
			goto next;
		}
		findmail(name[1] ? name + 1 : myname, name[1] != '\0' || uflag,
				xname, sizeof xname);
		return savestr(xname);
	case '#':
		if (name[1] != 0)
			break;
		if (prevfile[0] == 0) {
			DEBUG_PRINT(catgets(catd, CATSET, 80, "No previous file\n"));
			return NULL;
		}
		return savestr(prevfile);
	case '&':
		if (name[1] == 0 && (name = value("MBOX")) == NULL)
			name = "~/mbox";
		/* fall through */
	}
	if (name[0] == '@' && which_protocol(mailname) == PROTO_IMAP) {
		snprintf(xname, sizeof xname, "%s/%s", protbase(mailname),
				&name[1]);
		name = savestr(xname);
	}
	if (name[0] == '+' && getfold(foldbuf, sizeof foldbuf) >= 0) {
		if (which_protocol(foldbuf) == PROTO_IMAP &&
				strcmp(foldbuf, protbase(foldbuf)))
		snprintf(xname, sizeof xname, "%s%s", foldbuf, name+1);
		else
			snprintf(xname, sizeof xname, "%s/%s", foldbuf, name+1);
		name = savestr(xname);
		if (foldbuf[0] == '%' && foldbuf[1] == ':')
			goto next;
	}
	/* catch the most common shell meta character */
	if (name[0] == '~' && (name[1] == '/' || name[1] == '\0')) {
		snprintf(xname, sizeof xname, "%s%s", homedir, name + 1);
		name = savestr(xname);
	}
	if (!anyof(name, "|&;<>~{}()[]*?$`'\"\\"))
		return name;
	if (which_protocol(name) == PROTO_FILE)
		return globname(name);
	else
		return name;
}

static char *
globname(char *name)
{
#ifdef	HAVE_WORDEXP
	wordexp_t we;
	char *cp;
	sigset_t nset;
	int i;

	/*
	 * Some systems (notably Open UNIX 8.0.0) fork a shell for
	 * wordexp() and wait for it; waiting will fail if our SIGCHLD
	 * handler is active.
	 */
	sigemptyset(&nset);
	sigaddset(&nset, SIGCHLD);
	sigprocmask(SIG_BLOCK, &nset, NULL);
	i = wordexp(name, &we, 0);
	sigprocmask(SIG_UNBLOCK, &nset, NULL);
	switch (i) {
	case 0:
		break;
	case WRDE_NOSPACE:
		DEBUG_ERROR( catgets(catd, CATSET, 83,
				"\"%s\": Expansion buffer overflow.\n"), name);
		return NULL;
	case WRDE_BADCHAR:
	case WRDE_SYNTAX:
	default:
		DEBUG_ERROR( catgets(catd, CATSET, 242,
				"Syntax error in \"%s\"\n"), name);
		return NULL;
	}
	switch (we.we_wordc) {
	case 1:
		cp = savestr(we.we_wordv[0]);
		break;
	case 0:
		DEBUG_ERROR( catgets(catd, CATSET, 82,
					"\"%s\": No match.\n"), name);
		cp = NULL;
		break;
	default:
		DEBUG_ERROR( catgets(catd, CATSET, 84,
				"\"%s\": Ambiguous.\n"), name);
		cp = NULL;
	}
	wordfree(&we);
	return cp;
#else	/* !HAVE_WORDEXP */
	char xname[PATHSIZE];
	char cmdbuf[PATHSIZE];		/* also used for file names */
	int pid, l;
	char *cp, *shell;
	int pivec[2];
	extern int wait_status;
	struct stat sbuf;

	if (pipe(pivec) < 0) {
		perror("pipe");
		return name;
	}
	snprintf(cmdbuf, sizeof cmdbuf, "echo %s", name);
	if ((shell = value("SHELL")) == NULL)
		shell = SHELL;
	pid = start_command(shell, 0, -1, pivec[1], "-c", cmdbuf, NULL);
	if (pid < 0) {
		close(pivec[0]);
		close(pivec[1]);
		return NULL;
	}
	close(pivec[1]);
again:
	l = read(pivec[0], xname, sizeof xname);
	if (l < 0) {
		if (errno == EINTR)
			goto again;
		perror("read");
		close(pivec[0]);
		return NULL;
	}
	close(pivec[0]);
	if (wait_child(pid) < 0 && WTERMSIG(wait_status) != SIGPIPE) {
		DEBUG_ERROR( catgets(catd, CATSET, 81,
				"\"%s\": Expansion failed.\n"), name);
		return NULL;
	}
	if (l == 0) {
		DEBUG_ERROR( catgets(catd, CATSET, 82,
					"\"%s\": No match.\n"), name);
		return NULL;
	}
	if (l == sizeof xname) {
		DEBUG_ERROR( catgets(catd, CATSET, 83,
				"\"%s\": Expansion buffer overflow.\n"), name);
		return NULL;
	}
	xname[l] = 0;
	for (cp = &xname[l-1]; *cp == '\n' && cp > xname; cp--)
		;
	cp[1] = '\0';
	if (strchr(xname, ' ') && stat(xname, &sbuf) < 0) {
		DEBUG_ERROR( catgets(catd, CATSET, 84,
				"\"%s\": Ambiguous.\n"), name);
		return NULL;
	}
	return savestr(xname);
#endif	/* !HAVE_WORDEXP */
}

/*
 * Determine the current folder directory name.
 */
int
getfold(char *name, int size)
{
	char *folder;
	enum protocol	p;

	if ((folder = value("folder")) == NULL)
		return (-1);
	if (*folder == '/' || (p = which_protocol(folder)) != PROTO_FILE &&
			p != PROTO_MAILDIR) {
		strncpy(name, folder, size);
		name[size-1]='\0';
	} else {
		snprintf(name, size, "%s/%s", homedir, folder);
	}
	return (0);
}

/*
 * Return the name of the dead.letter file.
 */
char *
getdeadletter(void)
{
	char *cp;

	if ((cp = value("DEAD")) == NULL || (cp = expand(cp)) == NULL)
		cp = expand("~/dead.letter");
	else if (*cp != '/') {
		char *buf;
		size_t sz;

		buf = ac_alloc(sz = strlen(cp) + 3);
		snprintf(buf, sz, "~/%s", cp);
		snprintf(buf, sz, "~/%s", cp);
		cp = expand(buf);
		ac_free(buf);
	}
	return cp;
}

/*
 * line is a buffer with the result of fgets(). Returns the first
 * newline or the last character read.
 */
static size_t
length_of_line(const char *line, size_t linesize)
{
	register size_t i;

	/*
	 * Last character is always '\0' and was added by fgets.
	 */
	linesize--;
	for (i = 0; i < linesize; i++)
		if (line[i] == '\n')
			break;
	return i < linesize ? i + 1 : linesize;
}

/*
 * fgets replacement to handle lines of arbitrary size and with
 * embedded \0 characters.
 * line - line buffer. *line be NULL.
 * linesize - allocated size of line buffer.
 * count - maximum characters to read. May be NULL.
 * llen - length_of_line(*line).
 * fp - input FILE.
 * appendnl - always terminate line with \n, append if necessary.
 */
char *
fgetline(char **line, size_t *linesize, size_t *count, size_t *llen,
		FILE *fp, int appendnl)
{
	long i_llen, sz;

	if (count == NULL)
		/*
		 * If we have no count, we cannot determine where the
		 * characters returned by fgets() end if there was no
		 * newline. We have to read one character at one.
		 */
		return fgetline_byone(line, linesize, llen, fp, appendnl, 0);

	if (*line == NULL || *linesize < LINESIZE)
		*line = srealloc(*line, *linesize = LINESIZE);
	sz = *linesize <= *count ? *linesize : *count + 1;
	if (sz <= 1 || fgets(*line, sz, fp) == NULL)
		/*
		 * Leave llen untouched; it is used to determine whether
		 * the last line was \n-terminated in some callers.
		 */
		return NULL;
	i_llen = length_of_line(*line, sz);
	*count -= i_llen;
	while ((*line)[i_llen - 1] != '\n') {
		*line = srealloc(*line, *linesize += 256);
		sz = *linesize - i_llen;
		sz = (sz <= *count ? sz : *count + 1);
		if (sz <= 1 || fgets(&(*line)[i_llen], sz, fp) == NULL) {
			if (appendnl) {
				(*line)[i_llen++] = '\n';
				(*line)[i_llen] = '\0';
			}
			break;
		}
		sz = length_of_line(&(*line)[i_llen], sz);
		i_llen += sz;
		*count -= sz;
	}
	if (llen)
		*llen = i_llen;
	return *line;
}

/*
 * Read a line, one character at once.
 */
static char *
fgetline_byone(char **line, size_t *linesize, size_t *llen,
		FILE *fp, int appendnl, size_t n)
{
	int c;

	if (*line == NULL || *linesize < LINESIZE + n + 1)
		*line = srealloc(*line, *linesize = LINESIZE + n + 1);
	for (;;) {
		if (n >= *linesize - 128)
			*line = srealloc(*line, *linesize += 256);
		c = getc(fp);
		if (c != EOF) {
			(*line)[n++] = c;
			(*line)[n] = '\0';
			if (c == '\n')
				break;
		} else {
			if (n > 0) {
				if (appendnl) {
					(*line)[n++] = '\n';
					(*line)[n] = '\0';
				}
				break;
			} else
				return NULL;
		}
	}
	if (llen)
		*llen = n;
	return *line;
}

static enum okay
get_header(struct message *mp)
{
	switch (mb.mb_type) {
	case MB_FILE:
	case MB_MAILDIR:
		return OKAY;
	case MB_POP3:
		return STOP;
	case MB_IMAP:
	case MB_CACHE:
		return STOP;
	case MB_VOID:
		return STOP;
	}
	/*NOTREACHED*/
	return STOP;
}

enum okay
get_body(struct message *mp)
{
	switch (mb.mb_type) {
	case MB_FILE:
	case MB_MAILDIR:
		return OKAY;
	case MB_POP3:
		return STOP;
	case MB_IMAP:
	case MB_CACHE:
		return STOP;
	case MB_VOID:
		return STOP;
	}
	/*NOTREACHED*/
	return STOP;
}

static long xwrite(int fd, const char *data, size_t sz);

static long
xwrite(int fd, const char *data, size_t sz)
{
	long wo, wt = 0;

	do {
		if ((wo = write(fd, data + wt, sz - wt)) < 0) {
			if (errno == EINTR)
				continue;
			else
				return -1;
		}
		wt += wo;
	} while (wt < sz);
	return sz;
}

int
sclose(struct sock *sp)
{
	int	i;

	if (sp->s_fd > 0) {
		if (sp->s_onclose != NULL)
			(*sp->s_onclose)();
		if (sp->s_use_ssl) {
			sp->s_use_ssl = 0;
			SSL_shutdown(sp->s_ssl);
			SSL_free(sp->s_ssl);
			sp->s_ssl = NULL;
			SSL_CTX_free(sp->s_ctx);
			sp->s_ctx = NULL;
		}
		{
			i = close(sp->s_fd);
		}
		sp->s_fd = -1;
		return i;
	}
	sp->s_fd = -1;
	return 0;
}

enum okay
swrite(struct sock *sp, const char *data)
{
	return swrite1(sp, data, strlen(data), 0);
}

#define SOCKET_BUF_SIZE 4096

static char wbuf[SOCKET_BUF_SIZE];

enum okay
swrite1(struct sock *sp, const char *data, int sz, int use_buffer)
{
	int	x;

	if (use_buffer > 0) {
		int	di;
		enum okay	ok;

		if (sp->s_wbuf == NULL) {
			sp->s_wbufsize = SOCKET_BUF_SIZE;
			sp->s_wbuf = wbuf;
			sp->s_wbufpos = 0;
		}
		while (sp->s_wbufpos + sz > sp->s_wbufsize) {
			di = sp->s_wbufsize - sp->s_wbufpos;
			sz -= di;
			if (sp->s_wbufpos > 0) {
				memcpy(&sp->s_wbuf[sp->s_wbufpos], data, di);
				ok = swrite1(sp, sp->s_wbuf,
						sp->s_wbufsize, -1);
			} else
				ok = swrite1(sp, data,
						sp->s_wbufsize, -1);
			if (ok != OKAY)
				return STOP;
			data += di;
			sp->s_wbufpos = 0;
		}
		if (sz == sp->s_wbufsize) {
			ok = swrite1(sp, data, sp->s_wbufsize, -1);
			if (ok != OKAY)
				return STOP;
		} else if (sz) {
			memcpy(&sp->s_wbuf[sp->s_wbufpos], data, sz);
			sp->s_wbufpos += sz;
		}
		return OKAY;
	} 
	else if (use_buffer == 0 && sp->s_wbuf != NULL &&
			sp->s_wbufpos > 0) {
		x = sp->s_wbufpos;
		sp->s_wbufpos = 0;
		if (swrite1(sp, sp->s_wbuf, x, -1) != OKAY)
			return STOP;
	}
	if (sz == 0)
		return OKAY;
	if (sp->s_use_ssl) {
ssl_retry:	x = SSL_write(sp->s_ssl, data, sz);
		if (x < 0) {
			switch (SSL_get_error(sp->s_ssl, x)) {
			case SSL_ERROR_WANT_READ:
			case SSL_ERROR_WANT_WRITE:
				goto ssl_retry;
			}
		}
	} else
	{
		x = xwrite(sp->s_fd, data, sz);
	}
	if (x != sz) {
		char	o[512];
		snprintf(o, sizeof o, "%s write error",
				sp->s_desc ? sp->s_desc : "socket");
		sp->s_use_ssl ? ssl_gen_err("%s", o) : perror(o);
		if (x < 0)
			sclose(sp);
		return STOP;
	}
	return OKAY;
}

int
sgetline(char **line, size_t *linesize, size_t *linelen, struct sock *sp)
{
	char	*lp = *line;

	if (sp->s_rsz < 0) {
		sclose(sp);
		return sp->s_rsz;
	}
	do {
		if (*line == NULL || lp > &(*line)[*linesize - 128]) {
			size_t diff = lp - *line;
			*line = srealloc(*line, *linesize += 256);
			lp = &(*line)[diff];
		}
		if (sp->s_rbufptr == NULL ||
				sp->s_rbufptr >= &sp->s_rbuf[sp->s_rsz]) {
			if (sp->s_use_ssl) {
		ssl_retry:	if ((sp->s_rsz = SSL_read(sp->s_ssl,
						sp->s_rbuf,
						sizeof sp->s_rbuf)) <= 0) {
					if (sp->s_rsz < 0) {
						char	o[512];
						switch(SSL_get_error(sp->s_ssl,
							sp->s_rsz)) {
						case SSL_ERROR_WANT_READ:
						case SSL_ERROR_WANT_WRITE:
							goto ssl_retry;
						}
						snprintf(o, sizeof o, "%s",
							sp->s_desc ?
								sp->s_desc :
								"socket");
						ssl_gen_err("%s", o);

					}
					break;
				}
			} else{
			again:	if ((sp->s_rsz = read(sp->s_fd, sp->s_rbuf,
						sizeof sp->s_rbuf)) <= 0) {
					if (sp->s_rsz < 0) {
						char	o[512];
						if (errno == EINTR)
							goto again;
						snprintf(o, sizeof o, "%s",
							sp->s_desc ?
								sp->s_desc :
								"socket");
						perror(o);
					}
					break;
				}
			}
			sp->s_rbufptr = sp->s_rbuf;
		}
	} while ((*lp++ = *sp->s_rbufptr++) != '\n');
	*lp = '\0';
	if (linelen)
		*linelen = lp - *line;
	return lp - *line;
}

enum okay
sopen(const char *xserver, struct sock *sp, int use_ssl,
		const char *uhp, const char *portstr)
{
	char	hbuf[NI_MAXHOST];
	struct addrinfo	hints, *res0, *res;
	int	sockfd;
	char	*cp;
	char	*server = (char *)xserver;

	if ((cp = strchr(server, ':')) != NULL) {
		portstr = &cp[1];
		server = salloc(cp - xserver + 1);
		memcpy(server, xserver, cp - xserver);
		server[cp - xserver] = '\0';
	}
	memset(&hints, 0, sizeof hints);
	hints.ai_socktype = SOCK_STREAM;

	DEBUG_PRINT("Resolving host %s . . .", server);
	DEBUG_PRINT("portstr is %s", portstr);
	
	if (getaddrinfo(server, portstr, &hints, &res0) != 0) {
		DEBUG_ERROR( catgets(catd, CATSET, 252,
				"Could not resolve host: %s\n"), server);
		mail_exit(MAIL_STATUS_ERR_RESOLVE);
	} else {
		DEBUG_PRINT(" done.\n");
	}
	
	sockfd = -1;
	for (res = res0; res != NULL && sockfd < 0; res = res->ai_next) {
		if (getnameinfo(res->ai_addr, res->ai_addrlen,
						hbuf, sizeof hbuf, NULL, 0,
						NI_NUMERICHOST) != 0)
				strcpy(hbuf, "unknown host");
			DEBUG_PRINT( catgets(catd, CATSET, 192,
					"Connecting to %s:%s . . ."),
					hbuf, portstr);

		if ((sockfd = socket(res->ai_family, res->ai_socktype,
				res->ai_protocol)) >= 0) {
			if (connect(sockfd, res->ai_addr, res->ai_addrlen)!=0) {
				close(sockfd);
				sockfd = -1;
			}
		}
	}
	if (sockfd < 0) {
		perror(catgets(catd, CATSET, 254, "could not connect"));
		freeaddrinfo(res0);
		mail_exit(MAIL_STATUS_ERR_CONNECT);
	}
	freeaddrinfo(res0);

	DEBUG_PRINT(catgets(catd, CATSET, 193, " connected.\n"));
	memset(sp, 0, sizeof *sp);
	sp->s_fd = sockfd;
	if (use_ssl) {
		enum okay ok;
		if ((ok = ssl_open(server, sp, uhp)) != OKAY) {
			sclose(sp);
			mail_exit(MAIL_STATUS_ERR_SSL);
		}
		return ok;
	}
	return OKAY;
}
