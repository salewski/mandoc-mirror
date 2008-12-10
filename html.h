/* $Id$ */
/*
 * Copyright (c) 2008 Kristaps Dzonsons <kristaps@kth.se>
 *
 * Permission to use, copy, modify, and distribute this software for any
 * purpose with or without fee is hereby granted, provided that the
 * above copyright notice and this permission notice appear in all
 * copies.
 *
 * THE SOFTWARE IS PROVIDED "AS IS" AND THE AUTHOR DISCLAIMS ALL
 * WARRANTIES WITH REGARD TO THIS SOFTWARE INCLUDING ALL IMPLIED
 * WARRANTIES OF MERCHANTABILITY AND FITNESS. IN NO EVENT SHALL THE
 * AUTHOR BE LIABLE FOR ANY SPECIAL, DIRECT, INDIRECT, OR CONSEQUENTIAL
 * DAMAGES OR ANY DAMAGES WHATSOEVER RESULTING FROM LOSS OF USE, DATA OR
 * PROFITS, WHETHER IN AN ACTION OF CONTRACT, NEGLIGENCE OR OTHER
 * TORTIOUS ACTION, ARISING OUT OF OR IN CONNECTION WITH THE USE OR
 * PERFORMANCE OF THIS SOFTWARE.
 */
#ifndef HTML_H
#define HTML_H

#include "ml.h"

enum	html_tag {
	HTML_TAG_SPAN	= 0,
	HTML_TAG_HTML	= 1,
	HTML_TAG_HEAD	= 2,
	HTML_TAG_META	= 3,
	HTML_TAG_TITLE	= 4,
	HTML_TAG_STYLE	= 5,
	HTML_TAG_LINK	= 6,
	HTML_TAG_BODY	= 7,
	HTML_TAG_DIV	= 8,
	HTML_TAG_TABLE	= 9,
	HTML_TAG_TD	= 10,
	HTML_TAG_TR	= 11,
	HTML_TAG_OL	= 12,
	HTML_TAG_UL	= 13,
	HTML_TAG_LI	= 14,
	HTML_TAG_H1	= 15,
	HTML_TAG_H2	= 16,
	HTML_TAG_A	= 17
};

enum	html_attr {
	HTML_ATTR_CLASS = 0,
	HTML_ATTR_HTTP_EQUIV = 1,
	HTML_ATTR_CONTENT = 2,
	HTML_ATTR_NAME	= 3,
	HTML_ATTR_TYPE	= 4,
	HTML_ATTR_REL	= 5,
	HTML_ATTR_HREF	= 6,
	HTML_ATTR_WIDTH	= 7,
	HTML_ATTR_ALIGN	= 8,
	HTML_ATTR_VALIGN = 9,
	HTML_ATTR_NOWRAP = 10
};

enum	html_type {
	HTML_TYPE_4_01_STRICT = 0
};

struct	html_pair {
	enum html_attr	 attr;
	char		*val;
};

__BEGIN_DECLS

int		 html_typeput(struct md_mbuf *, 
			enum html_type, size_t *);
int		 html_commentput(struct md_mbuf *, 
			enum ml_scope, size_t *);
int		 html_tput(struct md_mbuf *, 
			enum ml_scope, enum html_tag, size_t *);
int		 html_aput(struct md_mbuf *, enum ml_scope, 
			enum html_tag, size_t *, 
			int, const struct html_pair *);
int		 html_stput(struct md_mbuf *,
			enum html_tag, size_t *);
int		 html_saput(struct md_mbuf *, enum html_tag, 
			size_t *, int, const struct html_pair *);

__END_DECLS

#endif /*!HTML_H*/
