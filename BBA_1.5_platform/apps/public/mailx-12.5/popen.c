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
static char sccsid[] = "@(#)popen.c	2.20 (gritter) 3/4/06";
#endif
#endif /* not lint */

#include "rcv.h"
#include "extern.h"
#include <sys/stat.h>
#include <fcntl.h>
#include <unistd.h>
#include <sys/wait.h>
#include <errno.h>

#include "mail.h"

#ifndef	NSIG
#define	NSIG	32
#endif

#define READ 0
#define WRITE 1

struct fp {
	FILE *fp;
	struct fp *link;
	char *realfile;
	long offset;
	int omode;
	int pipe;
	int pid;
	enum {
		FP_UNCOMPRESSED	= 00,
		FP_GZIPPED	= 01,
		FP_BZIP2ED	= 02,
		FP_IMAP		= 03,
		FP_MAILDIR	= 04,
		FP_MASK		= 0177,
		FP_READONLY	= 0200
	} compressed;
};
static struct fp *fp_head;

struct child {
	int pid;
	char done;
	char free;
	int status;
	struct child *link;
};
static struct child	*child;

static int scan_mode(const char *mode, int *omode);
static void register_file(FILE *fp, int omode);
static enum okay compress(struct fp *fpp);
static enum okay unregister_file(FILE *fp);
static int file_pid(FILE *fp);
static int wait_command(int pid);
static struct child *findchild(int pid);
static void delchild(struct child *cp);

/*
 * Provide BSD-like signal() on all systems.
 */
sighandler_type
safe_signal(int signum, sighandler_type handler)
{
	struct sigaction nact, oact;

	nact.sa_handler = handler;
	sigemptyset(&nact.sa_mask);
	nact.sa_flags = 0;
#ifdef	SA_RESTART
	nact.sa_flags |= SA_RESTART;
#endif
	if (sigaction(signum, &nact, &oact) != 0)
		return SIG_ERR;
	return oact.sa_handler;
}

static int 
scan_mode(const char *mode, int *omode)
{

	if (!strcmp(mode, "r")) {
		*omode = O_RDONLY;
	} else if (!strcmp(mode, "w")) {
		*omode = O_WRONLY | O_CREAT | O_TRUNC;
	} else if (!strcmp(mode, "wx")) {
		*omode = O_WRONLY | O_CREAT | O_EXCL;
	} else if (!strcmp(mode, "a")) {
		*omode = O_WRONLY | O_APPEND | O_CREAT;
	} else if (!strcmp(mode, "a+")) {
		*omode = O_RDWR | O_APPEND;
	} else if (!strcmp(mode, "r+")) {
		*omode = O_RDWR;
	} else if (!strcmp(mode, "w+")) {
		*omode = O_RDWR   | O_CREAT | O_EXCL;
	} else {
		DEBUG_ERROR( catgets(catd, CATSET, 152,
			"Internal error: bad stdio open mode %s\n"), mode);
		errno = EINVAL;
		return -1;
	}
	return 0;
}

FILE *
safe_fopen(const char *file, const char *mode, int *omode)
{
	int  fd;

	if (scan_mode(mode, omode) < 0) 
		return NULL;
	if ((fd = open(file, *omode, 0666)) < 0)
		return NULL;
	return fdopen(fd, mode);
}

FILE *
Fopen(const char *file, const char *mode)
{
	FILE *fp;
	int omode;

	if ((fp = safe_fopen(file, mode, &omode)) != NULL) {
		register_file(fp, omode);
		fcntl(fileno(fp), F_SETFD, FD_CLOEXEC);
	}
	return fp;
}

FILE *
Fdopen(int fd, const char *mode)
{
	FILE *fp;
	int	omode;

	scan_mode(mode, &omode);
	if ((fp = fdopen(fd, mode)) != NULL) {
		register_file(fp, omode);
		fcntl(fileno(fp), F_SETFD, FD_CLOEXEC);
	}
	return fp;
}

int
Fclose(FILE *fp)
{
	int	i = 0;
	if (unregister_file(fp) == OKAY)
		i |= 1;
	if (fclose(fp) == 0)
		i |= 2;
	return i == 3 ? 0 : EOF;
}

int
Pclose(FILE *ptr)
{
	int i;
	sigset_t nset, oset;

	i = file_pid(ptr);
	if (i < 0)
		return 0;
	unregister_file(ptr);
	fclose(ptr);
	sigemptyset(&nset);
	sigaddset(&nset, SIGINT);
	sigaddset(&nset, SIGHUP);
	sigprocmask(SIG_BLOCK, &nset, &oset);
	i = wait_child(i);
	sigprocmask(SIG_SETMASK, &oset, (sigset_t *)NULL);
	return i;
}

void 
close_all_files(void)
{

	while (fp_head)
		if (fp_head->pipe)
			Pclose(fp_head->fp);
		else
			Fclose(fp_head->fp);
}

static void
register_file(FILE *fp, int omode)
{
	struct fp *fpp;

	fpp = (struct fp*)smalloc(sizeof *fpp);
	fpp->fp = fp;
	fpp->omode = omode;
	fpp->pipe = 0;
	fpp->pid = 0;
	fpp->link = fp_head;
	fpp->compressed = FP_UNCOMPRESSED;
	fpp->realfile = NULL;
	fpp->offset = 0L;
	fp_head = fpp;
}

static enum okay
compress(struct fp *fpp)
{
	int	output;
	char	*command[2];
	enum okay	ok;

	if (fpp->omode == O_RDONLY)
		return OKAY;
	fflush(fpp->fp);
	clearerr(fpp->fp);
	fseek(fpp->fp, fpp->offset, SEEK_SET);
	if ((fpp->compressed&FP_MASK) == FP_IMAP) {
		return STOP;
	}
	if ((fpp->compressed&FP_MASK) == FP_MAILDIR) {
		return maildir_append(fpp->realfile, fpp->fp);
	}
	if ((output = open(fpp->realfile,
			(fpp->omode|O_CREAT)&~O_EXCL,
			0666)) < 0) {
		DEBUG_ERROR("Fatal: cannot create ");
		perror(fpp->realfile);
		return STOP;
	}
	if ((fpp->omode & O_APPEND) == 0)
		ftruncate(output, 0);
	switch (fpp->compressed & FP_MASK) {
	case FP_GZIPPED:
		command[0] = "gzip"; command[1] = "-c"; break;
	case FP_BZIP2ED:
		command[0] = "bzip2"; command[1] = "-c"; break;
	default:
		command[0] = "cat"; command[1] = NULL; break;
	}
	if (run_command(command[0], 0, fileno(fpp->fp), output,
				command[1], NULL, NULL) < 0)
		ok = STOP;
	else
		ok = OKAY;
	close(output);
	return ok;
}

static enum okay
unregister_file(FILE *fp)
{
	struct fp **pp, *p;
	enum okay	ok = OKAY;

	for (pp = &fp_head; (p = *pp) != (struct fp *)NULL; pp = &p->link)
		if (p->fp == fp) {
			if ((p->compressed&FP_MASK) != FP_UNCOMPRESSED)
				ok = compress(p);
			*pp = p->link;
			free(p);
			return ok;
		}
	panic(catgets(catd, CATSET, 153, "Invalid file pointer"));
	/*NOTREACHED*/
	return STOP;
}

static int
file_pid(FILE *fp)
{
	struct fp *p;

	for (p = fp_head; p; p = p->link)
		if (p->fp == fp)
			return (p->pid);
	return -1;
}

/*
 * Run a command without a shell, with optional arguments and splicing
 * of stdin and stdout.  The command name can be a sequence of words.
 * Signals must be handled by the caller.
 * "Mask" contains the signals to ignore in the new process.
 * SIGINT is enabled unless it's in the mask.
 */
/*VARARGS4*/
int
run_command(char *cmd, sigset_t *mask, int infd, int outfd,
		char *a0, char *a1, char *a2)
{
	int pid;

	if ((pid = start_command(cmd, mask, infd, outfd, a0, a1, a2)) < 0)
		return -1;
	return wait_command(pid);
}

/*VARARGS4*/
int
start_command(const char *cmd, sigset_t *mask, int infd, int outfd,
		const char *a0, const char *a1, const char *a2)
{
	int pid;

	if ((pid = fork()) < 0) {
		perror("fork");
		return -1;
	}
	if (pid == 0) {
		char *argv[100];
		int i = getrawlist(cmd, strlen(cmd),
				argv, sizeof argv / sizeof *argv, 0);

		if ((argv[i++] = (char *)a0) != NULL &&
		    (argv[i++] = (char *)a1) != NULL &&
		    (argv[i++] = (char *)a2) != NULL)
			argv[i] = NULL;
		prepare_child(mask, infd, outfd);
		execvp(argv[0], argv);
		perror(argv[0]);
		mail_exit(MAIL_STATUS_ERR_GENERAL);
	}
	return pid;
}

void
prepare_child(sigset_t *nset, int infd, int outfd)
{
	int i;
	sigset_t fset;

	/*
	 * All file descriptors other than 0, 1, and 2 are supposed to be
	 * close-on-exec.
	 */
	if (infd >= 0)
		dup2(infd, 0);
	if (outfd >= 0)
		dup2(outfd, 1);
	if (nset) {
		for (i = 1; i < NSIG; i++)
			if (sigismember(nset, i))
				safe_signal(i, SIG_IGN);
		if (!sigismember(nset, SIGINT))
			safe_signal(SIGINT, SIG_DFL);
	}
	sigfillset(&fset);
	sigprocmask(SIG_UNBLOCK, &fset, (sigset_t *)NULL);
}

static int 
wait_command(int pid)
{

	if (wait_child(pid) < 0 && (value("bsdcompat") || value("bsdmsgs"))) {
		DEBUG_PRINT(catgets(catd, CATSET, 154, "Fatal error in process.\n"));
		return -1;
	}
	return 0;
}

static struct child *
findchild(int pid)
{
	struct child **cpp;

	for (cpp = &child; *cpp != (struct child *)NULL && (*cpp)->pid != pid;
	     cpp = &(*cpp)->link)
			;
	if (*cpp == (struct child *)NULL) {
		*cpp = (struct child *) smalloc(sizeof (struct child));
		(*cpp)->pid = pid;
		(*cpp)->done = (*cpp)->free = 0;
		(*cpp)->link = (struct child *)NULL;
	}
	return *cpp;
}

static void 
delchild(struct child *cp)
{
	struct child **cpp;

	for (cpp = &child; *cpp != cp; cpp = &(*cpp)->link)
		;
	*cpp = cp->link;
	free(cp);
}

/*ARGSUSED*/
void 
sigchild(int signo)
{
	int pid;
	int status;
	struct child *cp;

again:
	while ((pid = waitpid(-1, (int*)&status, WNOHANG)) > 0) {
		cp = findchild(pid);
		if (cp->free)
			delchild(cp);
		else {
			cp->done = 1;
			cp->status = status;
		}
	}
	if (pid == -1 && errno == EINTR)
		goto again;
}

int wait_status;

int 
wait_child(int pid)
{
	pid_t term;
	struct child *cp;
	struct sigaction nact, oact;
	
	nact.sa_handler = SIG_DFL;
	sigemptyset(&nact.sa_mask);
	nact.sa_flags = SA_NOCLDSTOP;
	sigaction(SIGCHLD, &nact, &oact);
	
	cp = findchild(pid);
	if (!cp->done) {
		do {
			term = wait(&wait_status);
			if (term == -1 && errno == EINTR)
				continue;
			if (term == 0 || term == -1)
				break;
			cp = findchild(term);
			if (cp->free || term == pid) {
				delchild(cp);
			} else {
				cp->done = 1;
				cp->status = wait_status;
			}
		} while (term != pid);
	} else {
		wait_status = cp->status;
		delchild(cp);
	}
	
	sigaction(SIGCHLD, &oact, NULL);
	/*
	 * Make sure no zombies are left.
	 */
	sigchild(SIGCHLD);
	
	if (WIFEXITED(wait_status) && (WEXITSTATUS(wait_status) == 0))
		return 0;
	return -1;
}
