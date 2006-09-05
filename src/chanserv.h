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

#ifndef _osvs_chanserv_h
#define _osvs_chanserv_h

#include "usercache.h"

void chanserv_pvt(char *sender, char *cmd, char *args);
void chanserv_sjoin(char *sender, char *cmd, char *args);
void chanserv_topic(char *sender, char *cmd, char *args);
void chanserv_mode(char *sender, char *cmd, char *args);
void chanserv_akickprocess(char *nick, char *userhost, chanent *channels);

#endif
