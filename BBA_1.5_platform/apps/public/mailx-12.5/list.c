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
static char sccsid[] = "@(#)list.c	2.62 (gritter) 12/11/08";
#endif
#endif /* not lint */

#include "rcv.h"
#include <ctype.h>
#include "extern.h"
#ifdef	HAVE_WCTYPE_H
#include <wctype.h>
#endif	/* HAVE_WCTYPE_H */

/*
 * Mail -- a mail program
 *
 * Message list handling.
 */

enum idfield {
	ID_REFERENCES,
	ID_IN_REPLY_TO
};
 
static size_t	STRINGLEN;

static int	lexnumber;		/* Number of TNUMBER from scan() */
static char	*lexstring;		/* String from TSTRING, scan() */
static int	regretp;		/* Pointer to TOS of regret tokens */
static int	regretstack[REGDEP];	/* Stack of regretted tokens */
static char	*string_stack[REGDEP];	/* Stack of regretted strings */
static int	numberstack[REGDEP];	/* Stack of regretted numbers */
static int	threadflag;		/* mark entire threads */

/*
 * Mark all messages that the user wanted from the command
 * line in the message structure.  Return 0 on success, -1
 * on error.
 */

/*
 * Bit values for colon modifiers.
 */

#define	CMNEW		01		/* New messages */
#define	CMOLD		02		/* Old messages */
#define	CMUNREAD	04		/* Unread messages */
#define	CMDELETED	010		/* Deleted messages */
#define	CMREAD		020		/* Read messages */
#define	CMFLAG		040		/* Flagged messages */
#define	CMANSWER	0100		/* Answered messages */
#define	CMDRAFT		0200		/* Draft messages */
#define	CMKILL		0400		/* Killed messages */
#define	CMJUNK		01000		/* Junk messages */

/*
 * The following table describes the letters which can follow
 * the colon and gives the corresponding modifier bit.
 */

static struct coltab {
	char	co_char;		/* What to find past : */
	int	co_bit;			/* Associated modifier bit */
	int	co_mask;		/* m_status bits to mask */
	int	co_equal;		/* ... must equal this */
} coltab[] = {
	{ 'n',		CMNEW,		MNEW,		MNEW },
	{ 'o',		CMOLD,		MNEW,		0 },
	{ 'u',		CMUNREAD,	MREAD,		0 },
	{ 'd',		CMDELETED,	MDELETED,	MDELETED },
	{ 'r',		CMREAD,		MREAD,		MREAD },
	{ 'f',		CMFLAG,		MFLAGGED,	MFLAGGED },
	{ 'a',		CMANSWER,	MANSWERED,	MANSWERED },
	{ 't',		CMDRAFT,	MDRAFTED,	MDRAFTED },
	{ 'k',		CMKILL,		MKILL,		MKILL },
	{ 'j',		CMJUNK,		MJUNK,		MJUNK },
	{ 0,		0,		0,		0 }
};

static	int	lastcolmod;
#define	markall_ret(i)		{ \
					retval = i; \
					ac_free(lexstring); \
					goto out; \
				} 
/*
 * Scan out the list of string arguments, shell style
 * for a RAWLIST.
 */
int
getrawlist(const char *line, size_t linesize, char **argv, int argc,
		int echolist)
{
	char c, *cp2, quotec;
	const char	*cp;
	int argn;
	char *linebuf;

	argn = 0;
	cp = line;
	linebuf = ac_alloc(linesize + 1);
	for (;;) {
		for (; blankchar(*cp & 0377); cp++);
		if (*cp == '\0')
			break;
		if (argn >= argc - 1) {
			DEBUG_PRINT(catgets(catd, CATSET, 126,
			"Too many elements in the list; excess discarded.\n"));
			break;
		}
		cp2 = linebuf;
		quotec = '\0';
		while ((c = *cp) != '\0') {
			cp++;
			if (quotec != '\0') {
				if (c == quotec) {
					quotec = '\0';
					if (echolist)
						*cp2++ = c;
				} else if (c == '\\')
					switch (c = *cp++) {
					case '\0':
						*cp2++ = '\\';
						cp--;
						break;
					default:
						if (cp[-1]!=quotec || echolist)
							*cp2++ = '\\';
						*cp2++ = c;
					}
					else
					*cp2++ = c;
			} else if (c == '"' || c == '\'') {
				if (echolist)
					*cp2++ = c;
				quotec = c;
			} else if (c == '\\' && !echolist) {
				if (*cp)
					*cp2++ = *cp++;
				else
					*cp2++ = c;
			} else if (blankchar(c & 0377))
				break;
			else
				*cp2++ = c;
		}
		*cp2 = '\0';		
		argv[argn++] = savestr(linebuf);
	}
	argv[argn] = NULL;
	ac_free(linebuf);
	return argn;
}

/*
 * scan out a single lexical item and return its token number,
 * updating the string pointer passed **p.  Also, store the value
 * of the number or string scanned in lexnumber or lexstring as
 * appropriate.  In any event, store the scanned `thing' in lexstring.
 */

static struct lex {
	char	l_char;
	enum ltoken	l_token;
} singles[] = {
	{ '$',	TDOLLAR },
	{ '.',	TDOT },
	{ '^',	TUP },
	{ '*',	TSTAR },
	{ '-',	TDASH },
	{ '+',	TPLUS },
	{ '(',	TOPEN },
	{ ')',	TCLOSE },
	{ ',',	TCOMMA },
	{ ';',	TSEMI },
	{ '`',	TBACK },
	{ 0,	0 }
}; 
/*
 * Find the first message whose flags & m == f  and return
 * its message number.
 */
int 
first(int f, int m)
{
	struct message *mp;

	if (msgCount == 0)
		return 0;
	f &= MDELETED;
	m &= MDELETED;
	for (mp = dot; mb.mb_threaded ? mp != NULL : mp < &message[msgCount];
			mb.mb_threaded ? mp = next_in_thread(mp) : mp++) {
		if (!(mp->m_flag & MHIDDEN) && (mp->m_flag & m) == f)
			return mp - message + 1;
	}
	if (dot > &message[0]) {
		for (mp = dot-1; mb.mb_threaded ?
					mp != NULL : mp >= &message[0];
				mb.mb_threaded ?
					mp = prev_in_thread(mp) : mp--) {
			if (!(mp->m_flag & MHIDDEN) && (mp->m_flag & m) == f)
				return mp - message + 1;
		}
	}
	return 0;
}

/*
 * See if the given string matches inside the subject field of the
 * given message.  For the purpose of the scan, we ignore case differences.
 * If it does, return true.  The string search argument is assumed to
 * have the form "/search-string."  If it is of the form "/," we use the
 * previous search string.
 */

static char lastscan[128];