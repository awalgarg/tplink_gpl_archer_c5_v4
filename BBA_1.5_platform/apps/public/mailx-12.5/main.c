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
static char copyright[]
= "@(#) Copyright (c) 1980, 1993 The Regents of the University of California.  All rights reserved.\n";
static char sccsid[] = "@(#)main.c	2.51 (gritter) 10/1/07";
#endif	/* DOSCCS */
#endif /* not lint */

/*
 * Most strcpy/sprintf functions have been changed to strncpy/snprintf to
 * correct several buffer overruns (at least one ot them was exploitable).
 * Sat Jun 20 04:58:09 CEST 1998 Alvaro Martinez Echevarria <alvaro@lander.es>
 * ---
 * Note: We set egid to realgid ... and only if we need the egid we will
 *       switch back temporary.  Nevertheless, I do not like seg faults.
 *       Werner Fink, <werner@suse.de>
 */


#include "config.h"
#ifdef	HAVE_NL_LANGINFO
#include <langinfo.h>
#endif	/* HAVE_NL_LANGINFO */
#define _MAIL_GLOBS_
#include "rcv.h"
#include "extern.h"
#include <sys/stat.h>
#include <sys/ioctl.h>
#include <termios.h>
#include <unistd.h>
#include <fcntl.h>
#ifdef	HAVE_SETLOCALE
#include <locale.h>
#endif	/* HAVE_SETLOCALE */

#include <mail.h>

#define ATTACHMENT_MAX 8

/*
 * Mail -- a mail program
 *
 * Startup -- interface with user.
 */

static sigjmp_buf	hdrjmp;
char	*progname;
sighandler_type	dflpipe = SIG_DFL;
char *mailrc = NULL;

int 
main(int argc, char *argv[])
{
	const char optstr[] = "A:BHEFINVT:RS:a:b:c:dDefh:inqr:p:s:tu:xv~";
	int i;
	struct name *to, *smopts;
	struct attachment *attach;
	char *subject, *cp, *fromaddr = NULL;
	sighandler_type prevint;
	/*
	 * Absolutely the first thing we do is save our egid
	 * and set it to the rgid, so that we can safely run
	 * setgid.  We use the sgid (saved set-gid) to allow ourselves
	 * to revert to the egid if we want (temporarily) to become
	 * priveliged.
	 */
	effectivegid = getegid();
	realgid = getgid();
	if (setgid(realgid) < 0) {
		perror("setgid");
		mail_exit(MAIL_STATUS_ERR_GENERAL);
	}

	starting = 1;
	progname = strrchr(argv[0], '/');
	if (progname != NULL)
		progname++;
	else
		progname = argv[0];
	/*
	 * Set up a reasonable environment.
	 * Figure out whether we are being run interactively,
	 * start the SIGCHLD catcher, and so forth.
	 */
	safe_signal(SIGCHLD, sigchild);
	is_a_tty[0] = isatty(0);
	is_a_tty[1] = isatty(1);
	assign("header", "");
	assign("save", "");
#ifdef	HAVE_SETLOCALE
	setlocale(LC_CTYPE, "");
	setlocale(LC_COLLATE, "");
	setlocale(LC_MESSAGES, "");
	mb_cur_max = MB_CUR_MAX;
#if defined (HAVE_NL_LANGINFO) && defined (CODESET)
	if (value("ttycharset") == NULL && (cp = nl_langinfo(CODESET)) != NULL)
		assign("ttycharset", cp);
#endif	/* HAVE_NL_LANGINFO && CODESET */
#if defined (HAVE_MBTOWC) && defined (HAVE_WCTYPE_H)
	if (mb_cur_max > 1) {
		wchar_t	wc;
		if (mbtowc(&wc, "\303\266", 2) == 2 && wc == 0xF6 &&
				mbtowc(&wc, "\342\202\254", 3) == 3 &&
				wc == 0x20AC)
			utf8 = 1;
	}
#endif	/* HAVE_MBTOWC && HAVE_WCTYPE_H */
#else	/* !HAVE_SETLOCALE */
	mb_cur_max = 1;
#endif	/* !HAVE_SETLOCALE */
	image = -1;
	/*
	 * Now, determine how we are being used.
	 * We successively pick off - flags.
	 * If there is anything left, it is the base of the list
	 * of users to mail to.  Argp will be set to point to the
	 * first of these users.
	 */
	to = NULL;
	attach = NULL;
	smopts = NULL;
	subject = NULL;
	
	while ((i = getopt(argc, argv, optstr)) != EOF) {
		switch (i) {
		case 'c':
			/* config file path */
			mailrc = optarg;
			break;
		case '?':
			DEBUG_ERROR("unknown argument format\n");
			mail_exit(MAIL_STATUS_ERR_GENERAL);
		}
	}

	tinit();
	input = stdin;
	rcvmode = 0;
	spreserve();

	if (NULL == mailrc)
	{
		mail_exit(MAIL_STATUS_ERR_GENERAL);
	}
	else
	{
		load(mailrc);
	}
	
	subject = value("mail-subject");

	char attachment[256] = {0};
	int index = 0;
	for (index = 0; index < ATTACHMENT_MAX; index++)
	{
		memset(attachment, 0, sizeof(attachment));
		sprintf(attachment, "mail-attachment-%d", index);
		if (value(attachment))
		{
			attach = add_attachment(attach, value(attachment));
		}
	}

	/*
	 * 占位符，由于在check connection时不实际发送邮件，
	 * 实际上与to的值无关，但为了通过后面的各种条件检测，
	 * 就在这里赋以一个非空值。
	 */
	if (0 == strcmp(value("mail-type"), "check-connection"))
	{
		assign("mail-to", "test@tp-link.net");
	}
	
	to = checkaddrs(cat(to, extract(value("mail-to"), GTO|GFULL)));

	/*
	 * Check for inconsistent arguments.
	 */
	if (to == NULL){
		DEBUG_ERROR( catgets(catd, CATSET, 138,
			"Send options without primary recipient specified.\n"));
		mail_exit(MAIL_STATUS_ERR_USER);
	}
	
	starting = 0;

	/*
	 * From address from command line overrides rc files.
	 */
	if (fromaddr)
		assign("from", fromaddr);

	if (!rcvmode) {
		int ok = mail(to, smopts, subject, attach);

		if (senderr || !ok){
			mail_exit(MAIL_STATUS_ERR_GENERAL);
		} else {
			mail_exit(MAIL_STATUS_CHECKING);
		}
	}
}
