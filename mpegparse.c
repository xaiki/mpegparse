/*
 * mpegparse.c
 * This file is part of mpegparse.
 *
 * Copyright (C) 2011 - Niv Sardi <xaiki@debian.org>
 *
 * This library is free software; you can redistribute it and/or
 * modify it under the terms of the GNU General Public
 * License as published by the Free Software Foundation.
 * version 2.0 of the License (strictly).
 *
 * This library is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU
 * General Public License for more details.
 *
 * You should have received a copy of the GNU General Public
 * License along with this library; if not, write to the Free Software
 * Foundation, Inc., 51 Franklin Street, Fifth Floor, Boston, MA  02110-1301  USA
 */

#include <stdio.h>
#include <fcntl.h>
#include <stdlib.h>
#include <string.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <endian.h>
#include "mpegparse.h"

#define MIN(X,Y) ((X)<(Y))?(X):(Y)
#define MAX(X,Y) ((X)>(Y))?(X):(Y)

const char _PM_EMBEDDED[]   = "Embeded Parseme";
const char _PM_EMBEDDED_A[] = "Embeded Parseme Array";

int _check_zero (parseme_t p[], int i, char *buf, size_t buflen) {
	dprintf("checking with %s\n", __func__);
	if (p[i].data == 0)
		return 1;

	PM_CHECK_FAIL;
	return 0;
}

int _check_range (parseme_t p[], int i, char *buf, size_t buflen) {
	dprintf("checking with %s\n", __func__);
	uint16_t a = UINT32GET12(p[i].def);
	uint16_t b = UINT32GET22(p[i].def);
	pm_data_type d = p[i].data;

	if (a > b)
		SWAP(a,b);
	if ((d < a) || (d > b)) {
		PM_CHECK_FAIL;
		return 0;
	}
	return 1;
}

void mpegparse_destroy(parseme_t *p) {
	free (p);
}

parseme_t *_mpegparse_new (parseme_t *p, size_t nmemb) {
	dprintf("new: %p, %d\n", p, nmemb);
	parseme_t *ret = calloc (nmemb, sizeof(*ret));
	if (!ret)
		return NULL;

	memcpy (ret, p, nmemb*sizeof(*ret));
	//	dprintf("returning %p\n", ret);
	return ret;
}

int pm_get_key(parseme_t p[], char *key, pm_data_type *value) {
	int i;

	for ( i = 0; p[i].name; i++ ) {
		dprintf("trying for '%s'.\n", p[i].name);
		if (p[i].name == _PM_EMBEDDED) {
			dprintf("trying _PM_EMBEDDED\n");
			if (pm_get_key(_PM_EMBEDDED_GETPM(p[i]), key, value))
				return 1;
		}

		if (strcmp (p[i].name, key) == 0) {
			dprintf("ok for '%s'.\n", p[i].name);
			*value = p[i].data;
			return 1;
		}
	}
	return 0;
}

int pm_parse_describe (parseme_t *p) {
	int i;

	for (i = 0; p[i].name; i++) {
		if (p[i].name == _PM_EMBEDDED) {
			parseme_t *e = _PM_EMBEDDED_GETPM(p[i]);
			if (e) {
				printf ("[%s]\n", pm_get_name(e));
				pm_parse_describe (e);
			} else {
				printf("[%d]%s'%s:%u' \t-> 'Stalling Embed' \n",
				       i, p[i].def?"*":p[i].check?"=":" ", p[i].name, p[i].size);
			}
		} else {
			if (p[i].size) {
				printf ("[%d]%s'%s:%u' \t-> '%d' \n",
					i, p[i].def?"*":p[i].check?"=":" ", p[i].name, p[i].size, p[i].data);
			}
		}
	}

}

int pm_file_describe_last (parsefile_t *f) {
	struct parseblock *b = f->b;
	struct parseme *p = b[f->last].p;

	printf ("0x%08x [ %s [#%d]]\n", f->pos, pm_get_name(p), b[f->last].seen);
	return pm_parse_describe (p);
}

int pm_parse_file (char *buf, size_t size, parsefile_t *f, char **endptr) {
	int ret = 0;
	int adv;
	int seen = 0;
	int i;

	struct parseblock *b = f->b;

	for (i=0; b[i].p; i++) {
		if (b[i].flags & PB_COMPLETE)
			continue;
		dprintf ("parsing at offset %d, starts %s\n", ret, buf + ret);
		adv = pm_parse (buf + ret, size - ret, b[i].p, endptr);
		dprintf ("got: %d\n", adv);
		if (adv == 0)
			if (b[i].flags & PB_OPTIONAL)
				continue;
		if (adv <= 0) {
			return adv;
		}
		seen++;
		b[i].seen ++;
		f->pos += adv;
		ret += adv;
		if (b[i].flags & PB_UNIQUE) {
			b[i].flags |= PB_COMPLETE;
			if (b[i].seen > 1)
				printf ("Warning, we got 2 %s when it's supposed to be unique", pm_get_name (b[i].p));
		}
		break;
	}
			dprintf("Returned: %d\n", p[i].size);
	f->last = i;
	return ret;
}

int pm_parse (char *b, size_t bufsiz, parseme_t p[], char **endptr) {
	int i;
	uint8_t offset = 0;
	char *buf = b;

	int pad = 0;
	int tot = 0; /* XXX(xaiki): hack */
	ssize_t size = 0;

	size_t align =  sizeof(buf[0])*8;

	const char *name = "p";

	if (endptr)
		*endptr = b;

	if (p[0].size == 0 && p[0].def == 0 && p[0].check == NULL) {
		name = p[0].name;
		p++;
	}

	/* XXX(xaiki): hack */
	for ( i = 0; p[i].name; i++ ) {
		dprintf("IN %s[%d](%s).size = %d.\n", name, i, p[i].name, p[i].size/8);
		if (p[i].name != _PM_EMBEDDED)
			tot += p[i].size/8;	/* XXX(xaiki): hack */
		p[i].data = 0;
	}

	/* XXX(xaiki): hack */
	if (bufsiz < tot) {
		dprintf("invalid bufsiz = %u, need at least %u .\n", bufsiz, tot);
		return -tot; /* we don't know how much we'll need to parse this, but at least tot */
	}

	for ( i = 0; p[i].name; i++) {
		dprintf("%s[%d] trying on %s (%u), offset %u, left %u: %x\n",
			name, i,  p[i].name, size?size:p[i].size, (buf - b), bufsiz - (buf - b), buf[0]);
		if ((p[i].name == _PM_EMBEDDED) && p[i].check) {
			if (p[i].size && p[i].def == 1) {
				dprintf ("Already parsed, skipping\n");
				continue;
			}
			int ret = pm_parse (buf, bufsiz - (buf - b), _PM_EMBEDDED_GETPM(p[i]), NULL);
			dprintf("Returned: %d\n", ret);
			if (ret <= 0) {
				int j; /* reset pm_embedded */
				for (j = 0; p[j].name; j++) {
					if (p[j].name == _PM_EMBEDDED && p[j].check) {
						if (p[j].size && p[j].def == 1) {
							p[j].size = 0;
						}
					}
				}
				return ret + (buf - b);
			} else {
				buf += ret;
				p[i].size = ret*8;
				//if (p[i+1].name)
				//p[i+1].size -= p[i].size; /* humm, well, ok, why not ... */
			}

			continue;
		}

		if (p[i].size == 0) {
			dprintf("size == 0 ? probably some unhandled stuff, skip for now.\n");
			continue;
		} else if (p[i].size > sizeof(pm_data_type)*8) {
			tot = p[i].size/8;
			p[i].size = 0;

			if (offset) {
				dprintf("No offset: Not Implemeted.\n");
				goto err_print;
			}
			if (p[i].def) {
				dprintf("def '%u' checking: Not Implemeted.\n", p[i].def);
				goto err_print;
			}

			if (p[i].check && ! p[i].check(p, i, buf, bufsiz - (buf - b)))
				goto err_print;

			p[i].data = -1;

			if (tot > bufsiz - (buf - b)) {
				dprintf("Buf too small !!! %u left, and I still want to put %d ! total packet size is %d\n",
					bufsiz - (buf - b), tot, buf - b + tot);
#ifdef NDEBUG
				int j;
				for ( j = 0; p[j].name; j++)
					if (p[j].size <= sizeof(pm_data_type)*8)
						dprintf ("%s'%s:%u'->'%u'",  p[j].def?"*":p[j].check?"=":" ", p[j].name, p[j].size, p[j].data);
				dprintf("\n");
#endif
				return b - buf -tot;
			}
			buf += tot;
			continue;
		}

		if (size < 1)
		retry:
			size = p[i].size;

		if (offset + size < align)
			pad = align - (offset + size);
		else
			pad = 0;

		p[i].data |= *buf>>pad & ~(~0<<(align - (offset + pad)));


		if (offset + size > align) { /* we overflow */
			p[i].data = p[i].data<<(MIN(size, align));
			size -= (align - (offset + pad));
			offset = 0;
			i--;
			if (!offset) buf++;
			continue;
		}
		if (p[i].check) {
			int t = p[i].size;
			if (! p[i].check(p, i, buf, bufsiz - (buf - b))) {
				printf ("at 0x%08x, offset %ld:%d, Field '%s':'%d' couldn't be checked by '%p'.\n",
					(unsigned int) (buf - b)*4, buf - b, offset, p[i].name, p[i].data, p[i].check);
				goto err;
			}
			if (t != p[i].size) /* updated, retry */
				goto retry;
		} else if (p[i].def && p[i].data != p[i].def) {
			printf ("at 0x%08x, offset %ld:%d, Field '%s' should be '%02x' but is '%02x'.\n",
				(unsigned int) (buf - b)*4, buf - b, offset, p[i].name, p[i].def, p[i].data);

			goto err;
		}

		size -= (align - (offset + pad));
		offset = pad?align-pad:0;
		/* HACK */
		if (p[i+1].data != 0) {
			dprintf("WARNING !!!!! DATA NOT 0 !!!!\n");
			p[i+1].data = 0;
		}
		dprintf("Successfully parsed %s = %d\n", p[i].name, p[i].data);
		if (!offset) buf++;
	}

	dprintf ("parsing succeded ! %u left !\n", bufsiz - (buf - b));

#ifdef DEBUG
	printf ("[%s]\n", name);
	pm_parse_describe (p);
#endif

	if (endptr)
		*endptr = buf;

	return buf - b;

 err_print:
	printf ("at 0x%08x, offset %ld:%d, Field '%s' should be '%02x' but is '%02x'.\n",
		(unsigned int)(buf - b)*4, buf - b, offset, p[i].name, p[i].def, p[i].data);
 err:
	return 0;
}
