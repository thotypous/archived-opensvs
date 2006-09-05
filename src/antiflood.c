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
#include <time.h>

#include "antiflood.h"
#include "socket.h"
#include "config.h"

typedef struct {
	time_t timer;
	unsigned int counter;
} entry;

static entry antiflood_tbl[CONFIG_ANTIFLOOD_SIZE + 1] = { { 0 } };

static inline unsigned int antiflood_hash(char *nick) {
	unsigned int hash = 0;
	while(*nick)
		hash = (hash << 5) - hash + *nick++;
	return hash & CONFIG_ANTIFLOOD_SIZE;
}

int antiflood_check(char *nick) {
	
	entry *ent = &antiflood_tbl[antiflood_hash(nick)];
	time_t now = time(0);

	if((now - ent->timer) > CONFIG_ANTIFLOOD_PERSEC) {
		ent->timer = now;
		ent->counter = 1;
	}
	else if(++ent->counter > CONFIG_ANTIFLOOD_LIMIT) {
		
		fprintf(socket_fp, ":NickServ NOTICE %s :Você será ignorado "
				"por %d segundos.\n",
				nick, CONFIG_ANTIFLOOD_IGNORETIME);
		
		fflush(socket_fp);
		
		ent->timer = CONFIG_ANTIFLOOD_IGNORETIME + now;
		ent->counter = CONFIG_ANTIFLOOD_LIMIT;
		
		return 1;

	}

	return 0;
	
}
