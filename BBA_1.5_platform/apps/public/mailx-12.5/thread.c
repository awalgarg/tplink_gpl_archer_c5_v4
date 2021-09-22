/*
 * Heirloom mailx - a mail user agent derived from Berkeley Mail.
 *
 * Copyright (c) 2000-2004 Gunnar Ritter, Freiburg i. Br., Germany.
 */
/*
 * Copyright (c) 2004
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
static char sccsid[] = "@(#)thread.c	1.57 (gritter) 3/4/06";
#endif
#endif /* not lint */

#include "config.h"

#include "rcv.h"
#include "extern.h"
#include <time.h>

/*
 * Mail -- a mail program
 *
 * Message threading.
 */

/*
 * Open addressing is used for Message-IDs because the maximum number of
 * messages in the table is known in advance (== msgCount).
 */
struct	mitem {
	struct message	*mi_data;
	char	*mi_id;
};

struct msort {
	union {
		long	ms_long;
		char	*ms_char;
		float	ms_float;
	} ms_u;
	int	ms_n;
};
static void colps(struct message *b, int cl);
static void colpm(struct message *m, int cl, int *cc, int *uc);

#define	NOT_AN_ID	((struct mitem *)-1)

struct message *
next_in_thread(struct message *mp)
{
	if (mp->m_child)
		return mp->m_child;
	if (mp->m_younger)
		return mp->m_younger;
	while (mp->m_parent) {
		if (mp->m_parent->m_younger)
			return mp->m_parent->m_younger;
		mp = mp->m_parent;
	}
	return NULL;
}

struct message *
prev_in_thread(struct message *mp)
{
	if (mp->m_elder) {
		mp = mp->m_elder;
		while (mp->m_child) {
			mp = mp->m_child;
			while (mp->m_younger)
				mp = mp->m_younger;
		}
		return mp;
	}
	return mp->m_parent;
}

static void 
colps(struct message *b, int cl)
{
	struct message	*m;
	int	cc = 0, uc = 0;

	if (cl && (b->m_collapsed > 0 || (b->m_flag & (MNEW|MREAD)) == MNEW))
		return;
	if (b->m_child) {
		m = b->m_child;
		colpm(m, cl, &cc, &uc);
		for (m = m->m_younger; m; m = m->m_younger)
			colpm(m, cl, &cc, &uc);
	}
	if (cl) {
		b->m_collapsed = -cc;
		for (m = b->m_parent; m; m = m->m_parent)
			if (m->m_collapsed <= -uc ) {
				m->m_collapsed += uc;
				break;
			}
	} else {
		if (b->m_collapsed > 0) {
			b->m_collapsed = 0;
			uc++;
		}
		for (m = b; m; m = m->m_parent)
			if (m->m_collapsed <= -uc) {
				m->m_collapsed += uc;
				break;
			}
	}
}

static void 
colpm(struct message *m, int cl, int *cc, int *uc)
{
	if (cl) {
		if (m->m_collapsed > 0)
			(*uc)++;
		if ((m->m_flag & (MNEW|MREAD)) != MNEW || m->m_collapsed < 0)
			m->m_collapsed = 1;
		if (m->m_collapsed > 0)
			(*cc)++;
	} else {
		if (m->m_collapsed > 0) {
			m->m_collapsed = 0;
			(*uc)++;
		}
	}
	if (m->m_child) {
		m = m->m_child;
		colpm(m, cl, cc, uc);
		for (m = m->m_younger; m; m = m->m_younger)
			colpm(m, cl, cc, uc);
	}
}

void 
uncollapse1(struct message *m, int always)
{
	if (mb.mb_threaded == 1 && (always || m->m_collapsed > 0))
		colps(m, 0);
}
