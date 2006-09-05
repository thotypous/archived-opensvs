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

#include "trap.h"
#include "socket.h"
#include "parser.h"
#include "usercache.h"
#include "nickserv.h"
#include "chanserv.h"

static void trap_ping(char *sender, char *cmd, char *args) {
	fprintf(socket_fp, "PONG %s\n", args);
	fflush(socket_fp);
}

/* must be in alphabetical order */
trap_entry trap_table[] = {
	{ "302",     usercache_302  },
	{ "MODE",    chanserv_mode  },
	{ "NICK",    nickserv_nick  },
	{ "PING",    trap_ping      },
	{ "PRIVMSG", parser_privmsg },
	{ "QUIT",    nickserv_quit  },
	{ "SJOIN",   chanserv_sjoin },
	{ "TOPIC",   chanserv_topic },
};

int trap_table_sz = sizeof(trap_table)/sizeof(trap_entry) - 1;

trap_entry trap_pvt_table[] = {
	{ "ChanServ", chanserv_pvt },
	{ "NickServ", nickserv_pvt },
};

int trap_pvt_table_sz = sizeof(trap_pvt_table)/sizeof(trap_entry) - 1;

