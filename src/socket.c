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
#include <unistd.h>
#include <fcntl.h>
#include <sys/types.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <arpa/inet.h>
#include <netdb.h>
#include <assert.h>

#include "socket.h"
#include "parser.h"
#include "fakeuser.h"
#include "database.h"
#include "config.h"

static char socket_buffer[CONFIG_BUFFER_LINEMAX];
static FILE *socket_fr = 0;
FILE *socket_fp;

static void socket_connect() {
	
	struct sockaddr_in addr;
	struct hostent *host;
	int fd;
	
	if(socket_fr) {
		fclose(socket_fp);
		fclose(socket_fr);
	}
	
	assert((fd = socket(AF_INET, SOCK_STREAM, 0)) >= 0);
	
	assert(host = gethostbyname(CONFIG_CONNECT_HOST));
	
	memset((void *)&addr, 0, sizeof(addr));
	addr.sin_family = AF_INET;
	addr.sin_port = htons(CONFIG_CONNECT_PORT);
	memcpy((void *)&addr.sin_addr, host->h_addr, host->h_length);

	while(connect(fd, (struct sockaddr *)&addr, sizeof(addr)) < 0)
		sleep(CONFIG_CONNECT_RETRY_TIME);

	assert(socket_fp = fdopen(fd, "w"));
	assert(socket_fr = fdopen(fd, "r"));

	fprintf(socket_fp, "PASS " CONFIG_CONNECT_PASSWORD " :TS\n"
		"CAPAB TS5 EX IE HOPS HUB AOPS\n"
		"SERVER " CONFIG_SVS_SERVER " 1 :" CONFIG_SVS_DESCRIPTION "\n");
	fakeuser_init();
	database_onconnect();
	fflush(socket_fp);
	
}

void socket_mainloop() {
	for(;;) {
		socket_connect();
		for(;fgets(socket_buffer, CONFIG_BUFFER_LINEMAX, socket_fr);)
			parser_feedline(socket_buffer);
		sleep(CONFIG_CONNECT_RETRY_TIME);
	}
}
