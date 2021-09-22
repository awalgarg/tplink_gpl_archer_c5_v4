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
static char sccsid[] = "@(#)head.c	2.17 (gritter) 3/4/06";
#endif
#endif /* not lint */

#include "rcv.h"
#include "extern.h"
#include <time.h>

/*
 * Mail -- a mail program
 *
 * Routines for processing and detecting headlines.
 */

/*
 * Check whether the passed line is a header line of
 * the desired breed.  Return the field body, or 0.
 */
char *
thisfield(const char *linebuf, const char *field)
{
	while (lowerconv(*linebuf&0377) == lowerconv(*field&0377)) {
		linebuf++;
		field++;
	}
	if (*field != '\0')
		return NULL;
	while (blankchar(*linebuf&0377))
		linebuf++;
	if (*linebuf++ != ':')
		return NULL;
	while (blankchar(*linebuf&0377))
		linebuf++;
	return (char *)linebuf;
}

/*
 * Start of a "comment".
 * Ignore it.
 */
char *
skip_comment(const char *cp)
{
	int nesting = 1;

	for (; nesting > 0 && *cp; cp++) {
		switch (*cp) {
		case '\\':
			if (cp[1])
				cp++;
			break;
		case '(':
			nesting++;
			break;
		case ')':
			nesting--;
			break;
		}
	}
	return (char *)cp;
}

/*
 * Return the start of a route-addr (address in angle brackets),
 * if present.
 */
char *
routeaddr(const char *name)
{
	const char	*np, *rp = NULL;

	for (np = name; *np; np++) {
		switch (*np) {
		case '(':
			np = skip_comment(&np[1]) - 1;
			break;
		case '"':
			while (*np) {
				if (*++np == '"')
					break;
				if (*np == '\\' && np[1])
					np++;
			}
			break;
		case '<':
			rp = np;
			break;
		case '>':
			return (char *)rp;
		}
	}
	return NULL;
}

/*
 * Skin an arpa net address according to the RFC 822 interpretation
 * of "host-phrase."
 */
char *
skin(char *name)
{
	int c;
	char *cp, *cp2;
	char *bufend;
	int gotlt, lastsp;
	char *nbuf;

	if (name == NULL)
		return(NULL);
	if (strchr(name, '(') == NULL && strchr(name, '<') == NULL
	    && strchr(name, ' ') == NULL)
		return(name);
	gotlt = 0;
	lastsp = 0;
	nbuf = ac_alloc(strlen(name) + 1);
	bufend = nbuf;
	for (cp = name, cp2 = bufend; (c = *cp++) != '\0'; ) {
		switch (c) {
		case '(':
			cp = skip_comment(cp);
			lastsp = 0;
			break;

		case '"':
			/*
			 * Start of a "quoted-string".
			 * Copy it in its entirety.
			 */
			*cp2++ = c;
			while ((c = *cp) != '\0') {
				cp++;
				if (c == '"') {
					*cp2++ = c;
					break;
				}
				if (c != '\\')
					*cp2++ = c;
				else if ((c = *cp) != '\0') {
					*cp2++ = c;
					cp++;
				}
			}
			lastsp = 0;
			break;

		case ' ':
			if (cp[0] == 'a' && cp[1] == 't' && cp[2] == ' ')
				cp += 3, *cp2++ = '@';
			else
			if (cp[0] == '@' && cp[1] == ' ')
				cp += 2, *cp2++ = '@';
#if 0
			/*
			 * RFC 822 specifies spaces are STRIPPED when
			 * in an adress specifier.
			 */
			else
				lastsp = 1;
#endif
			break;

		case '<':
			cp2 = bufend;
			gotlt++;
			lastsp = 0;
			break;

		case '>':
			if (gotlt) {
				gotlt = 0;
				while ((c = *cp) != '\0' && c != ',') {
					cp++;
					if (c == '(')
						cp = skip_comment(cp);
					else if (c == '"')
						while ((c = *cp) != '\0') {
							cp++;
							if (c == '"')
								break;
							if (c == '\\' && *cp)
								cp++;
						}
				}
				lastsp = 0;
				break;
			}
			/* Fall into . . . */

		default:
			if (lastsp) {
				lastsp = 0;
				*cp2++ = ' ';
			}
			*cp2++ = c;
			if (c == ',' && !gotlt) {
				*cp2++ = ' ';
				for (; *cp == ' '; cp++)
					;
				lastsp = 0;
				bufend = cp2;
			}
		}
	}
	*cp2 = 0;
	cp = savestr(nbuf);
	ac_free(nbuf);
	return cp;
}

char *
nexttoken(char *cp)
{
	for (;;) {
		if (*cp == '\0')
			return NULL;
		if (*cp == '(') {
			int nesting = 0;

			while (*cp != '\0') {
				switch (*cp++) {
				case '(':
					nesting++;
					break;
				case ')':
					nesting--;
					break;
				}
				if (nesting <= 0)
					break;
			}
		} else if (blankchar(*cp & 0377) || *cp == ',')
			cp++;
		else
			break;
	}
	return cp;
}
#define	leapyear(year)	((year % 100 ? year : year / 100) % 4 == 0)

time_t
combinetime(int year, int month, int day, int hour, int minute, int second)
{
	time_t t;

	if (second < 0 || minute < 0 || hour < 0 || day < 1)
		return -1;
	t = second + minute * 60 + hour * 3600 + (day - 1) * 86400;
	if (year < 70)
		year += 2000;
	else if (year < 1900)
		year += 1900;
	if (month > 1)
		t += 86400 * 31;
	if (month > 2)
		t += 86400 * (leapyear(year) ? 29 : 28);
	if (month > 3)
		t += 86400 * 31;
	if (month > 4)
		t += 86400 * 30;
	if (month > 5)
		t += 86400 * 31;
	if (month > 6)
		t += 86400 * 30;
	if (month > 7)
		t += 86400 * 31;
	if (month > 8)
		t += 86400 * 31;
	if (month > 9)
		t += 86400 * 30;
	if (month > 10)
		t += 86400 * 31;
	if (month > 11)
		t += 86400 * 30;
	year -= 1900;
	t += (year - 70) * 31536000 + ((year - 69) / 4) * 86400 -
		((year - 1) / 100) * 86400 + ((year + 299) / 400) * 86400;
	return t;
}

int
check_from_and_sender(struct name *fromfield, struct name *senderfield)
{
	if (fromfield && fromfield->n_flink && senderfield == NULL) {
		DEBUG_ERROR("A Sender: field is required with multiple "
				"addresses in From: field.\n");
		return 1;
	}
	if (senderfield && senderfield->n_flink) {
		DEBUG_ERROR("The Sender: field may contain "
				"only one address.\n");
		return 2;
	}
	return 0;
}
