/*
 * UNG's Not GNU
 *
 * Copyright (c) 2011-2019, Jakob Kaivo <jkk@ung.org>
 *
 * Permission is hereby granted, free of charge, to any person obtaining a copy
 * of this software and associated documentation files (the "Software"), to deal
 * in the Software without restriction, including without limitation the rights
 * to use, copy, modify, merge, publish, distribute, sublicense, and/or sell
 * copies of the Software, and to permit persons to whom the Software is
 * furnished to do so, subject to the following conditions:
 *
 * The above copyright notice and this permission notice shall be included in
 * all copies or substantial portions of the Software.
 *
 * THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR
 * IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY,
 * FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT. IN NO EVENT SHALL THE
 * AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER
 * LIABILITY, WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING FROM,
 * OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS IN THE
 * SOFTWARE.
 */

#define _XOPEN_SOURCE 700
#include <errno.h>
#include <ftw.h>
#include <grp.h>
#include <inttypes.h>
#include <limits.h>
#include <locale.h>
#include <pwd.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <unistd.h>

#ifndef OPEN_MAX
#define OPEN_MAX _POSIX_OPEN_MAX
#endif

/* TODO: replace these with NFTW_* flags */
static enum { NOFOLLOW, FOLLOWARGV, FOLLOWALL } links = FOLLOWARGV;
static uid_t newowner = -1;
static gid_t newgroup = -1;
static int retval = 0;

static int ch_own(const char *p, const struct stat *st, int typeflag, struct FTW *f)
{
	(void)typeflag; (void)f;

	if (st == NULL) {
		fprintf(stderr, "chown: %s: unknown error\n", p);
		retval = 1;

	} else if (S_ISLNK(st->st_mode)) {
		/* TODO: based on value of 'links' */

	} else {
		if (chown(p, newowner, newgroup) != 0) {
			fprintf(stderr, "chown: %s: %s\n", p, strerror(errno));
			retval = 1;
		}
	}

	return 0;
}

static void parse_owner(char *owner)
{
	char *colon = strchr(owner, ':');
	if (colon) {
		*colon = '\0';
		char *grpname = colon + 1;
		struct group *grp = getgrnam(grpname);
		if (!grp) {
			char *end;
			newgroup = (gid_t)strtoumax(grpname, &end, 0);
			if (end && *end != '\0') {
				printf("chown: couldn't find group '%s'\n",
					grpname);
				exit(0);
			}
		} else {
			newgroup = grp->gr_gid;
		}
	}

	struct passwd *pwd = getpwnam(owner);
	if (!pwd) {
		char *end;
		newowner = (uid_t)strtoumax(owner, &end, 0);
		if (end && *end != '\0') {
			printf("chown: couldn't find user '%s'\n", owner);
			exit(0);
		}
	} else {
		newowner = pwd->pw_uid;
	}
}

int main(int argc, char *argv[])
{
	int recursive = 0;

	setlocale(LC_ALL, "");

	int c;
	while ((c = getopt(argc, argv, "hHLPR")) != -1) {
		switch (c) {
		case 'h':
			if (recursive) {
				return 1;
			}

			recursive = -1;
			links = NOFOLLOW;
			break;

		case 'H':
			links = FOLLOWARGV;
			break;

		case 'L':
			links = FOLLOWALL;
			break;

		case 'P':
			links = NOFOLLOW;
			break;

		case 'R':
			if (recursive == -1) {
				return 1;
			}

			recursive = 1;
			break;

		default:
			return 1;
		}
	}

	if (optind >= argc - 1) {
		fprintf(stderr, "chown: missing operand(s)\n");
		return 1;
	}

	parse_owner(argv[optind++]);

	do {
		char *path = argv[optind];

		struct stat st;
		if (stat(path, &st) != 0) {
			fprintf(stderr, "chown: %s: %s\n", path,
				strerror(errno));
			retval = 1;
			continue;
		}

		if (S_ISDIR(st.st_mode) && recursive) {
			nftw(path, ch_own, OPEN_MAX, FTW_DEPTH | FTW_PHYS);
		} else {
			ch_own(path, &st, 0, NULL);
		}
		
	} while (++optind < argc);

	return 0;
}
