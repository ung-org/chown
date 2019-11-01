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
#include <ftw.h>
#include <grp.h>
#include <limits.h>
#include <pwd.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <unistd.h>

#define NOFOLLOW	0
#define FOLLOWARGV	1
#define FOLLOWALL	2

static int recursive = 0;
static int links = FOLLOWARGV;
static uid_t newowner = -1;
static gid_t newgroup = -1;

int _chown(const char *);

int nftw_chown(const char *p, const struct stat *sb, int typeflag,
	       struct FTW *f)
{
	(void)sb; (void)typeflag; (void)f;
	return _chown(p);
}

int _chown(const char *p)
{
	struct stat st;
	if (lstat(p, &st) != 0) {
		perror(p);
		return 1;
	}
	// FIXME: about symlinks....
	// if (links == NOFOLLOW
	// lchown (p, newowner, newgroup) 
	if (chown(p, newowner, newgroup) != 0) {
		perror(p);
	}
	return 0;
}

static void chown_parse(const char *ug)
{
	char *colon = strchr(ug, ':');
	if (colon == NULL) {
		struct passwd *pwd = getpwnam(ug);
		if (pwd == NULL)
			exit(1);
		newowner = pwd->pw_uid;
	} else {
		struct passwd *pwd;
		struct group *grp;
		char user[NAME_MAX];
		strcpy(user, ug);
		user[strlen(ug) - strlen(colon)] = '\0';
		pwd = getpwnam(user);
		grp = getgrnam(&colon[1]);
		if (pwd == NULL || grp == NULL) {
			exit(1);
		}
		newowner = pwd->pw_uid;
		newgroup = grp->gr_gid;
	}
}

int do_chown(char *path, int recurse, int howlinks, uid_t uid, gid_t gid)
{
	recursive = recurse;
	links = howlinks;
	newowner = uid;
	newgroup = gid;
	// FIXME: This is where to recurse and handle the difference betwixt links types
	return _chown(path);
}

int main(int argc, char *argv[])
{
	int c;
	while ((c = getopt(argc, argv, ":hHLPR")) != -1) {
		switch (c) {
		case 'h':
			if (recursive)
				return 1;
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
			if (recursive == -1)
				return 1;
			recursive = 1;
			break;
		default:
			return 1;
		}
	}

	if (optind >= argc - 1) {
		return 1;
	}

	chown_parse(argv[optind++]);

	while (optind < argc) {
		do_chown(argv[optind], recursive, links, newowner, newgroup);
		optind++;
	}

	return 0;
}
