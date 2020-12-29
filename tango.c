/*
 * Copyright (c) 2020 Omar Polo <op@omarpolo.com>
 *
 * Permission to use, copy, modify, and distribute this software for any
 * purpose with or without fee is hereby granted, provided that the above
 * copyright notice and this permission notice appear in all copies.
 *
 * THE SOFTWARE IS PROVIDED "AS IS" AND THE AUTHOR DISCLAIMS ALL WARRANTIES
 * WITH REGARD TO THIS SOFTWARE INCLUDING ALL IMPLIED WARRANTIES OF
 * MERCHANTABILITY AND FITNESS. IN NO EVENT SHALL THE AUTHOR BE LIABLE FOR
 * ANY SPECIAL, DIRECT, INDIRECT, OR CONSEQUENTIAL DAMAGES OR ANY DAMAGES
 * WHATSOEVER RESULTING FROM LOSS OF USE, DATA OR PROFITS, WHETHER IN AN
 * ACTION OF CONTRACT, NEGLIGENCE OR OTHER TORTIOUS ACTION, ARISING OUT OF
 * OR IN CONNECTION WITH THE USE OR PERFORMANCE OF THIS SOFTWARE.
 */

#include <sys/types.h>

#include <err.h>
#include <regex.h>
#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>

#ifndef __OpenBSD__
# define pledge(a, b) 0
#endif

int
is_pre(char *line)
{
	return *line == '`'
		&& *(line+1) == '`'
		&& *(line+2) == '`';
}

int
process_file(regex_t *re, const char *path, FILE *out)
{
	FILE *in;
	char *line = NULL;
	size_t linesize = 0;
	ssize_t linelen;
	int in_pre = 0, tangle;

	if ((in = fopen(path, "r")) == NULL)
                err(1, "open: %s", path);

	tangle = re == NULL;
	while ((linelen = getline(&line, &linesize, in)) != -1) {
                if (is_pre(line)) {
			in_pre = !in_pre;
			if (tangle && !in_pre)
				fprintf(out, "\n");
			if (in_pre && re != NULL)
				tangle = regexec(re, line+3, 0, NULL, 0) != REG_NOMATCH;
		} else if (in_pre && tangle)
			fwrite(line, linelen, 1, out);
	}

	free(line);
	if (ferror(in))
		err(1, "getline");
	fclose(in);
	return 1;
}

__dead void
usage(const char *me)
{
	fprintf(stderr, "USAGE: %s [-r regexp] [-o out] [file...]\n", me);
	exit(1);
}

int
main(int argc, char **argv)
{
	FILE *out;
	int ch, has_reg, i;
	regex_t r;

	out = stdout;
	has_reg = 0;
	while ((ch = getopt(argc, argv, "o:r:")) != -1) {
		switch (ch) {
		case 'o':
			if (out != stdout)
				errx(1, "-o can only be provided once");
			if ((out = fopen(optarg, "r")) == NULL)
				err(1, "open: %s", optarg);
			break;

		case 'r':
			if (has_reg)
				errx(1, "-r can only be provided once");
			has_reg = 1;
			if (regcomp(&r, optarg, REG_ICASE | REG_NOSUB | REG_NEWLINE))
				errx(1, "cannot compile regex: %s", optarg);
			break;

		default:
			usage(*argv);
		}
	}
	argc -= optind;
	argv += optind;

	if (pledge("stdio rpath", NULL) == -1)
		err(1, "pledge");

	for (i = 0; i < argc; ++i)
		process_file(has_reg ? &r : NULL, argv[i], out);

	if (has_reg)
		regfree(&r);

	if (out != stdout)
		fclose(out);

	return 0;
}
