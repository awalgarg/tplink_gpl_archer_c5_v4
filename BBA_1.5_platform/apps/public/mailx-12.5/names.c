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
static char sccsid[] = "@(#)names.c	2.22 (gritter) 3/4/06";
#endif
#endif /* not lint */

/*
 * Mail -- a mail program
 *
 * Handle name lists.
 */

#include "rcv.h"
#include "extern.h"
#include <sys/stat.h>
#include <fcntl.h>
#include <time.h>
#include <unistd.h>

static struct name *tailof(struct name *name);
static struct name *extract1(char *line, enum gfield ntype, char *separators,
		int copypfx);
static char *yankword(char *ap, char *wbuf, char *separators, int copypfx);
static int same_name(char *n1, char *n2);
static struct name *gexpand(struct name *nlist, struct grouphead *gh,
		int metoo, int ntype);
static struct name *put(struct name *list, struct name *node);

/*
 * Allocate a single element of a name list,
 * initialize its name field to the passed
 * name and return it.
 */
struct name *
nalloc(char *str, enum gfield ntype)
{
	struct name *np;
	struct str	in, out;

	/*LINTED*/
	np = (struct name *)salloc(sizeof *np);
	np->n_flink = NULL;
	np->n_blink = NULL;
	np->n_type = ntype;
	if (ntype & GFULL) {
		np->n_name = savestr(skin(str));
		if (strcmp(np->n_name, str)) {
			in.s = str;
			in.l = strlen(str);
			mime_fromhdr(&in, &out, TD_ISPR|TD_ICONV);
			np->n_fullname = savestr(out.s);
			free(out.s);
		} else
			np->n_fullname = np->n_name;
	} else if (ntype & GSKIN)
		np->n_fullname = np->n_name = savestr(skin(str));
	else
		np->n_fullname = np->n_name = savestr(str);
	return(np);
}

/*
 * Find the tail of a list and return it.
 */
static struct name *
tailof(struct name *name)
{
	struct name *np;

	np = name;
	if (np == NULL)
		return(NULL);
	while (np->n_flink != NULL)
		np = np->n_flink;
	return(np);
}

/*
 * Extract a list of names from a line,
 * and make a list of names from it.
 * Return the list or NULL if none found.
 */
struct name *
extract(char *line, enum gfield ntype)
{
	return extract1(line, ntype, " \t,(", 0);
}

struct name *
sextract(char *line, enum gfield ntype)
{
	if (line && strpbrk(line, ",\"\\(<"))
		return extract1(line, ntype, ",", 1);
	else
		return extract(line, ntype);
}

static struct name *
extract1(char *line, enum gfield ntype, char *separators, int copypfx)
{
	char *cp, *nbuf;
	struct name *top, *np, *t;

	if (line == NULL || *line == '\0')
		return NULL;
	top = NULL;
	np = NULL;
	cp = line;
	nbuf = ac_alloc(strlen(line) + 1);
	while ((cp = yankword(cp, nbuf, separators, copypfx)) != NULL) {
		t = nalloc(nbuf, ntype);
		if (top == NULL)
			top = t;
		else
			np->n_flink = t;
		t->n_blink = np;
		np = t;
	}
	ac_free(nbuf);
	return top;
}

/*
 * Grab a single word (liberal word)
 * Throw away things between ()'s, and take anything between <>.
 */
static char *
yankword(char *ap, char *wbuf, char *separators, int copypfx)
{
	char *cp, *pp, *wp;

	cp = ap;
	wp = wbuf;
	while (blankchar(*cp & 0377) || *cp == ',')
		cp++;
	pp = cp;
	if ((cp = nexttoken(cp)) == NULL)
		return NULL;
	if (copypfx)
		while (pp < cp)
			*wp++ = *pp++;
	if (*cp ==  '<')
		while (*cp && (*wp++ = *cp++) != '>');
	else {
		int incomm = 0;

		while (*cp && (incomm || !strchr(separators, *cp))) {
			if (*cp == '\"') {
				if (cp == ap || *(cp - 1) != '\\') {
					if (incomm)
						incomm--;
					else
						incomm++;
					*wp++ = '\"';
				} else if (cp != ap) {
					*(wp - 1) = '\"';
				}
				cp++;
				continue;
			}
			*wp++ = *cp++;
		}
	}
	*wp = '\0';
	return cp;
}

/*
 * Determine if the passed address is a local "send to file" address.
 * If any of the network metacharacters precedes any slashes, it can't
 * be a filename.  We cheat with .'s to allow path names like ./...
 */
int 
is_fileaddr(char *name)
{
	char *cp;

	if (strchr(name, '@') != NULL)
		return 0;
	if (*name == '+')
		return 1;
	for (cp = name; *cp; cp++) {
		if (*cp == '!' || *cp == '%')
			return 0;
		if (*cp == '/')
			return 1;
	}
	return 0;
}

static int
same_name(char *n1, char *n2)
{
	int c1, c2;

	if (value("allnet") != NULL) {
		do {
			c1 = (*n1++ & 0377);
			c2 = (*n2++ & 0377);
			c1 = lowerconv(c1);
			c2 = lowerconv(c2);
			if (c1 != c2)
				return 0;
		} while (c1 != '\0' && c2 != '\0' && c1 != '@' && c2 != '@');
		return 1;
	} else
		return asccasecmp(n1, n2) == 0;
}

/*
 * Map all of the aliased users in the invoker's mailrc
 * file and insert them into the list.
 * Changed after all these months of service to recursively
 * expand names (2/14/80).
 */

struct name *
usermap(struct name *names)
{
	struct name *new, *np, *cp;
	struct grouphead *gh;
	int metoo;

	new = NULL;
	np = names;
	metoo = (value("metoo") != NULL);
	while (np != NULL) {
		if (np->n_name[0] == '\\') {
			cp = np->n_flink;
			new = put(new, np);
			np = cp;
			continue;
		}
		gh = findgroup(np->n_name);
		cp = np->n_flink;
		if (gh != NULL)
			new = gexpand(new, gh, metoo, np->n_type);
		else
			new = put(new, np);
		np = cp;
	}
	return(new);
}

/*
 * Recursively expand a group name.  We limit the expansion to some
 * fixed level to keep things from going haywire.
 * Direct recursion is not expanded for convenience.
 */

static struct name *
gexpand(struct name *nlist, struct grouphead *gh, int metoo, int ntype)
{
	struct group *gp;
	struct grouphead *ngh;
	struct name *np;
	static int depth;
	char *cp;

	if (depth > MAXEXP) {
		DEBUG_PRINT(catgets(catd, CATSET, 150,
			"Expanding alias to depth larger than %d\n"), MAXEXP);
		return(nlist);
	}
	depth++;
	for (gp = gh->g_list; gp != NULL; gp = gp->ge_link) {
		cp = gp->ge_name;
		if (*cp == '\\')
			goto quote;
		if (strcmp(cp, gh->g_name) == 0)
			goto quote;
		if ((ngh = findgroup(cp)) != NULL) {
			nlist = gexpand(nlist, ngh, metoo, ntype);
			continue;
		}
quote:
		np = nalloc(cp, ntype|GFULL);
		/*
		 * At this point should allow to expand
		 * to self if only person in group
		 */
		if (gp == gh->g_list && gp->ge_link == NULL)
			goto skip;
		if (!metoo && same_name(cp, myname))
			np->n_type |= GDEL;
skip:
		nlist = put(nlist, np);
	}
	depth--;
	return(nlist);
}

/*
 * Concatenate the two passed name lists, return the result.
 */
struct name *
cat(struct name *n1, struct name *n2)
{
	struct name *tail;

	if (n1 == NULL)
		return(n2);
	if (n2 == NULL)
		return(n1);
	tail = tailof(n1);
	tail->n_flink = n2;
	n2->n_blink = tail;
	return(n1);
}

/*
 * Remove all of the duplicates from the passed name list by
 * insertion sorting them, then checking for dups.
 * Return the head of the new list.
 */
struct name *
elide(struct name *names)
{
	struct name *np, *t, *new;
	struct name *x;

	if (names == NULL)
		return(NULL);
	new = names;
	np = names;
	np = np->n_flink;
	if (np != NULL)
		np->n_blink = NULL;
	new->n_flink = NULL;
	while (np != NULL) {
		t = new;
		while (asccasecmp(t->n_name, np->n_name) < 0) {
			if (t->n_flink == NULL)
				break;
			t = t->n_flink;
		}

		/*
		 * If we ran out of t's, put the new entry after
		 * the current value of t.
		 */

		if (asccasecmp(t->n_name, np->n_name) < 0) {
			t->n_flink = np;
			np->n_blink = t;
			t = np;
			np = np->n_flink;
			t->n_flink = NULL;
			continue;
		}

		/*
		 * Otherwise, put the new entry in front of the
		 * current t.  If at the front of the list,
		 * the new guy becomes the new head of the list.
		 */

		if (t == new) {
			t = np;
			np = np->n_flink;
			t->n_flink = new;
			new->n_blink = t;
			t->n_blink = NULL;
			new = t;
			continue;
		}

		/*
		 * The normal case -- we are inserting into the
		 * middle of the list.
		 */

		x = np;
		np = np->n_flink;
		x->n_flink = t;
		x->n_blink = t->n_blink;
		t->n_blink->n_flink = x;
		t->n_blink = x;
	}

	/*
	 * Now the list headed up by new is sorted.
	 * Go through it and remove duplicates.
	 */

	np = new;
	while (np != NULL) {
		t = np;
		while (t->n_flink != NULL &&
		       asccasecmp(np->n_name, t->n_flink->n_name) == 0)
			t = t->n_flink;
		if (t == np || t == NULL) {
			np = np->n_flink;
			continue;
		}
		
		/*
		 * Now t points to the last entry with the same name
		 * as np.  Make np point beyond t.
		 */

		np->n_flink = t->n_flink;
		if (t->n_flink != NULL)
			t->n_flink->n_blink = np;
		np = np->n_flink;
	}
	return(new);
}

/*
 * Put another node onto a list of names and return
 * the list.
 */
static struct name *
put(struct name *list, struct name *node)
{
	node->n_flink = list;
	node->n_blink = NULL;
	if (list != NULL)
		list->n_blink = node;
	return(node);
}

/*
 * Determine the number of undeleted elements in
 * a name list and return it.
 */
int 
count(struct name *np)
{
	int c;

	for (c = 0; np != NULL; np = np->n_flink)
		if ((np->n_type & GDEL) == 0)
			c++;
	return c;
}
