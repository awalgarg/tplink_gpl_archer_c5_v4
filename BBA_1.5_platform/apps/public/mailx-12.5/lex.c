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
static char sccsid[] = "@(#)lex.c	2.86 (gritter) 12/25/06";
#endif
#endif /* not lint */

#include "rcv.h"
#include "extern.h"
#include <errno.h>
#include <sys/stat.h>
#include <fcntl.h>
#include <unistd.h>

#include "mail.h"
/*
 * Mail -- a mail program
 *
 * Lexical processing of commands.
 */

static char	*prompt;
static sighandler_type	oldpipe;

static const struct cmd *lex(char *Word);
static void stop(int s);
static void hangup(int s);

static int	*msgvec;
static int	reset_on_stop;			/* do a reset() if stopped */

/*
 * Interpret user commands one by one.  If standard input is not a tty,
 * print no prompt.
 */
void 
commands(void)
{
	int eofloop = 0;
	int n, x;
	char *linebuf = NULL, *av, *nv;
	size_t linesize = 0;

	(void)&eofloop;
	if (!sourcing) {
		if (safe_signal(SIGHUP, SIG_IGN) != SIG_IGN)
			safe_signal(SIGHUP, hangup);
		safe_signal(SIGTSTP, stop);
		safe_signal(SIGTTOU, stop);
		safe_signal(SIGTTIN, stop);
	}
	oldpipe = safe_signal(SIGPIPE, SIG_IGN);
	safe_signal(SIGPIPE, oldpipe);
	setexit();
	for (;;) {
		interrupts = 0;
		handlerstacktop = NULL; 
		fflush(stdout);
		sreset();
		/*
		 * Read a line of commands from the current input
		 * and handle end of file specially.
		 */
		n = 0;
		
		for (;;) {
			n = readline_restart(input, &linebuf, &linesize, n);
			if (n < 0)
				break;
			if (n == 0 || linebuf[n - 1] != '\\')
				break;
			linebuf[n - 1] = ' ';
		}
		
		reset_on_stop = 0;
		if (n < 0) {
				/* eof */
			if (loading)
				break;
			if (sourcing) {
				unstack();
				continue;
			} 
			break;
		}
		eofloop = 0;
		inhook = 0;
		if (execute(linebuf, 0, n))
			break;
	}
	if (linebuf)
		free(linebuf);
}

/*
 * Execute a single command.
 * Command functions return 0 for success, 1 for error, and -1
 * for abort.  A 1 or -1 aborts a load or source.  A -1 aborts
 * the interactive command loop.
 * Contxt is non-zero if called while composing mail.
 */
int
execute(char *linebuf, int contxt, size_t linesize)
{
	char *word;
	char *arglist[MAXARGC];
	const struct cmd *com = (struct cmd *)NULL;
	char *cp, *cp2;
	int c;
	int muvec[2];
	int e = 1;

	/*
	 * Strip the white space away from the beginning
	 * of the command, then scan out a word, which
	 * consists of anything except digits and white space.
	 *
	 * Handle ! escapes differently to get the correct
	 * lexical conventions.
	 */
	word = ac_alloc(linesize + 1);
	for (cp = linebuf; whitechar(*cp & 0377); cp++);
	cp2 = word;
	if (*cp != '|') {
		while (*cp && strchr(" \t0123456789$^.:/-+*'\",;(`", *cp)
				== NULL)
			*cp2++ = *cp++;
	} else
		*cp2++ = *cp++;
	*cp2 = '\0';

	/*
	 * Look up the command; if not found, bitch.
	 * Normally, a blank command would map to the
	 * first command in the table; while sourcing,
	 * however, we ignore blank lines to eliminate
	 * confusion.
	 */

	if (sourcing && *word == '\0') {
		ac_free(word);
		return(0);
	}
	
	com = lex(word);
	if (com == NULL) {
		DEBUG_PRINT(catgets(catd, CATSET, 91,
				"Unknown command: \"%s\"\n"), word);
		goto out;
	}

	/*
	 * See if we should execute the command -- if a conditional
	 * we always execute it, otherwise, check the state of cond.
	 */

	if ((com->c_argtype & F) == 0) {
		if ((cond == CRCV && !rcvmode) ||
				(cond == CSEND && rcvmode) ||
				(cond == CTERM)) {
			ac_free(word);
			return(0);
		}
	}

	/*
	 * Process the arguments to the command, depending
	 * on the type he expects.  Default to an error.
	 * If we are sourcing an interactive command, it's
	 * an error.
	 */

	if (!rcvmode && (com->c_argtype & M) == 0) {
		DEBUG_PRINT(catgets(catd, CATSET, 92,
			"May not execute \"%s\" while sending\n"), com->c_name);
		goto out;
	}
	switch (com->c_argtype & ~(F|P|I|M|T|W|R|A)) {
		/* only command 'set' supported */
		case RAWLIST:
			/*
			 * A vector of strings, in shell style.
			 */
			if ((c = getrawlist(cp, linesize, arglist,
					sizeof arglist / sizeof *arglist,
					(com->c_argtype&~(F|P|I|M|T|W|R|A))==ECHOLIST))
						< 0)
				break;
			if (c < com->c_minargs) {
				DEBUG_PRINT(catgets(catd, CATSET, 99,
					"%s requires at least %d arg(s)\n"),
					com->c_name, com->c_minargs);
				break;
			}
			if (c > com->c_maxargs) {
				DEBUG_PRINT(catgets(catd, CATSET, 100,
					"%s takes no more than %d arg(s)\n"),
					com->c_name, com->c_maxargs);
				break;
			}
			e = (*com->c_func)(arglist);
			break; 
		default:
			panic(catgets(catd, CATSET, 101, "Unknown argtype"));
	}
out:
	ac_free(word);
	/*
	 * Exit the current source file on
	 * error.
	 */
	if (e) {
		if (e < 0)
			return 1;
		if (loading)
			return 1;
		if (sourcing)
			unstack();
		return 0;
	}
	if (com == (struct cmd *)NULL)
		return(0);
	if (!sourcing && !inhook && (com->c_argtype & T) == 0)
		sawcom = 1;
	return(0);
}

/*
 * Find the correct command in the command table corresponding
 * to the passed command "word"
 */

static const struct cmd *
lex(char *Word)
{
	extern const struct cmd cmdtab[];
	const struct cmd *cp;

	for (cp = &cmdtab[0]; cp->c_name != NULL; cp++)
		if (is_prefix(Word, cp->c_name))
			return(cp);
	return(NULL);
}

/*
 * The following gets called on receipt of an interrupt.  This is
 * to abort printout of a command, mainly.
 * Dispatching here when command() is inactive crashes rcv.
 * Close all open files except 0, 1, 2, and the temporary.
 * Also, unstack all source files.
 */

static int	inithdr;		/* am printing startup headers */
/*
 * When we wake up after ^Z, reprint the prompt.
 */
static void 
stop(int s)
{
	sighandler_type old_action = safe_signal(s, SIG_DFL);
	sigset_t nset;

	sigemptyset(&nset);
	sigaddset(&nset, s);
	sigprocmask(SIG_UNBLOCK, &nset, (sigset_t *)NULL);
	kill(0, s);
	sigprocmask(SIG_BLOCK, &nset, (sigset_t *)NULL);
	safe_signal(s, old_action);
	if (reset_on_stop) {
		reset_on_stop = 0;
		reset(0);
	}
}

/*
 * Branch here on hangup signal and simulate "exit".
 */
/*ARGSUSED*/
static void 
hangup(int s)
{

	/* nothing to do? */
	mail_exit(MAIL_STATUS_ERR_GENERAL);
}

/*
 * Load a file of user definitions.
 */
void 
load(char *name)
{
	FILE *in, *oldin;
	if ((in = fopen(name, "r")) == NULL)
		return;
	oldin = input;
	input = in;
	loading = 1;
	sourcing = 1;
	commands();
	loading = 0;
	sourcing = 0;
	input = oldin;
	fclose(in);
}
