/*
 * flvtool.c
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
#include <stdlib.h>
#include <unistd.h>
#include <fcntl.h>
#include <string.h>

#include <sys/types.h>
#include <sys/stat.h>

#include "libxa1dump.h"
#include "flv.h"

typedef struct refbuf refbuf_t;
struct refbuf {
	char		*data;
	ssize_t		len;
};

refbuf_t *refbuf_new (size_t size) {
	refbuf_t *ret = malloc(sizeof(refbuf_t));

	if (!ret)
		return NULL;

	ret->data = malloc(size);
	if (!ret->data) {
		free (ret);
		return NULL;
	}
	ret->len = size;

	return ret;
}

refbuf_t *refbuf_realloc (refbuf_t *refbuf, size_t newsize) {
	char *newdata;

	dprintf("reallocing to %d\n", newsize);

	if (!refbuf)
		return refbuf_new (newsize);

	newdata = realloc(refbuf->data, newsize);
	if (!newdata)
		return NULL;

	refbuf->len = newsize;
	refbuf->data = newdata;

	return refbuf;
}

void refbuf_destroy (refbuf_t *refbuf) {
	free(refbuf->data);
	free(refbuf);
}

void xa1d (char *b) {
	printf ("b[%d]\n", b[0]);
}
int main (int argc, char *argv[]) {
	int 		fd, bytes, last = 0;
	ssize_t		len;
	size_t		pos = 0, readc = 0, tot = 0, fill = 0, off = 0;
	struct stat	statb;

	refbuf_t	*refbuf;
	parsefile_t	*f;

	if (argc < 2) {
		perror ("Need more arguments");
		return 0;
	}

	fd = open (argv[1], O_RDONLY);
	if (fd == -1) {
		perror (argv[1]);
		return 0;
	}

	if (fstat (fd, &statb) == -1) {
		perror (argv[1]);
		return 0;
	}

	f = &flv_file;

	refbuf = refbuf_new (4096);

	if (!f || !refbuf) {
		perror ("memory issue");
		return 0;
	}

	while (1) {
		dprintf ("trying to read at offset: %d, size %d\n",
			fill, refbuf->len - fill);
		bytes = read (fd, refbuf->data + fill, refbuf->len - fill); /* not really ideal */
		if (bytes <= 0) {
			printf ("couldn't read at offset %d/%d, size %d\n",
				readc + fill, statb.st_size, refbuf->len - fill);
			return 0;
		}

		fill += bytes;

		char *endptr;
		while (fill > pos) {
			dprintf ("parsing at %d, offset %d, readc %d\n", tot, pos, readc);
			bytes = pm_parse_file (refbuf->data + pos, fill - pos, f, &endptr);
			if (bytes == 0 || !endptr)
				goto err_parse;

			tot += endptr - refbuf->data - pos;
			pos = endptr - refbuf->data;
			dprintf ("got endptr, pos %d\n", pos);

			if (bytes < 0) {
				if (-bytes >= refbuf->len) {
					printf ("refbuf too small, need %d more, reallocing: %d, will copy from offset %d, size %d, fill %d\n",
						-bytes - refbuf->len, pos - bytes, pos, fill - pos, fill);

					refbuf_t *tmp = refbuf_new(pos - bytes);
					memcpy (tmp->data, refbuf->data + pos, fill - pos);

					refbuf_destroy (refbuf);
					refbuf = tmp;
				} else {
					dprintf ("not enough data, fetching more, keeping data of size %d at offset %d, total %d\n",
						fill - pos, pos, refbuf->len);
					memcpy (refbuf->data, refbuf->data + pos, fill - pos);
				}
				readc += pos;
				fill -= pos;
				pos = 0;
				break;
			}
			pm_file_describe_last (f);
			printf ("\n");
			dprintf ("PARSING OK left to read: %d\n\n", refbuf->len - fill);
			if (pos == fill) {/* we filled all we had, and have nothing stray */
				pos = fill = 0;
			}
		}
	}

 err_parse:
	printf ("ERROR: [%d/%d] Couldn't parse (bytes = %d, readc = %d)\n", tot, statb.st_size, bytes, readc);
	printf("[%x]", tot - 8);
	xa1_dump_size (refbuf->data + tot - readc -8, 8);
	printf("[%x]", tot);
	xa1_dump_size (refbuf->data + tot - readc, 8);
	printf("[%x]", tot + 8);
	xa1_dump_size (refbuf->data + tot - readc + 8, 8);
	return 0; /* need to free */

 err_readc:
	printf ("ERROR while reading at %d/%d, read %d needed %d %m\n",
		tot, statb.st_size, readc, refbuf->len);
	return 0;
}
