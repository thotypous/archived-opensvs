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
#include <unistd.h>
#include <assert.h>
#include <time.h>

#include "socket.h"
#include "database.h"
#include "killprot.h"

int main() {

	int pid;
	
	assert((pid = fork()) >= 0);
	if(pid) {
		printf("Opensvs started at pid %d.\n", pid);
		return 0;
	}
	
	assert(setsid() >= 0);
	close(0);
	close(1);
	close(2);

	srand(time(0));
	
	database_init();
	killprot_init();
	socket_mainloop();
	
	return 0;
	
}

