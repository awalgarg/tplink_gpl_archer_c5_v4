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
static char copyright[]
= "@(#) Copyright (c) 2000, 2002 Gunnar Ritter. All rights reserved.\n";
static char sccsid[]  = "@(#)mime.c	2.71 (gritter) 7/5/10";
#endif /* DOSCCS */
#endif /* not lint */

#include "rcv.h"
#include "extern.h"
#include <ctype.h>
#include <errno.h>
#ifdef	HAVE_WCTYPE_H
#include <wctype.h>
#endif	/* HAVE_WCTYPE_H */

/*
 * Mail -- a mail program
 *
 * MIME support functions.
 */

/*
 * You won't guess what these are for.
 */
static const char basetable[] = "0123456789ABCDEFGHIJKLMNOPQRSTUVWXYZ";
static char *mimetypes_world = "/etc/mime.types";
static char *mimetypes_user = "~/.mime.types";
char *us_ascii = "us-ascii";

static int mustquote_hdr(const char *cp, int wordstart, int wordend);
static int mustquote_inhdrq(int c);
static size_t	delctrl(char *cp, size_t sz);
static char *getcharset(int isclean);
static int has_highbit(register const char *s);
static int is_this_enc(const char *line, const char *encoding);
static char *mime_tline(char *x, char *l);
static char *mime_type(char *ext, char *filename);
static enum mimeclean mime_isclean(FILE *f);
static char *ctohex(int c, char *hex);
static void mime_fromqp(struct str *in, struct str *out, int ishdr);
static size_t mime_write_tohdr(struct str *in, FILE *fo);
static size_t convhdra(char *str, size_t len, FILE *fp);
static size_t mime_write_tohdr_a(struct str *in, FILE *f);
static void addstr(char **buf, size_t *sz, size_t *pos, char *str, size_t len);
static void addconv(char **buf, size_t *sz, size_t *pos, char *str, size_t len);
static size_t fwrite_td(void *ptr, size_t size, size_t nmemb, FILE *f,
		enum tdflags flags, char *prefix, size_t prefixlen);

/*
 * Check if c must be quoted inside a message's header.
 */
static int 
mustquote_hdr(const char *cp, int wordstart, int wordend)
{
	int	c = *cp & 0377;

	if (c != '\n' && (c < 040 || c >= 0177))
		return 1;
	if (wordstart && cp[0] == '=' && cp[1] == '?')
		return 1;
	if (cp[0] == '?' && cp[1] == '=' &&
			(wordend || cp[2] == '\0' || whitechar(cp[2]&0377)))
		return 1;
	return 0;
}

/*
 * Check if c must be quoted inside a quoting in a message's header.
 */
static int 
mustquote_inhdrq(int c)
{
	if (c != '\n'
		&& (c <= 040 || c == '=' || c == '?' || c == '_' || c >= 0177))
		return 1;
	return 0;
}

static size_t
delctrl(char *cp, size_t sz)
{
	size_t	x = 0, y = 0;

	while (x < sz) {
		if (!cntrlchar(cp[x]&0377))
			cp[y++] = cp[x];
		x++;
	}
	return y;
}

/*
 * Check if a name's address part contains invalid characters.
 */
int 
mime_name_invalid(char *name, int putmsg)
{
	char *addr, *p;
	int in_quote = 0, in_domain = 0, err = 0, hadat = 0;

	if (is_fileaddr(name))
		return 0;
	addr = skin(name);

	if (addr == NULL || *addr == '\0')
		return 1;
	for (p = addr; *p != '\0'; p++) {
		if (*p == '\"') {
			in_quote = !in_quote;
		} else if (*p < 040 || (*p & 0377) >= 0177) {
			err = *p & 0377;
			break;
		} else if (in_domain == 2) {
			if ((*p == ']' && p[1] != '\0') || *p == '\0'
					|| *p == '\\' || whitechar(*p & 0377)) {
				err = *p & 0377;
				break;
			}
		} else if (in_quote && in_domain == 0) {
			/*EMPTY*/;
		} else if (*p == '\\' && p[1] != '\0') {
			p++;
		} else if (*p == '@') {
			if (hadat++) {
				if (putmsg) {
					DEBUG_ERROR( catgets(catd, CATSET,
								142,
					"%s contains invalid @@ sequence\n"),
						addr);
					putmsg = 0;
				}
				err = *p;
				break;
			}
			if (p[1] == '[')
				in_domain = 2;
			else
				in_domain = 1;
			continue;
		} else if (*p == '(' || *p == ')' || *p == '<' || *p == '>'
				|| *p == ',' || *p == ';' || *p == ':'
				|| *p == '\\' || *p == '[' || *p == ']') {
			err = *p & 0377;
			break;
		}
		hadat = 0;
	}
	if (err && putmsg) {
		DEBUG_ERROR( catgets(catd, CATSET, 143,
				"%s contains invalid character '"), addr);
#ifdef	HAVE_SETLOCALE
		if (isprint(err))
#else	/* !HAVE_SETLOCALE */
		if (err >= 040 && err <= 0177)
#endif	/* !HAVE_SETLOCALE */
			DEBUG_ERROR("%c", err);
		else
			DEBUG_ERROR("\\%03o", err);
		DEBUG_ERROR( catgets(catd, CATSET, 144, "'\n"));
	}
	return err;
}

/*
 * Check all addresses in np and delete invalid ones.
 */
struct name *
checkaddrs(struct name *np)
{
	struct name *n = np;

	while (n != NULL) {
		if (mime_name_invalid(n->n_name, 1)) {
			if (n->n_blink)
				n->n_blink->n_flink = n->n_flink;
			if (n->n_flink)
				n->n_flink->n_blink = n->n_blink;
			if (n == np)
				np = n->n_flink;
		}
		n = n->n_flink;
	}
	return np;
}

static char defcharset[] = "utf-8";

/*
 * Get the character set dependant on the conversion.
 */
static char *
getcharset(int isclean)
{
	char *charset;

	if (isclean & (MIME_CTRLCHAR|MIME_HASNUL))
		charset = NULL;
	else if (isclean & MIME_HIGHBIT) {
		charset = (wantcharset && wantcharset != (char *)-1) ?
			wantcharset : value("charset");
		if (charset == NULL) {
			charset = defcharset;
		}
	} else {
		/*
		 * This variable shall remain undocumented because
		 * only experts should change it.
		 */
		charset = value("charset7");
		if (charset == NULL) {
			charset = us_ascii;
		}
	}
	return charset;
}

/*
 * Get the setting of the terminal's character set.
 */
char *
gettcharset(void)
{
	char *t;

	if ((t = value("ttycharset")) == NULL)
		if ((t = value("charset")) == NULL)
			t = defcharset;
	return t;
}

static int 
has_highbit(const char *s)
{
	if (s) {
		do
			if (*s & 0200)
				return 1;
		while (*s++ != '\0');
	}
	return 0;
}

static int
name_highbit(struct name *np)
{
	while (np) {
		if (has_highbit(np->n_name) || has_highbit(np->n_fullname))
			return 1;
		np = np->n_flink;
	}
	return 0;
}

char *
need_hdrconv(struct header *hp, enum gfield w)
{
	if (w & GIDENT) {
		if (hp->h_from && name_highbit(hp->h_from))
			goto needs;
		else if (has_highbit(myaddrs(hp)))
			goto needs;
		if (hp->h_organization && has_highbit(hp->h_organization))
			goto needs;
		else if (has_highbit(value("ORGANIZATION")))
			goto needs;
		if (hp->h_replyto && name_highbit(hp->h_replyto))
			goto needs;
		else if (has_highbit(value("replyto")))
			goto needs;
		if (hp->h_sender && name_highbit(hp->h_sender))
			goto needs;
		else if (has_highbit(value("sender")))
			goto needs;
	}
	if (w & GTO && name_highbit(hp->h_to))
		goto needs;
	if (w & GCC && name_highbit(hp->h_cc))
		goto needs;
	if (w & GBCC && name_highbit(hp->h_bcc))
		goto needs;
	if (w & GSUBJECT && has_highbit(hp->h_subject))
		goto needs;
	return NULL;
needs:	return getcharset(MIME_HIGHBIT);
}

static int
is_this_enc(const char *line, const char *encoding)
{
	int quoted = 0, c;

	if (*line == '"')
		quoted = 1, line++;
	while (*line && *encoding)
		if (c = *line++, lowerconv(c) != *encoding++)
			return 0;
	if (quoted && *line == '"')
		return 1;
	if (*line == '\0' || whitechar(*line & 0377))
		return 1;
	return 0;
}

/*
 * Get the mime encoding from a Content-Transfer-Encoding header field.
 */
enum mimeenc 
mime_getenc(char *p)
{
	if (is_this_enc(p, "7bit"))
		return MIME_7B;
	if (is_this_enc(p, "8bit"))
		return MIME_8B;
	if (is_this_enc(p, "base64"))
		return MIME_B64;
	if (is_this_enc(p, "binary"))
		return MIME_BIN;
	if (is_this_enc(p, "quoted-printable"))
		return MIME_QP;
	return MIME_NONE;
}

/*
 * Get the mime content from a Content-Type header field, other parameters
 * already stripped.
 */
int 
mime_getcontent(char *s)
{
	if (strchr(s, '/') == NULL)	/* for compatibility with non-MIME */
		return MIME_TEXT;
	if (asccasecmp(s, "text/plain") == 0)
		return MIME_TEXT_PLAIN;
	if (asccasecmp(s, "text/html") == 0)
		return MIME_TEXT_HTML;
	if (ascncasecmp(s, "text/", 5) == 0)
		return MIME_TEXT;
	if (asccasecmp(s, "message/rfc822") == 0)
		return MIME_822;
	if (ascncasecmp(s, "message/", 8) == 0)
		return MIME_MESSAGE;
	if (asccasecmp(s, "multipart/alternative") == 0)
		return MIME_ALTERNATIVE;
	if (asccasecmp(s, "multipart/digest") == 0)
		return MIME_DIGEST;
	if (ascncasecmp(s, "multipart/", 10) == 0)
		return MIME_MULTI;
	if (asccasecmp(s, "application/x-pkcs7-mime") == 0 ||
				asccasecmp(s, "application/pkcs7-mime") == 0)
		return MIME_PKCS7;
	return MIME_UNKNOWN;
}

/*
 * Get a mime style parameter from a header line.
 */
char *
mime_getparam(char *param, char *h)
{
	char *p = h, *q, *r;
	int c;
	size_t sz;

	sz = strlen(param);
	if (!whitechar(*p & 0377)) {
		c = '\0';
		while (*p && (*p != ';' || c == '\\')) {
			c = c == '\\' ? '\0' : *p;
			p++;
		}
		if (*p++ == '\0')
			return NULL;
	}
	for (;;) {
		while (whitechar(*p & 0377))
			p++;
		if (ascncasecmp(p, param, sz) == 0) {
			p += sz;
			while (whitechar(*p & 0377))
				p++;
			if (*p++ == '=')
				break;
		}
		c = '\0';
		while (*p && (*p != ';' || c == '\\')) {
			if (*p == '"' && c != '\\') {
				p++;
				while (*p && (*p != '"' || c == '\\')) {
					c = c == '\\' ? '\0' : *p;
					p++;
				}
				p++;
			} else {
				c = c == '\\' ? '\0' : *p;
				p++;
			}
		}
		if (*p++ == '\0')
			return NULL;
	}
	while (whitechar(*p & 0377))
		p++;
	q = p;
	c = '\0';
	if (*p == '"') {
		p++;
		if ((q = strchr(p, '"')) == NULL)
			return NULL;
	} else {
		q = p;
		while (*q && !whitechar(*q & 0377) && *q != ';')
			q++;
	}
	sz = q - p;
	r = salloc(q - p + 1);
	memcpy(r, p, sz);
	*(r + sz) = '\0';
	return r;
}

/*
 * Get the boundary out of a Content-Type: multipart/xyz header field.
 */
char *
mime_getboundary(char *h)
{
	char *p, *q;
	size_t sz;

	if ((p = mime_getparam("boundary", h)) == NULL)
		return NULL;
	sz = strlen(p);
	q = salloc(sz + 3);
	memcpy(q, "--", 2);
	memcpy(q + 2, p, sz);
	*(q + sz + 2) = '\0';
	return q;
}

/*
 * Get a line like "text/html html" and look if x matches the extension.
 */
static char *
mime_tline(char *x, char *l)
{
	char *type, *n;
	int match = 0;

	if ((*l & 0200) || alphachar(*l & 0377) == 0)
		return NULL;
	type = l;
	while (blankchar(*l & 0377) == 0 && *l != '\0')
		l++;
	if (*l == '\0')
		return NULL;
	*l++ = '\0';
	while (blankchar(*l & 0377) != 0 && *l != '\0')
		l++;
	if (*l == '\0')
		return NULL;
	while (*l != '\0') {
		n = l;
		while (whitechar(*l & 0377) == 0 && *l != '\0')
			l++;
		if (*l != '\0')
			*l++ = '\0';
		if (strcmp(x, n) == 0) {
			match = 1;
			break;
		}
		while (whitechar(*l & 0377) != 0 && *l != '\0')
			l++;
	}
	if (match != 0) {
		n = salloc(strlen(type) + 1);
		strcpy(n, type);
		return n;
	}
	return NULL;
}

/*
 * Check the given MIME type file for extension ext.
 */
static char *
mime_type(char *ext, char *filename)
{
	FILE *f;
	char *line = NULL;
	size_t linesize = 0;
	char *type = NULL;

	if ((f = Fopen(filename, "r")) == NULL)
		return NULL;
	while (fgetline(&line, &linesize, NULL, NULL, f, 0)) {
		if ((type = mime_tline(ext, line)) != NULL)
			break;
	}
	Fclose(f);
	if (line)
		free(line);
	return type;
}

/*
 * Return the Content-Type matching the extension of name.
 */
char *
mime_filecontent(char *name)
{
	char *ext, *content;

	if ((ext = strrchr(name, '.')) == NULL || *++ext == '\0')
		return NULL;
	if ((content = mime_type(ext, expand(mimetypes_user))) != NULL)
		return content;
	if ((content = mime_type(ext, mimetypes_world)) != NULL)
		return content;
	return NULL;
}

/*
 * Check file contents.
 */
static enum mimeclean
mime_isclean(FILE *f)
{
	long initial_pos;
	unsigned curlen = 1, maxlen = 0, limit = 950;
	enum mimeclean isclean = 0;
	char	*cp;
	int c = EOF, lastc;

	initial_pos = ftell(f);
	do {
		lastc = c;
		c = getc(f);
		curlen++;
		if (c == '\n' || c == EOF) {
			/*
			 * RFC 821 imposes a maximum line length of 1000
			 * characters including the terminating CRLF
			 * sequence. The configurable limit must not
			 * exceed that including a safety zone.
			 */
			if (curlen > maxlen)
				maxlen = curlen;
			curlen = 1;
		} else if (c & 0200) {
			isclean |= MIME_HIGHBIT;
		} else if (c == '\0') {
			isclean |= MIME_HASNUL;
			break;
		} else if ((c < 040 && (c != '\t' && c != '\f')) || c == 0177) {
			isclean |= MIME_CTRLCHAR;
		}
	} while (c != EOF);
	if (lastc != '\n')
		isclean |= MIME_NOTERMNL;
	clearerr(f);
	fseek(f, initial_pos, SEEK_SET);
	if ((cp = value("maximum-unencoded-line-length")) != NULL)
		limit = atoi(cp);
	if (limit < 0 || limit > 950)
		limit = 950;
	if (maxlen > limit)
		isclean |= MIME_LONGLINES;
	return isclean;
}

int
get_mime_convert(FILE *fp, char **contenttype, char **charset,
		enum mimeclean *isclean)
{
	int convert;
	
	*isclean = mime_isclean(fp);
	if (*isclean & MIME_HASNUL ||
			*contenttype && ascncasecmp(*contenttype, "text/", 5)) {
		convert = CONV_TOB64;
		if (*contenttype == NULL ||
				ascncasecmp(*contenttype, "text/", 5) == 0)
			*contenttype = "application/octet-stream";
		*charset = NULL;
	}else if (*isclean & (MIME_LONGLINES|MIME_CTRLCHAR|MIME_NOTERMNL)){
		//don't support quote-printed encoding, use base64 instead
		convert = CONV_TOB64;
	}
	else if (*isclean & MIME_HIGHBIT)
		convert = CONV_8BIT;
	else
		convert = CONV_7BIT;

	if (*contenttype == NULL ||
			ascncasecmp(*contenttype, "text/", 5) == 0) {
		*charset = getcharset(*isclean);
		if (wantcharset == (char *)-1) {
			*contenttype = "application/octet-stream";
			*charset = NULL;
		} if (*isclean & MIME_CTRLCHAR) {
			convert = CONV_TOB64;
			/*
			 * RFC 2046 forbids control characters other than
			 * ^I or ^L in text/plain bodies. However, some
			 * obscure character sets actually contain these
			 * characters, so the content type can be set.
			 */
			if ((*contenttype = value("contenttype-cntrl")) == NULL)
				*contenttype = "application/octet-stream";
		} else if (*contenttype == NULL)
			*contenttype = "text/plain";
	}
	return convert;
}

/*
 * Convert c to a hexadecimal character string and store it in hex.
 */
static char *
ctohex(int c, char *hex)
{
	unsigned char d;

	hex[2] = '\0';
	d = c % 16;
	hex[1] = basetable[d];
	if (c > d)
		hex[0] = basetable[(c - d) / 16];
	else
		hex[0] = basetable[0];
	return hex;
}

/*
 * Write to a stringstruct converting to quoted-printable.
 * The mustquote function determines whether a character must be quoted.
 */
static void 
mime_str_toqp(struct str *in, struct str *out, int (*mustquote)(int), int inhdr)
{
	char *p, *q, *upper;

	out->s = smalloc(in->l * 3 + 1);
	q = out->s;
	out->l = in->l;
	upper = in->s + in->l;
	for (p = in->s; p < upper; p++) {
		if (mustquote(*p&0377) || p+1 < upper && *(p + 1) == '\n' &&
				blankchar(*p & 0377)) {
			if (inhdr && *p == ' ') {
				*q++ = '_';
			} else {
				out->l += 2;
				*q++ = '=';
				ctohex(*p&0377, q);
				q += 2;
			}
		} else {
			*q++ = *p;
		}
	}
	*q = '\0';
}

/*
 * Write to a stringstruct converting from quoted-printable.
 */
static void 
mime_fromqp(struct str *in, struct str *out, int ishdr)
{
	char *p, *q, *upper;
	char quote[4];

	out->l = in->l;
	out->s = smalloc(out->l + 1);
	upper = in->s + in->l;
	for (p = in->s, q = out->s; p < upper; p++) {
		if (*p == '=') {
			do {
				p++;
				out->l--;
			} while (blankchar(*p & 0377) && p < upper);
			if (p == upper)
				break;
			if (*p == '\n') {
				out->l--;
				continue;
			}
			if (p + 1 >= upper)
				break;
			quote[0] = *p++;
			quote[1] = *p;
			quote[2] = '\0';
			*q = (char)strtol(quote, NULL, 16);
			q++;
			out->l--;
		} else if (ishdr && *p == '_')
			*q++ = ' ';
		else
			*q++ = *p;
	}
	return;
}

#define	mime_fromhdr_inc(inc) { \
		size_t diff = q - out->s; \
		out->s = srealloc(out->s, (maxstor += inc) + 1); \
		q = &(out->s)[diff]; \
	}
/*
 * Convert header fields from RFC 1522 format
 */
void 
mime_fromhdr(struct str *in, struct str *out, enum tdflags flags)
{
	char *p, *q, *op, *upper, *cs, *cbeg, *tcs, *lastwordend = NULL;
	struct str cin, cout;
	int convert;
	size_t maxstor, lastoutl = 0;

	tcs = gettcharset();
	maxstor = in->l;
	out->s = smalloc(maxstor + 1);
	out->l = 0;
	upper = in->s + in->l;
	for (p = in->s, q = out->s; p < upper; p++) {
		op = p;
		if (*p == '=' && *(p + 1) == '?') {
			p += 2;
			cbeg = p;
			while (p < upper && *p != '?')
				p++;	/* strip charset */
			if (p >= upper)
				goto notmime;
			cs = salloc(++p - cbeg);
			memcpy(cs, cbeg, p - cbeg - 1);
			cs[p - cbeg - 1] = '\0';
			switch (*p) {
			case 'B': case 'b':
				convert = CONV_FROMB64;
				break;
			case 'Q': case 'q':
				convert = CONV_FROMQP;
				break;
			default:	/* invalid, ignore */
				goto notmime;
			}
			if (*++p != '?')
				goto notmime;
			cin.s = ++p;
			cin.l = 1;
			for (;;) {
				if (p == upper)
					goto fromhdr_end;
				if (*p++ == '?' && *p == '=')
					break;
				cin.l++;
			}
			cin.l--;
			switch (convert) {
				case CONV_FROMB64:
					mime_fromb64(&cin, &cout, 1);
					break;
				case CONV_FROMQP:
					mime_fromqp(&cin, &cout, 1);
					break;
			}
			if (lastwordend) {
				q = lastwordend;
				out->l = lastoutl;
			}
				while (cout.l > maxstor - out->l)
					mime_fromhdr_inc(cout.l -
							(maxstor - out->l));
				memcpy(q, cout.s, cout.l);
				q += cout.l;
				out->l += cout.l;
			free(cout.s);
			lastwordend = q;
			lastoutl = out->l;
		} else {
notmime:
			p = op;
			while (out->l >= maxstor)
				mime_fromhdr_inc(16);
			*q++ = *p;
			out->l++;
			if (!blankchar(*p&0377))
				lastwordend = NULL;
		}
	}
fromhdr_end:
	*q = '\0';
	if (flags & TD_ISPR) {
		struct str	new;
		makeprint(out, &new);
		free(out->s);
		*out = new;
	}
	if (flags & TD_DELCTRL)
		out->l = delctrl(out->s, out->l);
	return;
}

/*
 * Convert header fields to RFC 1522 format and write to the file fo.
 */
static size_t
mime_write_tohdr(struct str *in, FILE *fo)
{
	char *upper, *wbeg, *wend, *charset, *lastwordend = NULL, *lastspc, b,
		*charset7;
	struct str cin, cout;
	size_t sz = 0, col = 0, wr, charsetlen, charset7len;
	int quoteany, mustquote, broken,
		maxcol = 65 /* there is the header field's name, too */;

	upper = in->s + in->l;
	charset = getcharset(MIME_HIGHBIT);
	if ((charset7 = value("charset7")) == NULL)
		charset7 = us_ascii;
	charsetlen = strlen(charset);
	charset7len = strlen(charset7);
	charsetlen = smax(charsetlen, charset7len);
	b = 0;
	for (wbeg = in->s, quoteany = 0; wbeg < upper; wbeg++) {
		b |= *wbeg;
		if (mustquote_hdr(wbeg, wbeg == in->s, wbeg == &upper[-1]))
			quoteany++;
	}
	if (2 * quoteany > in->l) {
		/*
		 * Print the entire field in base64.
		 */
		for (wbeg = in->s; wbeg < upper; wbeg = wend) {
			wend = upper;
			cin.s = wbeg;
			for (;;) {
				cin.l = wend - wbeg;
				if (cin.l * 4/3 + 7 + charsetlen
						< maxcol - col) {
					fprintf(fo, "=?%s?B?",
						b&0200 ? charset : charset7);
					wr = mime_write_tob64(&cin, fo, 1);
					fwrite("?=", sizeof (char), 2, fo);
					wr += 7 + charsetlen;
					sz += wr, col += wr;
					if (wend < upper) {
						fwrite("\n ", sizeof (char),
								2, fo);
						sz += 2;
						col = 0;
						maxcol = 76;
					}
					break;
				} else {
					if (col) {
						fprintf(fo, "\n ");
						sz += 2;
						col = 0;
						maxcol = 76;
					} else
						wend -= 4;
				}
			}
		}
	} else {
		/*
		 * Print the field word-wise in quoted-printable.
		 */
		broken = 0;
		for (wbeg = in->s; wbeg < upper; wbeg = wend) {
			lastspc = NULL;
			while (wbeg < upper && whitechar(*wbeg & 0377)) {
				lastspc = lastspc ? lastspc : wbeg;
				wbeg++;
				col++;
				broken = 0;
			}
			if (wbeg == upper) {
				if (lastspc)
					while (lastspc < wbeg) {
						putc(*lastspc&0377, fo);
							lastspc++,
							sz++;
						}
				break;
			}
			mustquote = 0;
			b = 0;
			for (wend = wbeg;
				wend < upper && !whitechar(*wend & 0377);
					wend++) {
				b |= *wend;
				if (mustquote_hdr(wend, wend == wbeg,
							wbeg == &upper[-1]))
					mustquote++;
			}
			if (mustquote || broken || (wend - wbeg) >= 74 &&
					quoteany) {
				for (;;) {
					cin.s = lastwordend ? lastwordend :
						wbeg;
					cin.l = wend - cin.s;
					mime_str_toqp(&cin, &cout,
							mustquote_inhdrq, 1);
					if ((wr = cout.l + charsetlen + 7)
							< maxcol - col) {
						if (lastspc)
							while (lastspc < wbeg) {
								putc(*lastspc
									&0377,
									fo);
								lastspc++,
								sz++;
							}
						fprintf(fo, "=?%s?Q?", b&0200 ?
							charset : charset7);
						fwrite(cout.s, sizeof *cout.s,
								cout.l, fo);
						fwrite("?=", 1, 2, fo);
						sz += wr, col += wr;
						free(cout.s);
						break;
					} else {
						broken = 1;
						if (col) {
							putc('\n', fo);
							sz++;
							col = 0;
							maxcol = 76;
							if (lastspc == NULL) {
								putc(' ', fo);
								sz++;
								maxcol--;
							} else
								maxcol -= wbeg -
									lastspc;
						} else {
							wend -= 4;
						}
						free(cout.s);
					}
				}
				lastwordend = wend;
			} else {
				if (col && wend - wbeg > maxcol - col) {
					putc('\n', fo);
					sz++;
					col = 0;
					maxcol = 76;
					if (lastspc == NULL) {
						putc(' ', fo);
						sz++;
						maxcol--;
					} else
						maxcol -= wbeg - lastspc;
				}
				if (lastspc)
					while (lastspc < wbeg) {
						putc(*lastspc&0377, fo);
						lastspc++, sz++;
					}
				wr = fwrite(wbeg, sizeof *wbeg,
						wend - wbeg, fo);
				sz += wr, col += wr;
				lastwordend = NULL;
			}
		}
	}
	return sz;
}

/*
 * Write len characters of the passed string to the passed file, 
 * doing charset and header conversion.
 */
static size_t
convhdra(char *str, size_t len, FILE *fp)
{
	struct str	cin;
	size_t	cbufsz;
	char	*cbuf;
	size_t	sz;

	cbuf = ac_alloc(cbufsz = 1);
		cin.s = str;
		cin.l = len;
	sz = mime_write_tohdr(&cin, fp);
	ac_free(cbuf);
	return sz;
}


/*
 * Write an address to a header field.
 */
static size_t
mime_write_tohdr_a(struct str *in, FILE *f)
{
	char	*cp, *lastcp;
	size_t	sz = 0;

	in->s[in->l] = '\0';
	lastcp = in->s;
	if ((cp = routeaddr(in->s)) != NULL && cp > lastcp) {
		sz += convhdra(lastcp, cp - lastcp, f);
		lastcp = cp;
	} else
		cp = in->s;
	for ( ; *cp; cp++) {
		switch (*cp) {
		case '(':
			sz += fwrite(lastcp, 1, cp - lastcp + 1, f);
			lastcp = ++cp;
			cp = skip_comment(cp);
			if (--cp > lastcp)
				sz += convhdra(lastcp, cp - lastcp, f);
			lastcp = cp;
			break;
		case '"':
			while (*cp) {
				if (*++cp == '"')
					break;
				if (*cp == '\\' && cp[1])
					cp++;
			}
			break;
		}
	}
	if (cp > lastcp)
		sz += fwrite(lastcp, 1, cp - lastcp, f);
	return sz;
}

static void
addstr(char **buf, size_t *sz, size_t *pos, char *str, size_t len)
{
	*buf = srealloc(*buf, *sz += len);
	memcpy(&(*buf)[*pos], str, len);
	*pos += len;
}

static void
addconv(char **buf, size_t *sz, size_t *pos, char *str, size_t len)
{
	struct str	in, out;

	in.s = str;
	in.l = len;
	mime_fromhdr(&in, &out, TD_ISPR|TD_ICONV);
	addstr(buf, sz, pos, out.s, out.l);
	free(out.s);
}

/*
 * Interpret MIME strings in parts of an address field.
 */
char *
mime_fromaddr(char *name)
{
	char	*cp, *lastcp;
	char	*res = NULL;
	size_t	ressz = 1, rescur = 0;

	if (name == NULL || *name == '\0')
		return name;
	if ((cp = routeaddr(name)) != NULL && cp > name) {
		addconv(&res, &ressz, &rescur, name, cp - name);
		lastcp = cp;
	} else
		cp = lastcp = name;
	for ( ; *cp; cp++) {
		switch (*cp) {
		case '(':
			addstr(&res, &ressz, &rescur, lastcp, cp - lastcp + 1);
			lastcp = ++cp;
			cp = skip_comment(cp);
			if (--cp > lastcp)
				addconv(&res, &ressz, &rescur, lastcp,
						cp - lastcp);
			lastcp = cp;
			break;
		case '"':
			while (*cp) {
				if (*++cp == '"')
					break;
				if (*cp == '\\' && cp[1])
					cp++;
			}
			break;
		}
	}
	if (cp > lastcp)
		addstr(&res, &ressz, &rescur, lastcp, cp - lastcp);
	res[rescur] = '\0';
	cp = savestr(res);
	free(res);
	return cp;
}

/*
 * fwrite whilst adding prefix, if present.
 */
size_t
prefixwrite(void *ptr, size_t size, size_t nmemb, FILE *f,
		char *prefix, size_t prefixlen)
{
	static FILE *lastf;
	static char lastc = '\n';
	size_t rsz, wsz = 0;
	char *p = ptr;

	if (nmemb == 0)
		return 0;
	if (prefix == NULL) {
		lastf = f;
		lastc = ((char *)ptr)[size * nmemb - 1];
		return fwrite(ptr, size, nmemb, f);
	}
	if (f != lastf || lastc == '\n') {
		if (*p == '\n' || *p == '\0')
			wsz += fwrite(prefix, sizeof *prefix, prefixlen, f);
		else {
			fputs(prefix, f);
			wsz += strlen(prefix);
		}
	}
	lastf = f;
	for (rsz = size * nmemb; rsz; rsz--, p++, wsz++) {
		putc(*p, f);
		if (*p != '\n' || rsz == 1) {
			continue;
		}
		if (p[1] == '\n' || p[1] == '\0')
			wsz += fwrite(prefix, sizeof *prefix, prefixlen, f);
		else {
			fputs(prefix, f);
			wsz += strlen(prefix);
		}
	}
	lastc = p[-1];
	return wsz;
}

/*
 * fwrite while checking for displayability.
 */
static size_t
fwrite_td(void *ptr, size_t size, size_t nmemb, FILE *f, enum tdflags flags,
		char *prefix, size_t prefixlen)
{
	char *upper;
	size_t sz, csize;
	char *mptr, *xmptr, *mlptr = NULL;
	size_t mptrsz;

	csize = size * nmemb;
	mptrsz = csize;
	mptr = xmptr = ac_alloc(mptrsz + 1);
	{
		memcpy(mptr, ptr, csize);
	}
	upper = mptr + csize;
	*upper = '\0';
	if (flags & TD_ISPR) {
		struct str	in, out;
		in.s = mptr;
		in.l = csize;
		makeprint(&in, &out);
		mptr = mlptr = out.s;
		csize = out.l;
	}
	if (flags & TD_DELCTRL)
		csize = delctrl(mptr, csize);
	sz = prefixwrite(mptr, sizeof *mptr, csize, f, prefix, prefixlen);
	ac_free(xmptr);
	free(mlptr);
	return sz;
}

/*
 * fwrite performing the given MIME conversion.
 */
size_t
mime_write(void *ptr, size_t size, FILE *f,
		enum conversion convert, enum tdflags dflags,
		char *prefix, size_t prefixlen,
		char **restp, size_t *restsizep)
{
	struct str in, out;
	size_t sz, csize;
	int is_text = 0;

	if (size == 0)
		return 0;
	csize = size;
		in.s = ptr;
		in.l = csize;
	switch (convert) {
	case CONV_FROMQP:
		mime_fromqp(&in, &out, 0);
		sz = fwrite_td(out.s, sizeof *out.s, out.l, f, dflags,
				prefix, prefixlen);
		free(out.s);
		break;
	case CONV_8BIT:
		sz = prefixwrite(in.s, sizeof *in.s, in.l, f,
				prefix, prefixlen);
		break;
	case CONV_FROMB64_T:
		is_text = 1;
		/*FALLTHROUGH*/
	case CONV_FROMB64:
		mime_fromb64_b(&in, &out, is_text, f);
		if (is_text && out.s[out.l-1] != '\n' && restp && restsizep) {
			*restp = ptr;
			*restsizep = size;
			sz = 0;
		} else {
			sz = fwrite_td(out.s, sizeof *out.s, out.l, f, dflags,
				prefix, prefixlen);
		}
		free(out.s);
		break;
	case CONV_TOB64:
		sz = mime_write_tob64(&in, f, 0);
		break;
	case CONV_FROMHDR:
		mime_fromhdr(&in, &out, TD_ISPR|TD_ICONV);
		sz = fwrite_td(out.s, sizeof *out.s, out.l, f,
				dflags&TD_DELCTRL, prefix, prefixlen);
		free(out.s);
		break;
	case CONV_TOHDR:
		sz = mime_write_tohdr(&in, f);
		break;
	case CONV_TOHDR_A:
		sz = mime_write_tohdr_a(&in, f);
		break;
	default:
		sz = fwrite_td(in.s, sizeof *in.s, in.l, f, dflags,
				prefix, prefixlen);
	}
	return sz;
}
