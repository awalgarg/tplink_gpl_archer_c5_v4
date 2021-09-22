/*
 * Heirloom mailx - a mail user agent derived from Berkeley Mail.
 *
 * Copyright (c) 2000-2004 Gunnar Ritter, Freiburg i. Br., Germany.
 */
/*
 * Copyright (c) 2000
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
static char sccsid[] = "@(#)smtp.c	2.43 (gritter) 8/4/07";
#endif
#endif /* not lint */

#include "rcv.h"

#include <sys/utsname.h>
#include <sys/socket.h>
#include <netdb.h>
#include <netinet/in.h>
#ifdef	HAVE_ARPA_INET_H
#include <arpa/inet.h>
#endif	/* HAVE_ARPA_INET_H */
#include <unistd.h>
#include <setjmp.h>

#include "extern.h"
#include "md5.h"

#include <mail.h>

/*
 * Do not change, you get incorrect base64 encodings else!
 */
#ifndef INFIX_BUF
#define	INFIX_BUF	972
#endif

/*
 * Mail -- a mail program
 *
 * SMTP client and other internet related functions.
 */
static int verbose;
static int _debug;

static int write_temp_file_to_socket(struct sock *sp, char *tempMail, FILE *nfo);
//static void smtp_data(char *tempMail);

/*
 * Return our hostname.
 */
char *
nodename(int mayoverride)
{
	static char *hostname;
	char *hn;
    struct utsname ut;
	struct addrinfo hints, *res;

	if (mayoverride && (hn = value("hostname")) != NULL && *hn) {
		free(hostname);
		hostname = sstrdup(hn);
	}
	if (hostname == NULL) {
		uname(&ut);
		hn = ut.nodename;
		memset(&hints, 0, sizeof hints);
		hints.ai_socktype = SOCK_DGRAM;	/* dummy */
		hints.ai_flags = AI_CANONNAME;
		if (getaddrinfo(hn, "0", &hints, &res) == 0) {
			if (res->ai_canonname) {
				hn = salloc(strlen(res->ai_canonname) + 1);
				strcpy(hn, res->ai_canonname);
			}
			freeaddrinfo(res);
		}
		hostname = smalloc(strlen(hn) + 1);
		strcpy(hostname, hn);
	}
	//return hostname;
	return "Archer_VR200v";
}

/*
 * Return the user's From: address(es).
 */
char *
myaddrs(struct header *hp)
{
	char *cp, *hn;
	static char *addr;
	size_t sz;

	if (hp != NULL && hp->h_from != NULL) {
		if (hp->h_from->n_fullname)
			return savestr(hp->h_from->n_fullname);
		if (hp->h_from->n_name)
			return savestr(hp->h_from->n_name);
	}
	if ((cp = value("from")) != NULL)
		return cp;
	/*
	 * When invoking sendmail directly, it's its task
	 * to generate a From: address.
	 */
	if (value("smtp") == NULL)
		return NULL;
	if (addr == NULL) {
		hn = nodename(1);
		sz = strlen(myname) + strlen(hn) + 2;
		addr = smalloc(sz);
		snprintf(addr, sz, "%s@%s", myname, hn);
	}
	return addr;
}

char *
myorigin(struct header *hp)
{
	char	*cp;
	struct name	*np;

	if ((cp = myaddrs(hp)) == NULL ||
			(np = sextract(cp, GEXTRA|GFULL)) == NULL)
		return NULL;
	return np->n_flink != NULL ? value("sender") : cp;
}

static int read_smtp(struct sock *sp, int value, int ign_eof);
static int talk_smtp(struct name *to, FILE *fi, struct sock *sp,
		char *server, char *uhp, struct header *hp,
		const char *user, const char *password, const char *skinned);

char *
smtp_auth_var(const char *type, const char *addr)
{
	char	*var, *cp;
	int	len;

	var = ac_alloc(len = strlen(type) + strlen(addr) + 7);
	snprintf(var, len, "smtp-auth%s-%s", type, addr);
	if ((cp = value(var)) != NULL){
		cp = savestr(cp);
	}
	else {
		snprintf(var, len, "smtp-auth%s", type);
		if ((cp = value(var)) != NULL)
			cp = savestr(cp);
	}
	ac_free(var);
	return cp;
}

static char	*smtpbuf;
static size_t	smtpbufsize;

/*
 * Get the SMTP server's answer, expecting value.
 */
static int 
read_smtp(struct sock *sp, int value, int ign_eof)
{
	int ret;
	int len;

	do {
		if ((len = sgetline(&smtpbuf, &smtpbufsize, NULL, sp)) < 6) {
			if (len >= 0 && !ign_eof)
				DEBUG_ERROR( catgets(catd, CATSET, 241,
					"Unexpected EOF on SMTP connection\n"));
			return -1;
		}
		if (verbose || debug || _debug)
			DEBUG_ERROR(smtpbuf);
		switch (*smtpbuf) {
		case '1': ret = 1; break;
		case '2': ret = 2; break;
		case '3': ret = 3; break;
		case '4': ret = 4; break;
		default: ret = 5;
		}
		if (value != ret)
			DEBUG_ERROR( catgets(catd, CATSET, 191,
					"smtp-server: %s"), smtpbuf);
	} while (smtpbuf[3] == '-');
	return ret;
}

/*
 * Macros for talk_smtp.
 */
#define	_SMTP_ANSWER(x, ign_eof, err)	\
			if (!debug && !_debug) { \
				int	y; \
				if ((y = read_smtp(sp, x, ign_eof)) != (x) && \
					(!(ign_eof) || y != -1)) { \
					if (b != NULL) \
						free(b); \
					if (err == 0) {\
						mail_exit(MAIL_STATUS_ERR_RES);\
					} else {\
						mail_exit(err);\
					}\
				} \
			}

#define	SMTP_ANSWER(x, err)	_SMTP_ANSWER(x, 0, err)

#define	SMTP_OUT(x)	if (verbose || debug || _debug) \
				DEBUG_ERROR(">>> %s", x); \
			if (!debug && !_debug) \
				swrite(sp, x);

static int write_temp_file_to_socket(struct sock *sp, char *tempMail, FILE *nfo)
{
	char buff[LINESIZE] = {0};
	char *b = buff;
	size_t blen;
	size_t bsize = 0;
	size_t count;
	FILE *nfi;
	
	fflush(nfo);
	if (ferror(nfo)) {
		perror(catgets(catd, CATSET, 180, "temporary mail file"));
		fclose(nfo);
		fclose(nfi);
		return 1;
	}

	if ((nfi = fopen(tempMail, "r")) == NULL) {
		perror(tempMail);
		fclose(nfo);
		return 1;
	}

	count = fsize(nfi);
	while (1) {
		memset(b, 0, LINESIZE);
		if (NULL == fgets(b, LINESIZE, nfi)){
			break;
		}
		blen = strlen(b);

		if (*b == '.') {
			swrite1(sp, ".", 1, 1);
		}
		b[blen-1] = '\r';
		b[blen] = '\n';
		
		swrite1(sp, b, blen+1, 1);
	}

	return 0;
}

/*
 * Talk to a SMTP server.
 */
static int
talk_smtp(struct name *to, FILE *fi, struct sock *sp,
		char *xserver, char *uhp, struct header *hp,
		const char *user, const char *password, const char *skinned)
{
	struct name *n;
	char *b = NULL, o[LINESIZE];
	size_t blen, bsize = 0, count;
	char	*b64, *authstr, *cp;
	enum	{ AUTH_NONE, AUTH_PLAIN, AUTH_LOGIN, AUTH_CRAM_MD5 } auth;
	char *tempMail;
	int	inhdr = 1, inbcc = 0;
	int ret;
	
	if ((authstr = smtp_auth_var("", skinned)) == NULL)
		auth = user && password ? AUTH_LOGIN : AUTH_NONE;
	else if (strcmp(authstr, "plain") == 0)
		auth = AUTH_PLAIN;
	else if (strcmp(authstr, "login") == 0)
		auth = AUTH_LOGIN;
	else if (strcmp(authstr, "cram-md5") == 0)
		auth = AUTH_CRAM_MD5;
	else {
		DEBUG_ERROR("Unknown SMTP authentication "
				"method: \"%s\"\n", authstr);
		mail_exit(MAIL_STATUS_ERR_GENERAL);
	}
	if (auth != AUTH_NONE && (user == NULL || password == NULL)) {
		DEBUG_ERROR("User and password are necessary "
				"for SMTP authentication.\n");
		mail_exit(MAIL_STATUS_ERR_USER);
	}
	SMTP_ANSWER(2, MAIL_STATUS_ERR_IO);
	/* v11.0 compatibility */
	if (value("smtp-use-starttls") ||
			value("smtp-use-tls")) {
		char	*server;
		if ((cp = strchr(xserver, ':')) != NULL) {
			server = salloc(cp - xserver + 1);
			memcpy(server, xserver, cp - xserver);
			server[cp - xserver] = '\0';
		} else
			server = xserver;
		snprintf(o, sizeof o, "EHLO %s\r\n", nodename(1));
		SMTP_OUT(o);
		SMTP_ANSWER(2, 0);
		SMTP_OUT("STARTTLS\r\n");
		SMTP_ANSWER(2, 0);
		if (!debug && !_debug && ssl_open(server, sp, uhp) != OKAY) {
			mail_exit(MAIL_STATUS_ERR_STARTTLS);
		}
	}
	if (auth != AUTH_NONE) {
		snprintf(o, sizeof o, "EHLO %s\r\n", nodename(1));
		SMTP_OUT(o);
		SMTP_ANSWER(2, 0);
		switch (auth) {
		default:
		case AUTH_LOGIN:
			SMTP_OUT("AUTH LOGIN\r\n");
			SMTP_ANSWER(3, 0);
			b64 = strtob64(user);
			snprintf(o, sizeof o, "%s\r\n", b64);
			free(b64);
			SMTP_OUT(o);
			SMTP_ANSWER(3, 0);
			b64 = strtob64(password);
			snprintf(o, sizeof o, "%s\r\n", b64);
			free(b64);
			SMTP_OUT(o);
			SMTP_ANSWER(2, MAIL_STATUS_ERR_USER);
			break;
		case AUTH_PLAIN:
			SMTP_OUT("AUTH PLAIN\r\n");
			SMTP_ANSWER(3, 0);
			snprintf(o, sizeof o, "%c%s%c%s", '\0', user, '\0',
							  password);
			b64 = memtob64(o, strlen(user)+strlen(password)+2);
			snprintf(o, sizeof o, "%s\r\n", b64);
			SMTP_OUT(o);
			SMTP_ANSWER(2, MAIL_STATUS_ERR_USER);
			break;
		case AUTH_CRAM_MD5:
			SMTP_OUT("AUTH CRAM-MD5\r\n");
			SMTP_ANSWER(3, 0);
			for (cp = smtpbuf; digitchar(*cp&0377); cp++);
			while (blankchar(*cp&0377)) cp++;
			cp = cram_md5_string(user, password, cp);
			SMTP_OUT(cp);
			SMTP_ANSWER(2, MAIL_STATUS_ERR_USER);
			break;
		}
	} else {
		snprintf(o, sizeof o, "HELO %s\r\n", nodename(1));
		SMTP_OUT(o);
		SMTP_ANSWER(2, 0);
	}
	snprintf(o, sizeof o, "MAIL FROM:<%s>\r\n", skinned);
	SMTP_OUT(o);
	SMTP_ANSWER(2, MAIL_STATUS_ERR_EMAIL);
	for (n = to; n != NULL; n = n->n_flink) {
		if ((n->n_type & GDEL) == 0) {
			snprintf(o, sizeof o, "RCPT TO:<%s>\r\n",
					skin(n->n_name));
			SMTP_OUT(o);
			SMTP_ANSWER(2, MAIL_STATUS_ERR_EMAIL);
		}
	}

	if (0 == strcmp(value("mail-type"), "check-connection")){
		if (b != NULL)
		free(b);

		SMTP_OUT("QUIT\r\n");
		_SMTP_ANSWER(2, 1, 0);
		
		rm(tempMail);
		return 0;
	}
	
	SMTP_OUT("DATA\r\n");
	SMTP_ANSWER(3, 0);

	ret = smtp_data(tempMail, hp, fi, sp);
	if (ret != 0)
		return ret;
	
	SMTP_OUT(".\r\n");
	SMTP_ANSWER(2, 0);
	SMTP_OUT("QUIT\r\n");
	_SMTP_ANSWER(2, 1, 0);
	if (b != NULL)
		free(b);

	return 0;
}

int smtp_data(char *tempMail, struct header *hp, FILE *fi, struct sock *sp)
{
	FILE *nfo, *nfi;
	enum mimeclean isclean;
	enum conversion convert;
	char *charset = NULL;
	char *contenttype = NULL;
	int	lastc = EOF;
	size_t count;
	
	if ((nfo = Ftemp(&tempMail, "Rs", "w", 0600, 1)) == NULL) {
		perror(catgets(catd, CATSET, 178, "temporary mail file"));
		return 1;
	}
	

	convert = get_mime_convert(fi, &contenttype, &charset,
			&isclean);
	contenttype = "text/html";

	if (puthead(hp, nfo,
		   GTO|GSUBJECT|GCC|GBCC|GNL|GCOMMA|GUA|GMIME
		   |GMSGID|GIDENT|GREF|GDATE,
		   SEND_MBOX, convert, contenttype, charset)) {
		fclose(nfo);
		unlink(tempMail);
		return 1;
	}

	char *send_boundary = get_send_boundary();

	if (hp->h_attach != NULL) {
		//=================================================================//
		struct attachment *att;

		fputs("This is a multi-part message in MIME format.\n", nfo);
		if (fsize(fi) != 0) {
			char *buf, c = '\n';
			size_t sz, bufsize, count;

			fprintf(nfo, "\n--%s\n", send_boundary);
			fprintf(nfo, "Content-Type: %s", contenttype);
			if (charset)
				fprintf(nfo, "; charset=%s", charset);
			fprintf(nfo, "\nContent-Transfer-Encoding: %s\n"
					"Content-Disposition: inline\n\n",
					getencoding(convert));
			buf = smalloc(bufsize = INFIX_BUF);
			for (;;) {
				sz = fread(buf, sizeof *buf, bufsize, fi);
				if (sz == 0)
					break;
				
				c = buf[sz - 1];
				if (mime_write(buf, sz, nfo, convert,
						TD_ICONV, NULL, (size_t)0,
						NULL, NULL) == 0) {
					free(buf);
					fclose(nfo);
					unlink(tempMail);
					return 1;
				}
			}
			free(buf);
			if (ferror(fi))
			{
				fclose(nfo);
				unlink(tempMail);
				return 1;
			}
			if (c != '\n')
				putc('\n', nfo);
			if (charset != NULL)
				put_signature(nfo, convert);
		}

		fflush(nfo);

		if (write_temp_file_to_socket(sp, tempMail, nfo) != 0) {
			fclose(nfo);
			unlink(tempMail);
			return 1;
		}

		
		for (att = hp->h_attach; att != NULL; att = att->a_flink) {

			fclose(nfo);
			nfo = fopen(tempMail, "w");

			char *basename;
			char *contenttype = NULL;
			char *charset = NULL;
			enum conversion convert = CONV_TOB64;
			enum mimeclean isclean;
			
			if ((basename = strrchr(att->a_name, '/')) == NULL)
				basename = att->a_name;
			else
				basename++;

			if (att->a_content_type)
				contenttype = att->a_content_type;
			else
				contenttype = mime_filecontent(basename);

			if (att->a_charset)
				charset = att->a_charset;
			
			fprintf(nfo,
				"\n--%s\n"
				"Content-Type: %s",
				send_boundary, contenttype);

			if (charset == NULL)
				putc('\n', nfo);
			else
				fprintf(nfo, ";\n charset=%s\n", charset);


			if ((fi = fopen(att->a_name, "r")) == NULL) {
				perror(att->a_name);
				fclose(nfo);
				unlink(tempMail);
				return -1;
			}
			convert = get_mime_convert(fi, &contenttype, &charset, &isclean);
			
			if (att->a_content_disposition == NULL)
				att->a_content_disposition = "attachment";
			fprintf(nfo, "Content-Transfer-Encoding: %s\n"
				"Content-Disposition: %s;\n"
				" filename=\"",
				getencoding(convert),
				att->a_content_disposition);
		
			mime_write(basename, strlen(basename), nfo,
					CONV_TOHDR, TD_NONE, NULL, (size_t)0, NULL, NULL);
			fwrite("\"\n", sizeof (char), 2, nfo);
		
			if (att->a_content_id)
				fprintf(nfo, "Content-ID: %s\n", att->a_content_id);
		
			if (att->a_content_description)
				fprintf(nfo, "Content-Description: %s\n",
						att->a_content_description);
		
			putc('\n', nfo);
			if (write_temp_file_to_socket(sp, tempMail, nfo) != 0) {
				fclose(nfo);
				unlink(tempMail);
				return 1;
			}


			while (1){
				fclose(nfo);
				nfo = fopen(tempMail, "w");
				int status = attach_file(att, convert, isclean, fi, nfo);
				if (status < 0) {
					fclose(nfo);
					unlink(tempMail);
					return 1;
				}
				if (write_temp_file_to_socket(sp, tempMail, nfo) != 0) {
					fclose(nfo);
					unlink(tempMail);
					return 1;
				}
				if (status > 0)
					continue;
				else {
					break;
				}
			}
		}

		fclose(nfo);
		nfo = fopen(tempMail, "w");
		/* the final boundary with two attached dashes */
		fprintf(nfo, "\n--%s--\n", send_boundary);
		if (write_temp_file_to_socket(sp, tempMail, nfo) != 0){
			fclose(nfo);
			unlink(tempMail);
			return 1;
		}

		fclose(fi);
		fclose(nfo);

		unlink(tempMail);
		return 0;
		//=================================================================//
	} else {
		size_t sz, bufsize, count;
		char *buf;
		buf = smalloc(bufsize = INFIX_BUF);
		for (;;) {
			sz = fread(buf, sizeof *buf, bufsize, fi);
			if (sz == 0)
				break;
		
			lastc = buf[sz - 1];
			if (mime_write(buf, sz, nfo, convert,
					TD_ICONV, NULL, (size_t)0,
					NULL, NULL) == 0) {
				fclose(nfo);
				free(buf);
				unlink(tempMail);
				return 1;
			}
		}
		free(buf);
		if (ferror(fi)) {
			fclose(nfo);
			unlink(tempMail);
			return 1;
		}
		if (charset != NULL)
			put_signature(nfo, convert);

		if (write_temp_file_to_socket(sp, tempMail, nfo) != 0) {
			fclose(nfo);
			unlink(tempMail);
			return 1;
		}

		fclose(nfo);
		fclose(fi);

		unlink(tempMail);
		return 0;
	}
}

static sigjmp_buf	smtpjmp;

static void
onterm(int signo)
{
	siglongjmp(smtpjmp, 1);
}


int infix_and_transfer(char *server, struct name *to, FILE *fi, struct header *hp,
		const char *user, const char *password, const char *skinned)
{
	struct sock	so;
	int	use_ssl, ret;
	sighandler_type	saveterm;
	int use_starttls = 0;
	memset(&so, 0, sizeof so);
	
	saveterm = safe_signal(SIGTERM, SIG_IGN);
	if (sigsetjmp(smtpjmp, 1)) {
		safe_signal(SIGTERM, saveterm);
		mail_exit(MAIL_STATUS_ERR_GENERAL);
	}
	if (saveterm != SIG_IGN)
		safe_signal(SIGTERM, onterm);

	if ((NULL == value("smtp-use-ssl")) &&
		(NULL == value("smtp-use-starttls"))){
		if (sopen(server, &so, 0, server, "smtp") != OKAY) {
			safe_signal(SIGTERM, saveterm);
			mail_exit(MAIL_STATUS_ERR_CONNECT);
		}
		use_starttls = 0;
	}

	if (NULL != value("smtp-use-ssl")){
		if (sopen(server, &so, 1, server, "smtps") != OKAY) {
			safe_signal(SIGTERM, saveterm);
			mail_exit(MAIL_STATUS_ERR_SSL);
		}
		use_starttls = 0;
	}

	if (NULL != value("smtp-use-starttls")){
		if (sopen(server, &so, 0, server, "smtps") != OKAY) {
			safe_signal(SIGTERM, saveterm);
			mail_exit(MAIL_STATUS_ERR_STARTTLS);
		}
		use_starttls = 1;
	}
	
	so.s_desc = "SMTP";
	if (use_starttls)
		assign("smtp-use-starttls", "");
		
	ret = talk_smtp(to, fi, &so, server, server, hp,
			user, password, skinned);
	sclose(&so);
	if (smtpbuf) {
		free(smtpbuf);
		smtpbuf = NULL;
		smtpbufsize = 0;
	}
	safe_signal(SIGTERM, saveterm);
	return ret;
}
