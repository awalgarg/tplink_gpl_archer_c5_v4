/*
 * Heirloom mailx - a mail user agent derived from Berkeley Mail.
 *
 * Copyright (c) 2000-2004 Gunnar Ritter, Freiburg i. Br., Germany.
 */
/*-
 * Copyright (c) 1992, 1993
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
 *
 *	Sccsid @(#)extern.h	2.162 (gritter) 10/1/08
 */

/* aux.c */
char *savestr(const char *str);
char *save2str(const char *str, const char *old);
char *savecat(const char *s1, const char *s2);
void panic(const char *format, ...);
void mail_exit(int mailStatus);
void success(void);
void checking(void);
void i_strcpy(char *dest, const char *src, int size);
char *i_strdup(const char *src);
int substr(const char *str, const char *sub);
int unstack(void);
int anyof(char *s1, char *s2);
int is_prefix(const char *as1, const char *as2);
enum protocol which_protocol(const char *name);
char *protbase(const char *cp);
long nextprime(long n);
char *md5tohex(const void *vp);
char *cram_md5_string(const char *user, const char *pass, const char *b64);
char *getpassword(struct termios *otio, int *reset_tio, const char *query);
char *getrandstring(size_t length);
void out_of_memory(void);
void *smalloc(size_t s);
void *srealloc(void *v, size_t s);
void *scalloc(size_t nmemb, size_t size);
char *sstpcpy(char *dst, const char *src);
char *sstrdup(const char *cp);
enum okay makedir(const char *name);
void makeprint(struct str *in, struct str *out);
char *prstr(const char *s);
int asccasecmp(const char *s1, const char *s2);
int ascncasecmp(const char *s1, const char *s2, size_t sz);
char *asccasestr(const char *haystack, const char *xneedle);
/* base64.c */
char *strtob64(const char *p);
char *memtob64(const void *vp, size_t isz);
size_t mime_write_tob64(struct str *in, FILE *fo, int is_header);
void mime_fromb64(struct str *in, struct str *out, int is_text);
void mime_fromb64_b(struct str *in, struct str *out, int is_text, FILE *f);
/* cmd3.c */
int set(void *v);
struct shortcut *get_shortcut(const char *str);
/* cmdtab.c */
/* collect.c */
struct attachment *add_attachment(struct attachment *attach, const char *file);
FILE *collect(struct header *hp);
void savedeadletter(FILE *fp);
/* fio.c */
int putline(FILE *obuf, char *linebuf, size_t count);
#define	readline(a, b, c)	readline_restart(a, b, c, 0)
int readline_restart(FILE *ibuf, char **linebuf, size_t *linesize, size_t n);
FILE *setinput(struct mailbox *mp, struct message *m, enum needspec need);
struct message *setdot(struct message *mp);
int rm(char *name);
void holdsigs(void);
void relsesigs(void);
off_t fsize(FILE *iob);
char *expand(char *name);
int getfold(char *name, int size);
char *getdeadletter(void);
char *fgetline(char **line, size_t *linesize, size_t *count,
		size_t *llen, FILE *fp, int appendnl);
void newline_appended(void);
enum okay get_body(struct message *mp);
int sclose(struct sock *sp);
enum okay swrite(struct sock *sp, const char *data);
enum okay swrite1(struct sock *sp, const char *data, int sz, int use_buffer);
int sgetline(char **line, size_t *linesize, size_t *linelen, struct sock *sp);
enum okay sopen(const char *xserver, struct sock *sp, int use_ssl,
		const char *uhp, const char *portstr);
/* getname.c */
char *getname(int uid);
int getuserid(char *name);
/* getopt.c */
int getopt(int argc, char *const argv[], const char *optstring);
/* head.c */
char *thisfield(const char *linebuf, const char *field);
char *skip_comment(const char *cp);
char *routeaddr(const char *name);
char *skin(char *name);
char *nexttoken(char *cp);
time_t combinetime(int year, int month, int day,
		int hour, int minute, int second);
int check_from_and_sender(struct name *fromfield, struct name *senderfield);
/* lex.c */
void commands(void);
int execute(char *linebuf, int contxt, size_t linesize);
void load(char *name);
/* list.c */ 
int getrawlist(const char *line, size_t linesize,
		char **argv, int argc, int echolist);
int first(int f, int m);
/* maildir.c */
enum okay maildir_append(const char *name, FILE *fp);
/* main.c */
int main(int argc, char *argv[]);
/* mime.c */
int mime_name_invalid(char *name, int putmsg);
struct name *checkaddrs(struct name *np);
char *gettcharset(void);
char *need_hdrconv(struct header *hp, enum gfield w);
enum mimeenc mime_getenc(char *h);
int mime_getcontent(char *h);
char *mime_getparam(char *param, char *h);
char *mime_getboundary(char *h);
char *mime_filecontent(char *name);
int get_mime_convert(FILE *fp, char **contenttype, char **charset,
		enum mimeclean *isclean);
void mime_fromhdr(struct str *in, struct str *out, enum tdflags flags);
char *mime_fromaddr(char *name);
size_t prefixwrite(void *ptr, size_t size, size_t nmemb, FILE *f,
		char *prefix, size_t prefixlen);
size_t mime_write(void *ptr, size_t size, FILE *f,
		enum conversion convert, enum tdflags dflags,
		char *prefix, size_t prefixlen,
		char **rest, size_t *restsize);
/* names.c */
struct name *nalloc(char *str, enum gfield ntype);
struct name *extract(char *line, enum gfield ntype);
struct name *sextract(char *line, enum gfield ntype);
int is_fileaddr(char *name);
struct name *usermap(struct name *names);
struct name *cat(struct name *n1, struct name *n2);
struct name *elide(struct name *names);
int count(struct name *np);
/* openssl.c */
enum okay ssl_open(const char *server, struct sock *sp, const char *uhp);
void ssl_gen_err(const char *fmt, ...);
FILE *smime_sign(FILE *ip, struct header *);
FILE *smime_encrypt(FILE *ip, const char *certfile, const char *to);
struct message *smime_decrypt(struct message *m, const char *to,
		const char *cc, int signcall);
/* popen.c */
sighandler_type safe_signal(int signum, sighandler_type handler);
FILE *safe_fopen(const char *file, const char *mode, int *omode);
FILE *Fopen(const char *file, const char *mode);
FILE *Fdopen(int fd, const char *mode);
int Fclose(FILE *fp);
int Pclose(FILE *ptr);
void close_all_files(void);
int run_command(char *cmd, sigset_t *mask, int infd, int outfd,
		char *a0, char *a1, char *a2);
int start_command(const char *cmd, sigset_t *mask, int infd, int outfd,
		const char *a0, const char *a1, const char *a2);
void prepare_child(sigset_t *nset, int infd, int outfd);
void sigchild(int signo);
int wait_child(int pid);
/* sendout.c */
char *makeboundary(void);
int mail(struct name *to, struct name *smopts, char *subject, struct attachment *attach);
enum okay mail1(struct header *hp);
int mkdate(FILE *fo, const char *field);
int puthead(struct header *hp, FILE *fo, enum gfield w,
		enum sendaction action, enum conversion convert,
		char *contenttype, char *charset);
char *getencoding(enum conversion convert);
int put_signature(FILE *fo, int convert);
attach_file(struct attachment *ap, enum conversion convert, enum mimeclean isclean, FILE *fi, FILE *fo);
/* smtp.c */
char *nodename(int mayoverride);
char *myaddrs(struct header *hp);
char *myorigin(struct header *hp);
char *smtp_auth_var(const char *type, const char *addr);
int smtp_mta(char *server, struct name *to, FILE *fi, struct header *hp,
		const char *user, const char *password, const char *skinned);

/* ssl.c */
void ssl_set_vrfy_level(const char *uhp);
enum okay ssl_vrfy_decide(void);
char *ssl_method_string(const char *uhp);
enum okay smime_split(FILE *ip, FILE **hp, FILE **bp, long xcount, int keep);
FILE *smime_sign_assemble(FILE *hp, FILE *bp, FILE *sp);
FILE *smime_encrypt_assemble(FILE *hp, FILE *yp);
struct message *smime_decrypt_assemble(struct message *m, FILE *hp, FILE *bp);
enum okay rfc2595_hostname_match(const char *host, const char *pattern);

/* strings.c */
void *salloc(size_t size);
void *csalloc(size_t nmemb, size_t size);
void sreset(void);
void spreserve(void);
/* temp.c */
FILE *Ftemp(char **fn, char *prefix, char *mode, int bits, int register_file);
void Ftfree(char **fn);
void tinit(void);
/* thread.c */
struct message *next_in_thread(struct message *mp);
struct message *prev_in_thread(struct message *mp);
struct message *this_in_thread(struct message *mp, long n);
void uncollapse1(struct message *m, int always);
/* v7.local.c */
void findmail(char *user, int force, char *buf, int size);
char *username(void);
/* vars.c */
void assign(const char *name, const char *value);
char *value(const char *name);
struct grouphead *findgroup(char *name);
int hash(const char *name);
int unset_internal(const char *name);
 /* version.c */
