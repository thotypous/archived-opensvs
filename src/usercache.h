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

#ifndef _osvs_usercache
#define _osvs_usercache

typedef struct __chanent {
	unsigned int channel;
	struct __chanent *next;
} chanent;

void usercache_queue(char *nick, int channel);
void usercache_add(char *nick, char *userhost);
void usercache_del(char *nick);
void usercache_302(char *sender, char *cmd, char *args);

#endif
