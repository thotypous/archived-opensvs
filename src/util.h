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

#ifndef _osvs_util_h
#define _osvs_util_h

#include "trap.h"

inline char *strtolower(char *s);
int binsearch(trap_entry *tbl, int f, char *sender, char *cmd, char *args);
char *md5(char *buf);
int getint_cb(void *arg, int argc, char **argv, char **colname);

#endif
