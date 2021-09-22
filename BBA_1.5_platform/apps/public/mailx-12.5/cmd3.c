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
static char sccsid[] = "@(#)cmd3.c	2.87 (gritter) 10/1/08";
#endif
#endif /* not lint */

#include <math.h>
#include <float.h>
#include "rcv.h"
#include "extern.h"
#include <unistd.h>
#include <errno.h>

/*
 * Mail -- a mail program
 *
 * Still more user commands.
 */
static void asort(char **list);
static int diction(const void *a, const void *b);
/*
 * Expand the shell escape by expanding unescaped !'s into the
 * last issued command where possible.
 */

static char	*lastbang;
static size_t	lastbangsize;

static sigjmp_buf	pipejmp;

/*
 * Set or display a variable value.  Syntax is similar to that
 * of sh.
 */
int 
set(void *v)
{
	char **arglist = v;
	struct var *vp;
	char *cp, *cp2;
	char **ap, **p;
	int errs, h, s;
	FILE *obuf = stdout;
	int bsdset = value("bsdcompat") != NULL || value("bsdset") != NULL;

	(void)&cp;
	(void)&ap;
	(void)&obuf;
	(void)&bsdset;
	if (*arglist == NULL) {
		for (h = 0, s = 1; h < HSHSIZE; h++)
			for (vp = variables[h]; vp != NULL; vp = vp->v_link)
				s++;
		/*LINTED*/
		ap = (char **)salloc(s * sizeof *ap);
		for (h = 0, p = ap; h < HSHSIZE; h++)
			for (vp = variables[h]; vp != NULL; vp = vp->v_link)
				*p++ = vp->v_name;
		*p = NULL;
		asort(ap);
		for (p = ap; *p != NULL; p++) {
			if (bsdset)
				fprintf(obuf, "%s\t%s\n", *p, value(*p));
			else {
				if ((cp = value(*p)) != NULL && *cp)
					fprintf(obuf, "%s=\"%s\"\n",
							*p, value(*p));
				else
					fprintf(obuf, "%s\n", *p);
			}
		}
endpipe:
		if (obuf != stdout) {
			safe_signal(SIGPIPE, SIG_IGN);
			Pclose(obuf);
			safe_signal(SIGPIPE, dflpipe);
		}
		return(0);
	}

	errs = 0;
	for (ap = arglist; *ap != NULL; ap++) {
		char *varbuf;

		varbuf = ac_alloc(strlen(*ap) + 1);
		cp = *ap;
		cp2 = varbuf;
		while (*cp != '=' && *cp != '\0')
			*cp2++ = *cp++;
		*cp2 = '\0';
		if (*cp == '\0')
			cp = "";
		else
			cp++;
		if (equal(varbuf, "")) {
			DEBUG_PRINT(catgets(catd, CATSET, 41,
					"Non-null variable name required\n"));
			errs++;
			ac_free(varbuf);
			continue;
		}
		if (varbuf[0] == 'n' && varbuf[1] == 'o')
			errs += unset_internal(&varbuf[2]);
		else{
			assign(varbuf, cp);
		}
		ac_free(varbuf);
	}
	return(errs);
}

/*
 * Sort the passed string vecotor into ascending dictionary
 * order.
 */
static void 
asort(char **list)
{
	char **ap;

	for (ap = list; *ap != NULL; ap++)
		;
	if (ap-list < 2)
		return;
	qsort(list, ap-list, sizeof(*list), diction);
}

/*
 * Do a dictionary order comparison of the arguments from
 * qsort.
 */
static int 
diction(const void *a, const void *b)
{
	return(strcmp(*(char **)a, *(char **)b));
}

struct shortcut *
get_shortcut(const char *str)
{
	struct shortcut *s;

	for (s = shortcuts; s; s = s->sh_next)
		if (strcmp(str, s->sh_short) == 0)
			break;
	return s;
}
