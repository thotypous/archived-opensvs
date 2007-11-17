/*
 *  Opensvs - A lightweight services for Hybrid ircd OFTC branch.
 *  Copyright (C) 2006 The Openbrasil Opensvs Team
 *
 *  This program is free software; you can redistribute it and/or modify
 *  it under the terms of the GNU General Public License as published by
 *  the Free Software Foundation; either version 2 of the License, or
 *  (at your option) any later version.
 *
 *  This program is distributed in the hope that it will be useful,
 *  but WITHOUT ANY WARRANTY; without even the implied warranty of
 *  MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 *  GNU General Public License for more details.
 *
 *  You should have received a copy of the GNU General Public License
 *  along with this program; if not, write to the Free Software
 *  Foundation, Inc., 59 Temple Place, Suite 330, Boston, MA  02111-1307
 *  USA
 *
 */

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <ctype.h>

#include "util.h"
#include "md5.h"

static char util_md5buf[33];

inline char *strtolower(char *s) {
	char *p = s;
	if(!p) return 0;
	for(;*p;p++) *p = tolower(*p);
	return s;
}

int binsearch(trap_entry *tbl, int f, char *sender, char *cmd, char *args) {
	int i = 0, mid, ret;
	while(i <= f) {
		mid = (i + f) / 2;
		ret = strcmp(cmd, tbl[mid].cmd);
		if(!ret) {
			tbl[mid].cb(sender, cmd, args);
			return 1;
		}
		else if(ret < 0)
			f = mid - 1;
		else
			i = mid + 1;
	}
	return 0;
}

char *md5(char *buf) {
	
	struct MD5Context ctx;
	unsigned char digest[16];
	char *p;
	int i;
	
	MD5Init(&ctx);
	MD5Update(&ctx, (unsigned char*)buf, strlen(buf));
	MD5Final(digest, &ctx);
	
	for(p = util_md5buf, i = 0; i < 16; p += 2, i++)
		sprintf(p, "%02x", digest[i]);

	return util_md5buf;
	
}

int getint_cb(void *arg, int argc, char **argv, char **colname) {
	*((int *)arg) = atoi(argv[0]);
	return 1;
}

