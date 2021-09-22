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
static char sccsid[] = "@(#)vars.c	2.12 (gritter) 10/1/08";
#endif
#endif /* not lint */

#include "rcv.h"
#include "extern.h"

/*
 * Mail -- a mail program
 *
 * Variable handling stuff.
 */

static char *canonify(const char *vn);
static struct var *lookup(const char *name); 
static struct var *vcalloc_one();

static struct var vars[MAXVAR];
static int top = 0;

/*
 * If a variable name begins with a lowercase-character and contains at
 * least one '@', it is converted to all-lowercase. This is necessary
 * for lookups of names based on email addresses.
 *
 * Following the standard, only the part following the last '@' should
 * be lower-cased, but practice has established otherwise here.
 */
static char *
canonify(const char *vn)
{
	const char	*vp;

	if (upperchar(*vn&0377))
		return (char *)vn;
	for (vp = vn; *vp && *vp != '@'; vp++);
	if (*vp == '@')
		return i_strdup(vn);
	return (char *)vn;
}

static struct var *vcalloc_one()
{
	if (top == MAXVAR)
		return NULL;

	struct var *ret = &vars[top];
	++top;

	memset(ret, 0, sizeof(struct var));
	return ret;
}

/*
 * Assign a value to a variable.
 */
void 
assign(const char *name, const char *value)
{
	struct var *vp;
	int h;

	name = canonify(name);
	h = hash(name);
	vp = lookup(name);
	if (vp == NULL) {
		vp = vcalloc_one();
		strncpy(vp->v_name, name, sizeof(vp->v_name)); 
		vp->v_link = variables[h];
		variables[h] = vp;
	}
	strncpy(vp->v_value, value, sizeof(vp->v_value));
}

/*
 * Get the value of a variable and return it.
 * Look in the environment if its not available locally.
 */

char *
value(const char *name)
{
	struct var *vp;
	char	*vs;

	name = canonify(name);
	if ((vp = lookup(name)) == NULL) {
		if ((vs = getenv(name)) != NULL && *vs)
			vs = savestr(vs);
		return vs;
	}
	return vp->v_value;
}

/*
 * Locate a variable and return its variable
 * node.
 */

static struct var *
lookup(const char *name)
{
	struct var *vp;

	for (vp = variables[hash(name)]; vp != NULL; vp = vp->v_link)
		if (vp->v_name[0] == *name && equal(vp->v_name, name))
			return(vp);
	return(NULL);
}

/*
 * Locate a group name and return it.
 */

struct grouphead *
findgroup(char *name)
{
	struct grouphead *gh;

	for (gh = groups[hash(name)]; gh != NULL; gh = gh->g_link)
		if (*gh->g_name == *name && equal(gh->g_name, name))
			return(gh);
	return(NULL);
}
 
/*
 * Hash the passed string and return an index into
 * the variable or group hash table.
 */
int 
hash(const char *name)
{
	int h = 0;

	while (*name) {
		h <<= 2;
		h += *name++;
	}
	if (h < 0 && (h = -h) < 0)
		h = 0;
	return (h % HSHSIZE);
}

int 
unset_internal(const char *name)
{
	struct var *vp, *vp2;
	int h;

	name = canonify(name);
	if ((vp2 = lookup(name)) == NULL) {
		if (!sourcing && !unset_allow_undefined) {
			DEBUG_PRINT(catgets(catd, CATSET, 203,
				"\"%s\": undefined variable\n"), name);
			return 1;
		}
		return 0;
	}
	h = hash(name);
	if (vp2 == variables[h]) {
		variables[h] = variables[h]->v_link;
		free(vp2);
		return 0;
	}
	for (vp = variables[h]; vp->v_link != vp2; vp = vp->v_link);
	vp->v_link = vp2->v_link;
	free(vp2);
	return 0;
} 