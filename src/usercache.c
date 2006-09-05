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

#include "chanserv.h"
#include "socket.h"
#include "config.h"
#include "util.h"

typedef struct {
	char *nick;
	char *userhost;
	chanent *queue;
} entry;

static entry usercache_tbl[CONFIG_USERCACHE_SIZE + 1] = { { 0 } };

static inline unsigned int usercache_hash(char *nick) {
	unsigned int hash = 0;
	while(*nick)
		hash = (hash << 5) - hash + *nick++;
	return hash & CONFIG_USERCACHE_SIZE;
}

static void usercache_freequeue(entry *ent) {
	chanent *p;
	for(p = ent->queue; p;) {
		p = p->next;
		free(p);
	}
	ent->queue = 0;
}

static void usercache_free(entry *ent) {
	if(ent->nick) {
		free(ent->nick);
		ent->nick = 0;
	}
	if(ent->userhost) {
		free(ent->userhost);
		ent->userhost = 0;
	}
	usercache_freequeue(ent);
}

void usercache_queue(char *nick, int channel) {
	
	entry *ent = &usercache_tbl[usercache_hash(nick)];
	
	if(!ent->nick || strcmp(nick, ent->nick)) {
		usercache_free(ent);
		ent->nick = strdup(nick);
		ent->userhost = 0;
		ent->queue = 0;
	}

	chanent *new = (chanent *)malloc(sizeof(chanent));
	new->next = ent->queue;
	new->channel = channel;
	ent->queue = new;

	if(ent->userhost) {
		chanserv_akickprocess(nick, ent->userhost, ent->queue);
		usercache_freequeue(ent);
	}
	else {
		fprintf(socket_fp, "USERHOST %s\n", nick);
		fflush(socket_fp);
	}
	
}

void usercache_add(char *nick, char *userhost) {
	
	entry *ent = &usercache_tbl[usercache_hash(nick)];
	
	if(!ent->nick || strcmp(nick, ent->nick)) {
		usercache_free(ent);
		ent->nick = strdup(nick);
		ent->userhost = 0;
		ent->queue = 0;
	}
	
	ent->userhost = strdup(userhost);

	if(ent->queue) {
		chanserv_akickprocess(nick, userhost, ent->queue);
		usercache_freequeue(ent);
	}
	
}

void usercache_del(char *nick) {
	
	entry *ent = &usercache_tbl[usercache_hash(nick)];

	if(ent->nick && strcmp(nick, ent->nick)) {
		usercache_free(ent);
	}
	
}

void usercache_302(char *sender, char *cmd, char *args) {
	char *nick, *userhost;
	args = strchr(args, ':');
	if(!args || !args[1])
		return;
	nick = strtok(strtolower(args + 1), "=");
	userhost = strtok(0, " ") + 1;
	if(nick && userhost)
		usercache_add(nick, userhost);
}
