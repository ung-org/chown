/*
 * UNG's Not GNU
 * 
 * Copyright (c) 2011, Jakob Kaivo <jakob@kaivo.net>
 * 
 * Permission to use, copy, modify, and/or distribute this software for any
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
 *
 */

#include <stdio.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <getopt.h>
#include <grp.h>
#include <pwd.h>
#include <limits.h>
#include <ftw.h>
#include <string.h>
#include <stdlib.h>

const char *chown_desc = "change the file ownership";
const char *chown_inv  = "chown [-h] owner[:group] file...\nchown -R [-H|-L|-P] owner[:group] file...";

#define NOFOLLOW	0
#define FOLLOWARGV	1
#define FOLLOWALL	2

static int recursive = 0;
static int links = FOLLOWARGV;
static uid_t newowner = -1;
static gid_t newgroup = -1;

int _chown (const char*);

int nftw_chown (const char *p, const struct stat *sb, int typeflag, struct FTW *f)
{
  _chown (p);
}

int _chown (const char *p)
{
  struct stat st;
  if (lstat (p, &st) != 0) {
    perror (p);
    return 1;
  }

  // FIXME: about symlinks....
  // if (links == NOFOLLOW
  // lchown (p, newowner, newgroup) 
  if (chown (p, newowner, newgroup) != 0)
    perror (p);
  return 0;
}

static void chown_parse (const char *ug)
{
  char *colon = strchr (ug, ':');
  if (colon == NULL) {
    struct passwd *pwd = getpwnam (ug);
    if (pwd == NULL)
      exit (1);
    newowner = pwd->pw_uid;
  } else {
    struct passwd *pwd;
    struct group *grp;
    char user[NAME_MAX];
    strcpy (user, ug);
    user[strlen(ug)-strlen(colon)] = '\0';
    pwd = getpwnam (user);
    grp = getgrnam (&colon[1]);
    if (pwd == NULL || grp == NULL)
      exit (1);
    newowner = pwd->pw_uid;
    newgroup = grp->gr_gid;
  }
}

int do_chown (char *path, int recurse, int howlinks, uid_t uid, gid_t gid)
{
  recursive = recurse;
  links = howlinks;
  newowner = uid;
  newgroup = gid;
  // FIXME: This is where to recurse and handle the difference betwixt links types
  _chown (path);
}

int
main (int argc, char **argv)
{
  int c;

  while ((c = getopt (argc, argv, ":hHLPR")) != -1) {
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
  
  if (optind >= argc - 1)
    return 1;

  chown_parse (argv[optind++]);

  while (optind < argc) {
    do_chown (argv[optind], recursive, links, newowner, newgroup);
    optind++;
  }

  return 0;
}
