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
#include <unistd.h>
#include <signal.h>

#include "killprot.h"
#include "socket.h"
#include "config.h"
#include "util.h"

typedef struct {
	char *nick;
	time_t killtime;
} entry;

static entry killprot_tbl[CONFIG_KILLPROT_SIZE + 1] = { { 0 } };
static time_t killprot_schedtime = 0;

static void killprot_sigalrm(int sig);

static inline unsigned int killprot_hash(char *nick) {
	unsigned int hash = 0;
	while(*nick)
		hash = (hash << 5) - hash + *nick++;
	return hash & CONFIG_KILLPROT_SIZE;
}

static void killprot_do(entry *ent) {
	fprintf(socket_fp, "SVSNICK %s Ghost%d\n", ent->nick, rand()%9999999);
	fflush(socket_fp);
	free(ent->nick);
	ent->nick = 0;
}

static inline void killprot_alarm(int sec) {
	if(sec <= 0)
		killprot_sigalrm(SIGALRM);
	else
		alarm(sec);
}

static void killprot_sigalrm(int sig) {
	
	register unsigned int i;
	time_t now = time(0);

	killprot_schedtime = 0;

	for(i = 0; i < CONFIG_KILLPROT_SIZE; i++) {
		if(killprot_tbl[i].nick) {
			if(killprot_tbl[i].killtime <= now) {
				killprot_do(&killprot_tbl[i]);
			}
			else {
				if((killprot_tbl[i].killtime < killprot_schedtime)
						|| !killprot_schedtime)
					killprot_schedtime =
						killprot_tbl[i].killtime;
			}
		}
	}
	
	if(killprot_schedtime)
		killprot_alarm(killprot_schedtime - time(0));
		
}

void killprot_init() {
	signal(SIGALRM, killprot_sigalrm);
}

void killprot_sched(char *nick, time_t killtime) {
	
	entry *ent = &killprot_tbl[killprot_hash(nick)];
	
	if(ent->nick) {
		killprot_do(ent);
	}

	ent->nick = strdup(nick);
	ent->killtime = killtime;

	if((killtime < killprot_schedtime) || !killprot_schedtime) {
		killprot_alarm((killprot_schedtime = killtime) - time(0));
	}

}

void killprot_unsched(char *nick) {

	entry *ent = &killprot_tbl[killprot_hash(nick)];
	
	if(ent->nick && !strcmp(nick, ent->nick)) {
		free(ent->nick);
		ent->nick = 0;
	}
	
}
