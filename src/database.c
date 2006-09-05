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

#include <assert.h>
#include <stdlib.h>
#include <stdarg.h>

#include "database.h"

sqlite3 *db;

static void database_close() {
	sqlite3_close(db);
}

void database_init() {
	assert(!sqlite3_open("opensvs.db", &db));
	atexit(database_close);
}

int dbexec(char *fmt, sqlite3_callback cb, void *cbarg, char **errmsg, ...) {
	
	va_list ap;
	char *sql;
	int res;
	
	va_start(ap, errmsg);
	sql = sqlite3_vmprintf(fmt, ap);
	res = sqlite3_exec(db, sql, cb, cbarg, errmsg);
	sqlite3_free(sql);
	va_end(ap);

	return res;

}

void database_onconnect() {
	dbexec("update nicks set identified=0;", 0, 0, 0);
}
