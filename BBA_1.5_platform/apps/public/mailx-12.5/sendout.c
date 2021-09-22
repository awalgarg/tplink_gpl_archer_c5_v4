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
static char sccsid[] = "@(#)sendout.c	2.100 (gritter) 3/1/09";
#endif
#endif /* not lint */

#include "rcv.h"
#include "extern.h"
#include <errno.h>
#include <sys/stat.h>
#include <fcntl.h>
#include <unistd.h>
#include <time.h>
#include "md5.h"

#include <mail.h>

/*
 * Mail -- a mail program
 *
 * Mail to others.
 */

static char	*send_boundary;

static FILE *fTempMail = NULL;
static struct name *fixhead(struct header *hp, struct name *tolist);
static void message_id(FILE *fo, struct header *hp);
static int fmt(char *str, struct name *np, FILE *fo, int comma,
		int dropinvalid, int domime);

/*
 * Generate a boundary for MIME multipart messages.
 */
char *
makeboundary(void)
{
	static char bound[70];
	time_t	now;

	time(&now);
	snprintf(bound, sizeof bound, "=_%lx.%s", (long)now, getrandstring(48));
	send_boundary = bound;
	return send_boundary;
}

/*
 * Get an encoding flag based on the given string.
 */
char *
getencoding(enum conversion convert)
{
	switch (convert) {
	case CONV_7BIT:
		return "7bit";
	case CONV_8BIT:
		return "8bit";
	case CONV_TOB64:
		return "base64";
	default:
		break;
	}
	/*NOTREACHED*/
	return NULL;
}

/*
 * Fix the header by glopping all of the expanded names from
 * the distribution list into the appropriate fields.
 */
static struct name *
fixhead(struct header *hp, struct name *tolist)
{
	struct name *np;

	hp->h_to = NULL;
	hp->h_cc = NULL;
	hp->h_bcc = NULL;
	for (np = tolist; np != NULL; np = np->n_flink)
		if ((np->n_type & GMASK) == GTO)
			hp->h_to =
				cat(hp->h_to, nalloc(np->n_fullname,
							np->n_type|GFULL));
		else if ((np->n_type & GMASK) == GCC)
			hp->h_cc =
				cat(hp->h_cc, nalloc(np->n_fullname,
							np->n_type|GFULL));
		else if ((np->n_type & GMASK) == GBCC)
			hp->h_bcc =
				cat(hp->h_bcc, nalloc(np->n_fullname,
							np->n_type|GFULL));
	return tolist;
}


/*
 * Do not change, you get incorrect base64 encodings else!
 */
#ifndef INFIX_BUF
#define	INFIX_BUF	972
#endif

/*
 * Put the signature file at fo.
 */
int
put_signature(FILE *fo, int convert)
{
	char *sig, buf[INFIX_BUF], c = '\n';
	FILE *fsig;
	size_t sz;

	sig = value("signature");
	if (sig == NULL || *sig == '\0')
		return 0;
	else
		sig = expand(sig);
	if ((fsig = Fopen(sig, "r")) == NULL) {
		perror(sig);
		return -1;
	}
	while ((sz = fread(buf, sizeof *buf, INFIX_BUF, fsig)) != 0) {
		c = buf[sz - 1];
		if (mime_write(buf, sz, fo, convert, TD_NONE,
					NULL, (size_t)0, NULL, NULL)
				== 0) {
			perror(sig);
			Fclose(fsig);
			return -1;
		}
	}
	if (ferror(fsig)) {
		perror(sig);
		Fclose(fsig);
		return -1;
	}
	Fclose(fsig);
	if (c != '\n')
		putc('\n', fo);
	return 0;
}

static char obuf[INFIX_BUF] = {0};

/*
 * Write an attachment to the file buffer, converting to MIME.
 */
int
attach_file1(struct attachment *ap, enum conversion convert, enum mimeclean isclean, FILE *fi, FILE *fo)
{
	int err = 0;
	size_t sz;
	char *buf;
	size_t bufsize, count;
	int	lastc = EOF;

	bufsize = INFIX_BUF;
	buf = obuf;
	
	int totalSize = 0;
	
	for (;;) {
		if (totalSize >= 4096 * 16)
			break;

		if ((sz = fread(buf, sizeof(char), bufsize, fi)) == 0)
				break;
		
		totalSize += sz;
		lastc = buf[sz-1];
		if (mime_write(buf, sz, fo, convert, TD_ICONV,
					NULL, (size_t)0, NULL, NULL) == 0)
			err = -1;
	}
	if (ferror(fi))
		err = -1;

	if ((err == 0) && (sz != 0))
		err = 1;

	return err;
}


/*
 * Try out different character set conversions to attach a file.
 */
int
attach_file(struct attachment *ap, enum conversion convert, enum mimeclean isclean, FILE *fi, FILE *fo)
{
	char	*_wantcharset, *charsets, *ncs;
	size_t	offs = ftell(fo);
	int status = 0;

	if (ap->a_charset || (charsets = value("sendcharsets")) == NULL)
		return attach_file1(ap, convert, isclean, fi, fo);

	_wantcharset = wantcharset;
	wantcharset = savestr(charsets);
loop:	if ((ncs = strchr(wantcharset, ',')) != NULL)
		*ncs++ = '\0';
try1:	if ((status = attach_file1(ap, convert, isclean, fi, fo)) < 0) {
			if (errno == EILSEQ || errno == EINVAL) {
				if (ncs && *ncs) {
					wantcharset = ncs;
					clearerr(fo);
					fseek(fo, offs, SEEK_SET);
					goto loop;
				}
				if (wantcharset) {
					if (wantcharset == (char *)-1)
						wantcharset = NULL;
					else {
						wantcharset = (char *)-1;
						clearerr(fo);
						fseek(fo, offs, SEEK_SET);
						goto try1;
					}
				}
			}
		}
	wantcharset = _wantcharset;
	return status;
}

/*
 * Interface between the argument list and the mail1 routine
 * which does all the dirty work.
 */
int 
mail(struct name *to, struct name *smopts, char *subject, struct attachment *attach)
{
	struct header head;
	struct str in, out;

	memset(&head, 0, sizeof head);
	/* The given subject may be in RFC1522 format. */
	if (subject != NULL) {
		in.s = subject;
		in.l = strlen(subject);
		mime_fromhdr(&in, &out, TD_ISPR | TD_ICONV);
		head.h_subject = out.s;
	}
		head.h_to = to;
		head.h_cc = NULL;
		head.h_bcc = NULL;
	head.h_attach = attach;
	head.h_smopts = smopts;
	int ok = mail1(&head);
	if (subject != NULL)
		free(out.s);
	return(ok);
}

/*
 * Mail a message on standard input to the people indicated
 * in the passed header.  (Internal interface).
 */
enum okay 
mail1(struct header *hp)
{
	struct name *to;
	FILE *mtf, *nmtf;
	enum okay	ok = STOP;
	char	*charsets, *ncs = NULL, *cp;

	char *smtp;
	char *user = NULL;
	char *password = NULL;
	char *skinned = NULL;
	struct termios	otio;
	int	reset_tio;
	
	if ((smtp = value("smtp")) != NULL) {
		skinned = skin(myorigin(hp));
		if ((user = smtp_auth_var("-user", skinned)) != NULL &&
				(password = smtp_auth_var("-password",
					skinned)) == NULL)
			{
				password = getpassword(&otio, &reset_tio, NULL);
			}
	}
	
	cp = value("autocc");
	cp = value("autobcc");

	/*
	 * Collect user's mail from standard input.
	 * Get the result as mtf.
	 */
	if ((mtf = collect(hp)) == NULL)
	{
		mail_exit(MAIL_STATUS_ERR_GENERAL);
	}

	if (fsize(mtf) == 0) {
		if (hp->h_subject == NULL)
			DEBUG_PRINT(catgets(catd, CATSET, 184,
				"No message, no subject; hope that's ok\n"));
		else if (value("bsdcompat") || value("bsdmsgs"))
			DEBUG_PRINT(catgets(catd, CATSET, 185,
				"Null message body; hope that's ok\n"));
	}
	/*
	 * Now, take the user names from the combined
	 * to and cc lists and do all the alias
	 * processing.
	 */
	senderr = 0;

	if ((to = usermap(cat(hp->h_bcc, cat(hp->h_to, hp->h_cc)))) == NULL) {
			DEBUG_PRINT(catgets(catd, CATSET, 186, "No recipients specified\n"));
			mail_exit(MAIL_STATUS_ERR_EMAIL);
	}
	to = fixhead(hp, to);

	wantcharset = NULL;

	to = elide(to);
	if (count(to) == 0) {
		mail_exit(MAIL_STATUS_ERR_EMAIL);
	}

	if (to) {
		pid_t pid;
		sigset_t nset;
		
		/*
		 * Fork, set up the temporary mail file as standard
		 * input for "mail", and exec with the user list we generated
		 * far above.
		 */
		if ((pid = fork()) == -1) {
			perror("fork");
			mail_exit(MAIL_STATUS_ERR_GENERAL);
		}
		if (pid == 0) {
			sigemptyset(&nset);
			sigaddset(&nset, SIGHUP);
			sigaddset(&nset, SIGINT);
			sigaddset(&nset, SIGQUIT);
			sigaddset(&nset, SIGTSTP);
			sigaddset(&nset, SIGTTIN);
			sigaddset(&nset, SIGTTOU);
			freopen("/dev/null", "r", stdin);
			if (smtp != NULL) {
				prepare_child(&nset, 0, 1);
				if (infix_and_transfer(smtp, to, mtf, hp,
						user, password, skinned) != 0){
					mail_exit(MAIL_STATUS_ERR_GENERAL);
				} else {
					mail_exit(MAIL_STATUS_SUCCESS);
				}
			}
		}

		return OKAY;
	}

	mail_exit(MAIL_STATUS_ERR_EMAIL);
}

/*
 * Create a Message-Id: header field.
 * Use either the host name or the from address.
 */
static void
message_id(FILE *fo, struct header *hp)
{
	char	*cp;
	time_t	now;

	time(&now);
	if ((cp = value("hostname")) != NULL)
		fprintf(fo, "Message-ID: <%lx.%s@%s>\n",
				(long)now, getrandstring(24), cp);
	else if ((cp = skin(myorigin(hp))) != NULL && strchr(cp, '@') != NULL)
		fprintf(fo, "Message-ID: <%lx.%s%%%s>\n",
				(long)now, getrandstring(16), cp);
}

static const char *weekday_names[] = {
	"Sun", "Mon", "Tue", "Wed", "Thu", "Fri", "Sat"
};

const char *month_names[] = {
	"Jan", "Feb", "Mar", "Apr", "May", "Jun",
	"Jul", "Aug", "Sep", "Oct", "Nov", "Dec", NULL
};

/*
 * Create a Date: header field.
 * We compare the localtime() and gmtime() results to get the timezone,
 * because numeric timezones are easier to read and because $TZ is
 * not set on most GNU systems.
 */
int
mkdate(FILE *fo, const char *field)
{
	time_t t;
	struct tm *tmptr;
	int tzdiff, tzdiff_hour, tzdiff_min;

	time(&t);
	tzdiff = t - mktime(gmtime(&t));
	tzdiff_hour = (int)(tzdiff / 60);
	tzdiff_min = tzdiff_hour % 60;
	tzdiff_hour /= 60;
	tmptr = localtime(&t);
	if (tmptr->tm_isdst > 0)
		tzdiff_hour++;
	return fprintf(fo, "%s: %s, %02d %s %04d %02d:%02d:%02d %+05d\n",
			field,
			weekday_names[tmptr->tm_wday],
			tmptr->tm_mday, month_names[tmptr->tm_mon],
			tmptr->tm_year + 1900, tmptr->tm_hour,
			tmptr->tm_min, tmptr->tm_sec,
			tzdiff_hour * 100 + tzdiff_min);
}

static enum okay
putname(char *line, enum gfield w, enum sendaction action, int *gotcha,
		char *prefix, FILE *fo, struct name **xp)
{
	struct name	*np;

	np = sextract(line, GEXTRA|GFULL);
	if (xp)
		*xp = np;
	if (np == NULL)
		return 0;
	if (fmt(prefix, np, fo, w&(GCOMMA|GFILES), 0, action != SEND_TODISP))
		return 1;
	if (gotcha)
		(*gotcha)++;
	return 0;
}

#define	FMT_CC_AND_BCC	{ \
				if (hp->h_cc != NULL && w & GCC) { \
					if (fmt("Cc:", hp->h_cc, fo, \
							w&(GCOMMA|GFILES), 0, \
							action!=SEND_TODISP)) \
						return 1; \
					gotcha++; \
				} \
				if (hp->h_bcc != NULL && w & GBCC) { \
					if (fmt("Bcc:", hp->h_bcc, fo, \
							w&(GCOMMA|GFILES), 0, \
							action!=SEND_TODISP)) \
						return 1; \
					gotcha++; \
				} \
			}
/*
 * Dump the to, subject, cc header on the
 * passed file buffer.
 */
int
puthead(struct header *hp, FILE *fo, enum gfield w,
		enum sendaction action, enum conversion convert,
		char *contenttype, char *charset)
{
	int gotcha;
	char *addr/*, *cp*/;
	int stealthmua;
	struct name *np, *fromfield = NULL, *senderfield = NULL;


	if (value("stealthmua"))
		stealthmua = 1;
	else
		stealthmua = 0;
	gotcha = 0;
	if (w & GDATE) {
		mkdate(fo, "Date"), gotcha++;
	}
	if (w & GIDENT) {
		if (hp->h_from != NULL) {
			if (fmt("From:", hp->h_from, fo, w&(GCOMMA|GFILES), 0,
						action!=SEND_TODISP))
				return 1;
			gotcha++;
			fromfield = hp->h_from;
		} else if ((addr = myaddrs(hp)) != NULL)
			if (putname(addr, w, action, &gotcha, "From:", fo,
						&fromfield))
				return 1;
		if (((addr = hp->h_organization) != NULL ||
				(addr = value("ORGANIZATION")) != NULL)
				&& strlen(addr) > 0) {
			fwrite("Organization: ", sizeof (char), 14, fo);
			if (mime_write(addr, strlen(addr), fo,
					action == SEND_TODISP ?
					CONV_NONE:CONV_TOHDR,
					action == SEND_TODISP ?
					TD_ISPR|TD_ICONV:TD_ICONV,
					NULL, (size_t)0,
					NULL, NULL) == 0)
				return 1;
			gotcha++;
			putc('\n', fo);
		}
		if (hp->h_replyto != NULL) {
			if (fmt("Reply-To:", hp->h_replyto, fo,
					w&(GCOMMA|GFILES), 0,
					action!=SEND_TODISP))
				return 1;
			gotcha++;
		} else if ((addr = value("replyto")) != NULL)
			if (putname(addr, w, action, &gotcha, "Reply-To:", fo,
						NULL))
				return 1;
		if (hp->h_sender != NULL) {
			if (fmt("Sender:", hp->h_sender, fo,
					w&(GCOMMA|GFILES), 0,
					action!=SEND_TODISP))
				return 1;
			gotcha++;
			senderfield = hp->h_sender;
		} else if ((addr = value("sender")) != NULL)
			if (putname(addr, w, action, &gotcha, "Sender:", fo,
						&senderfield))
				return 1;
		if (check_from_and_sender(fromfield, senderfield))
			return 1;
	}
	if (hp->h_to != NULL && w & GTO) {
		if (fmt("To:", hp->h_to, fo, w&(GCOMMA|GFILES), 0,
					action!=SEND_TODISP))
			return 1;
		gotcha++;
	}
	if (value("bsdcompat") == NULL && value("bsdorder") == NULL)
		FMT_CC_AND_BCC
	if (hp->h_subject != NULL && w & GSUBJECT) {
		fwrite("Subject: ", sizeof (char), 9, fo);
		if (ascncasecmp(hp->h_subject, "re: ", 4) == 0) {
			fwrite("Re: ", sizeof (char), 4, fo);
			if (strlen(hp->h_subject + 4) > 0 &&
				mime_write(hp->h_subject + 4,
					strlen(hp->h_subject + 4),
					fo, action == SEND_TODISP ?
					CONV_NONE:CONV_TOHDR,
					action == SEND_TODISP ?
					TD_ISPR|TD_ICONV:TD_ICONV,
					NULL, (size_t)0,
					NULL, NULL) == 0)
				return 1;
		} else if (*hp->h_subject) {
			if (mime_write(hp->h_subject,
					strlen(hp->h_subject),
					fo, action == SEND_TODISP ?
					CONV_NONE:CONV_TOHDR,
					action == SEND_TODISP ?
					TD_ISPR|TD_ICONV:TD_ICONV,
					NULL, (size_t)0,
					NULL, NULL) == 0)
				return 1;
		}
		gotcha++;
		fwrite("\n", sizeof (char), 1, fo);
	}
	if (value("bsdcompat") || value("bsdorder"))
		FMT_CC_AND_BCC
	if (w & GMSGID && stealthmua == 0)
		message_id(fo, hp), gotcha++;
	if (hp->h_ref != NULL && w & GREF) {
		fmt("References:", hp->h_ref, fo, 0, 1, 0);
		if ((np = hp->h_ref) != NULL && np->n_name) {
			while (np->n_flink)
				np = np->n_flink;
			if (mime_name_invalid(np->n_name, 0) == 0) {
				fprintf(fo, "In-Reply-To: %s\n", np->n_name);
				gotcha++;
			}
		}
	}
	if (w & GUA && stealthmua == 0)
		fprintf(fo, "User-Agent: Heirloom mailx %s\n",
				version), gotcha++;
	if (w & GMIME) {
		fputs("MIME-Version: 1.0\n", fo), gotcha++;
		if (hp->h_attach != NULL) {
			makeboundary();
			fprintf(fo, "Content-Type: multipart/mixed;\n"
				" boundary=\"%s\"\n", send_boundary);
		} else {
			fprintf(fo, "Content-Type: %s", contenttype);
			if (charset)
				fprintf(fo, "; charset=%s", charset);
			fprintf(fo, "\nContent-Transfer-Encoding: %s\n",
					getencoding(convert));
		}
	}
	if (gotcha && w & GNL)
		putc('\n', fo);
	return(0);
}


char *get_send_boundary(void)
{
	return send_boundary;
}

/*
 * Format the given header line to not exceed 72 characters.
 */
static int
fmt(char *str, struct name *np, FILE *fo, int flags, int dropinvalid,
		int domime)
{
	int col, len, count = 0;
	int is_to = 0, comma;

	comma = flags&GCOMMA ? 1 : 0;
	col = strlen(str);
	if (col) {
		fwrite(str, sizeof *str, strlen(str), fo);
		if ((flags&GFILES) == 0 &&
				col == 3 && asccasecmp(str, "to:") == 0 ||
				col == 3 && asccasecmp(str, "cc:") == 0 ||
				col == 4 && asccasecmp(str, "bcc:") == 0 ||
				col == 10 && asccasecmp(str, "Resent-To:") == 0)
			is_to = 1;
	}
	for (; np != NULL; np = np->n_flink) {
		if (is_to && is_fileaddr(np->n_name))
			continue;
		if (np->n_flink == NULL)
			comma = 0;
		if (mime_name_invalid(np->n_name, !dropinvalid)) {
			if (dropinvalid)
				continue;
			else
				return 1;
		}
		len = strlen(np->n_fullname);
		col++;		/* for the space */
		if (count && col + len + comma > 72 && col > 1) {
			fputs("\n ", fo);
			col = 1;
		} else
			putc(' ', fo);
		len = mime_write(np->n_fullname,
				len, fo,
				domime?CONV_TOHDR_A:CONV_NONE,
				TD_ICONV, NULL, (size_t)0,
				NULL, NULL);
		if (comma && !(is_to && is_fileaddr(np->n_flink->n_name)))
			putc(',', fo);
		col += len + comma;
		count++;
	}
	putc('\n', fo);
	return 0;
}
