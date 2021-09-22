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
static char sccsid[] = "@(#)collect.c	2.54 (gritter) 6/16/07";
#endif
#endif /* not lint */

/*
 * Mail -- a mail program
 *
 * Collect input from standard input, handling
 * ~ escapes.
 */

#include "rcv.h"
#include "extern.h"
#include <unistd.h>
#include <sys/stat.h>

/*
 * Read a message from standard output and return a read file to it
 * or NULL on error.
 */

/*
 * The following hokiness with global variables is so that on
 * receipt of an interrupt signal, the partial message can be salted
 * away on dead.letter.
 */

static	sighandler_type		saveint;	/* Previous SIGINT value */
static	sighandler_type		savehup;	/* Previous SIGHUP value */
static	sighandler_type		savetstp;	/* Previous SIGTSTP value */
static	sighandler_type		savettou;	/* Previous SIGTTOU value */
static	sighandler_type		savettin;	/* Previous SIGTTIN value */
static	FILE	*collf;			/* File for saving away */
static	int	hadintr;		/* Have seen one SIGINT so far */

static	sigjmp_buf	colljmp;	/* To get back to work */
static	int		colljmp_p;	/* whether to long jump */
static	sigjmp_buf	collabort;	/* To end collection with error */

static	sigjmp_buf pipejmp;		/* On broken pipe */

static void collint(int s);
static int putesc(const char *s, FILE *stream);

/*
 * Put the given file to the end of the attachment list.
 */
struct attachment *
add_attachment(struct attachment *attach, const char *file)
{
	struct attachment *ap, *nap;

	if (access(file, R_OK) != 0)
		return NULL;
	/*LINTED*/
	nap = csalloc(1, sizeof *nap);
	nap->a_name = salloc(strlen(file) + 1);
	strcpy(nap->a_name, file);
	if (attach != NULL) {
		for (ap = attach; ap->a_flink != NULL; ap = ap->a_flink);
		ap->a_flink = nap;
		nap->a_blink = ap;
	} else {
		nap->a_blink = NULL;
		attach = nap;
	}
	return attach;
}

static buf[BUFSIZ] = {0};
	
FILE *
collect(struct header *hp)
{
	FILE *fbuf;
	struct ignoretab *quoteig;
	int lc, cc, eofcount;
	int c, t;
	char *linebuf = NULL, *cp;
	size_t linesize;
	char *tempMail = NULL;
	int getfields;
	sigset_t oset, nset;
	long count;
	enum sendaction	action;
	sighandler_type	savedtop;

	(void) &eofcount;
	(void) &getfields;
	(void) &tempMail;

	collf = NULL;
	/*
	 * Start catching signals from here, but we're still die on interrupts
	 * until we're in the main loop.
	 */
	sigemptyset(&nset);
	sigaddset(&nset, SIGINT);
	sigaddset(&nset, SIGHUP);
	sigprocmask(SIG_BLOCK, &nset, &oset);
	handlerpush(collint);
	if ((saveint = safe_signal(SIGINT, SIG_IGN)) != SIG_IGN)
		safe_signal(SIGINT, collint);
	if ((savehup = safe_signal(SIGHUP, SIG_IGN)) != SIG_IGN)
		;
	if (sigsetjmp(collabort, 1)) {
		if (tempMail != NULL) {
			//rm(tempMail);
			Ftfree(&tempMail);
		}
		goto err;
	}
	if (sigsetjmp(colljmp, 1)) {
		if (tempMail != NULL) {
			//rm(tempMail);
			Ftfree(&tempMail);
		}
		goto err;
	}
	sigprocmask(SIG_SETMASK, &oset, (sigset_t *)NULL);

	noreset++;
	if ((collf = Ftemp(&tempMail, "Rs", "w+", 0600, 1)) == NULL) {
		perror(catgets(catd, CATSET, 51, "temporary mail file"));
		goto err;
	}
	unlink(tempMail);
	Ftfree(&tempMail);

	if ((cp = value("MAILX_HEAD")) != NULL) {
		putesc(cp, collf);
	}

	/*
	 * If we are going to prompt for a subject,
	 * refrain from printing a newline after
	 * the headers (since some people mind).
	 */
	getfields = 0;
		t = GTO|GSUBJECT|GCC|GNL;
		if (value("fullnames"))
			t |= GCOMMA;

	cp = value("escape");
	eofcount = 0;
	hadintr = 0;

	if (!sigsetjmp(colljmp, 1)) { 
	} else {
		/*
		 * Come here for printing the after-signal message.
		 * Duplicate messages won't be printed because
		 * the write is aborted if we get a SIGTTOU.
		 */
cont:
		if (hadintr) {
			fflush(stdout);
			DEBUG_ERROR( catgets(catd, CATSET, 53,
				"\n(Interrupt -- one more to kill letter)\n"));
		} else {
			DEBUG_PRINT(catgets(catd, CATSET, 54, "(continue)\n"));
			fflush(stdout);
		}
	}
	if (value("interactive") == NULL && tildeflag <= 0) {
		/*
		 * No tilde escapes, interrupts not expected. Copy
		 * standard input the simple way.
		 */
		linebuf = buf; 
		linesize = sizeof(buf);
		//linebuf = srealloc(linebuf, linesize = BUFSIZ);
		while ((count = fread(linebuf, sizeof *linebuf,
						linesize, stdin)) > 0) {
			if (fwrite(linebuf, sizeof *linebuf,
						count, collf) != count)
				goto err;
		}
		goto out;
	}
	goto out;
err:
	if (collf != NULL) {
		Fclose(collf);
		collf = NULL;
	}
out:
	if (collf != NULL) {
		if ((cp = value("MAILX_TAIL")) != NULL) {
			fflush(collf);
			putesc(cp, collf);
		}
		rewind(collf);
	}
	handlerpop();
	noreset--;
	sigemptyset(&nset);
	sigaddset(&nset, SIGINT);
	sigaddset(&nset, SIGHUP);
#ifndef OLDBUG
	sigprocmask(SIG_BLOCK, &nset, (sigset_t *)NULL);
#else
	sigprocmask(SIG_BLOCK, &nset, &oset);
#endif
	safe_signal(SIGINT, saveint);
	safe_signal(SIGHUP, savehup);
	safe_signal(SIGTSTP, savetstp);
	safe_signal(SIGTTOU, savettou);
	safe_signal(SIGTTIN, savettin);
	sigprocmask(SIG_SETMASK, &oset, (sigset_t *)NULL);
	return collf;
}

/*
 * On interrupt, come here to save the partial message in ~/dead.letter.
 * Then jump out of the collection loop.
 */
/*ARGSUSED*/
static void 
collint(int s)
{
	/*
	 * the control flow is subtle, because we can be called from ~q.
	 */
	if (!hadintr) {
		if (value("ignore") != NULL) {
			puts("@");
			fflush(stdout);
			clearerr(stdin);
			return;
		}
		hadintr = 1;
		siglongjmp(colljmp, 1);
	}
	exit_status |= 04;
	rewind(collf);
	if (value("save") != NULL && s != 0)
		savedeadletter(collf);
	siglongjmp(collabort, 1);
}

void
savedeadletter(FILE *fp)
{
	FILE *dbuf;
	int c, lines = 0, bytes = 0;
	char *cp;

	if (fsize(fp) == 0)
		return;
	cp = getdeadletter();
	c = umask(077);
	dbuf = Fopen(cp, "a");
	umask(c);
	if (dbuf == NULL)
		return;
	DEBUG_PRINT("\"%s\" ", cp);
	while ((c = getc(fp)) != EOF) {
		putc(c, dbuf);
		bytes++;
		if (c == '\n')
			lines++;
	}
	Fclose(dbuf);
	DEBUG_PRINT("%d/%d\n", lines, bytes);
	rewind(fp);
}

static int
putesc(const char *s, FILE *stream)
{
	int	n = 0;

	while (s[0]) {
		if (s[0] == '\\') {
			if (s[1] == 't') {
				putc('\t', stream);
				n++;
				s += 2;
				continue;
			}
			if (s[1] == 'n') {
				putc('\n', stream);
				n++;
				s += 2;
				continue;
			}
		}
		putc(s[0]&0377, stream);
		n++;
		s++;
	}
	putc('\n', stream);
	return ++n;
}
