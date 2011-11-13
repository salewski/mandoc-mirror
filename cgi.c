/*	$Id$ */
#include <assert.h>
#include <fcntl.h>
#include <regex.h>
#include <stdio.h>
#include <stdarg.h>
#include <stdlib.h>
#include <string.h>

#include "apropos_db.h"
#include "mandoc.h"

/*
 * The page a request is trying to make.
 */
enum	page {
	PAGE_INDEX,
	PAGE_SEARCH,
	PAGE__MAX
};

/*
 * Key-value pair.  
 * Both key and val are on the heap.
 */
struct	kval {
	char		*key;
	char		*val;
};

/*
 * The media type, determined by suffix, of the requesting or responding
 * context.
 */
enum	media {
	MEDIA_HTML,
	MEDIA__MAX
};

/*
 * An HTTP request.
 */
struct	req {
	struct kval	*fields; /* query fields */
	size_t		 fieldsz;
	enum media	 media;
	enum page	 page;
};

#if 0
static	void		 html_printtext(const char *);
#endif
static	int 		 kval_decode(char *);
static	void		 kval_parse(struct kval **, size_t *, char *);
static	void		 kval_free(struct kval *, size_t);
static	void		 pg_index(const struct req *, char *);
static	void		 pg_search(const struct req *, char *);
#if 0
static	void		 pg_searchres(struct rec *, size_t, void *);
#endif

static	const char * const pages[PAGE__MAX] = {
	"index", /* PAGE_INDEX */ 
	"search", /* PAGE_SEARCH */
};

static	const char * const medias[MEDIA__MAX] = {
	"html", /* MEDIA_HTML */
};

#if 0
static void
html_printtext(const char *p)
{
	char		 c;

	while ('\0' != *p)
		switch ((c = *p++)) {
		case ('"'):
			printf("&quote;");
			break;
		case ('&'):
			printf("&amp;");
			break;
		case ('>'):
			printf("&gt;");
			break;
		case ('<'):
			printf("&lt;");
			break;
		default:
			putchar((unsigned char)c);
			break;
		}
}
#endif

static void
kval_free(struct kval *p, size_t sz)
{
	int		 i;

	for (i = 0; i < (int)sz; i++) {
		free(p[i].key);
		free(p[i].val);
	}
	free(p);
}

/*
 * Parse out key-value pairs from an HTTP request variable.
 * This can be either a cookie or a POST/GET string.
 */
static void
kval_parse(struct kval **kv, size_t *kvsz, char *p)
{
	char            *key, *val;
	size_t           sz, cur;

	cur = 0;

	while (p && '\0' != *p) {
		while (' ' == *p)
			p++;

		key = p;
		val = NULL;

		if (NULL != (p = strchr(p, '='))) {
			*p++ = '\0';
			val = p;

			sz = strcspn(p, ";&");
			/* LINTED */
			p += sz;

			if ('\0' != *p)
				*p++ = '\0';
		} else {
			p = key;
			sz = strcspn(p, ";&");
			/* LINTED */
			p += sz;

			if ('\0' != *p)
				p++;
			continue;
		}

		if ('\0' == *key || '\0' == *val)
			continue;

		/* Just abort handling. */

		if ( ! kval_decode(key))
			return;
		if ( ! kval_decode(val))
			return;

		if (*kvsz + 1 >= cur) {
			cur++;
			*kv = mandoc_realloc
				(*kv, cur * sizeof(struct kval));
		}

		(*kv)[(int)*kvsz].key = mandoc_strdup(key);
		(*kv)[(int)*kvsz].val = mandoc_strdup(val);
		(*kvsz)++;
	}
}

/*
 * In-place HTTP-decode a string.  The standard explanation is that this
 * turns "%4e+foo" into "n foo" in the regular way.  This is done
 * in-place over the allocated string.
 */
static int
kval_decode(char *p)
{
	char             hex[3];
	int              c;

	hex[2] = '\0';

	for ( ; '\0' != *p; p++) {
		if ('%' == *p) {
			if ('\0' == (hex[0] = *(p + 1)))
				return(0);
			if ('\0' == (hex[1] = *(p + 2)))
				return(0);
			if (1 != sscanf(hex, "%x", &c))
				return(0);
			if ('\0' == c)
				return(0);

			*p = (char)c;
			memmove(p + 1, p + 3, strlen(p + 3) + 1);
		} else
			*p = '+' == *p ? ' ' : *p;
	}

	*p = '\0';
	return(1);
}


/* ARGSUSED */
static void
pg_index(const struct req *req, char *path)
{

}

#if 0
static void
pg_searchres(struct rec *recs, size_t sz, void *arg)
{
	int		 i;
	const char	*pg;

	if (NULL == (pg = getenv("SCRIPT_NAME")))
		pg = "";

	for (i = 0; i < (int)sz; i++) {
		printf("<A HREF=\"%s/show/%u.html\">", 
				pg, recs[i].rec);
		html_printtext(recs[i].title);
		putchar('(');
		html_printtext(recs[i].cat);
		puts(")</A>");
	}
}
#endif

static void
pg_search(const struct req *req, char *path)
{
	int		 i;
	struct opts	 opt;

	for (i = 0; i < (int)req->fieldsz; i++)
		if (0 == strcmp(req->fields[i].key, "key"))
			break;

	if (i == (int)req->fieldsz)
		return;

	memset(&opt, 0, sizeof(struct opts));
	/*opt.types = TYPE_NAME | TYPE_DESC;
	apropos_search(&opt, req->fields[i].val, NULL, pg_searchres);*/
}

int
main(void)
{
	int		 i;
	struct req	 req;
	char		*p;
	char		*path, *subpath, *suffix;

	memset(&req, 0, sizeof(struct req));

	if (NULL != (p = getenv("QUERY_STRING")))
		kval_parse(&req.fields, &req.fieldsz, p);

	suffix = subpath = path = NULL;

	req.media = MEDIA_HTML;
	req.page = PAGE__MAX;

	if (NULL == (path = getenv("PATH_INFO")) || '\0' == *path)
		req.page = PAGE_INDEX;
	if (NULL != path && '/' == *path && '\0' == *++path)
		req.page = PAGE_INDEX;

	if (NULL != path && NULL != (suffix = strrchr(path, '.')))
		if (NULL != suffix && NULL == strchr(suffix, '/'))
			*suffix++ = '\0';

	if (NULL != path && NULL != (subpath = strchr(path, '/')))
			*subpath++ = '\0';

	if (NULL != suffix && '\0' != *suffix)
		for (i = 0; i < (int)MEDIA__MAX; i++)
			if (0 == strcmp(medias[i], suffix)) {
				req.media = (enum media)i;
				break;
			}

	if (NULL != path && '\0' != *path)
		for (i = 0; i < (int)PAGE__MAX; i++) 
			if (0 == strcmp(pages[i], path)) {
				req.page = (enum page)i;
				break;
			}

	switch (req.page) {
	case (PAGE_INDEX):
		pg_index(&req, subpath);
		break;
	case (PAGE_SEARCH):
		pg_search(&req, subpath);
		break;
	default:
		/* Blah */
		break;
	}

	kval_free(req.fields, req.fieldsz);
	return(EXIT_SUCCESS);
}
