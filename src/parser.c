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
#include <string.h>
#include "parser.h"
#include "trap.h"
#include "antiflood.h"
#include "util.h"

void parser_feedline(char *line) {
	
	char *cmd, *args, *sender=0;

	if(*line == ':') {
		sender = strtolower(strtok(line, " ") + 1);
		cmd = strtok(0, " \r\n");
	}
	else {
		cmd = strtok(line, " \r\n");
	}

	args = strtok(0, "\r\n");

	binsearch(trap_table, trap_table_sz, sender, cmd, args);
	
}

void parser_privmsg(char *sender, char *cmd, char *args) {
	
	int i;
	char *p, *dest = strtok(args, " ");
	args = strtok(0, "") + 1;
	
	if(antiflood_check(sender)) 
		return;

	if((p = strchr(dest, '@')))
		*p = 0;
	
	/* less entries, sequential search does well */
	for(i = 0; i <= trap_pvt_table_sz; i++) {
		if(!strcmp(dest, trap_pvt_table[i].cmd)) {
			trap_pvt_table[i].cb(sender, cmd, args);
			break;
		}
	}
	
}
