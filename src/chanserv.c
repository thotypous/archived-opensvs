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

#include <string.h>
#include <stdio.h>
#include <stdlib.h>
#include <time.h>
#include <ctype.h>

#include "chanserv.h"
#include "nickserv.h"
#include "database.h"
#include "usercache.h"
#include "socket.h"
#include "trap.h"
#include "util.h"
#include "config.h"

#define cs_msg(x) fprintf(socket_fp, ":ChanServ NOTICE %s :" x "\n", sender)

static int chanserv_get_level(char *nick, char *channel,
		int *user_id, int *channel_id, int *level) {

	*user_id = *channel_id = *level = -1;

	dbexec("select user from nicks where name='%q' and identified=1;",
			getint_cb, user_id, 0, nick);

	if(*user_id == -1)
		return 0;


	dbexec("select id from channels where name='%q';",
			getint_cb, channel_id, 0, channel);

	if(*channel_id == -1)
		return 0;

	dbexec("select level from access where user=%d and channel=%d;",
			getint_cb, level, 0, *user_id, *channel_id);

	if(*level == -1)
		return 0;

	return 1;
	
}

static int chanserv_access_cb(void *a, int argc, char **argv, char **colname) {
	fprintf(socket_fp, ":ChanServ NOTICE %s :"
		"\2%-5s\2 %s\n",
		(char *)a, argv[0], argv[1]);
	return 0;
}

void chanserv_access(char *sender, char *cmd, char *args) {
	
	char *channel, *param;
	int user_id, channel_id, level;
	
	strtolower(args);
	if(!(channel = strtok(args, " ")) || !(cmd = strtok(0, " "))) {
		cs_msg("Sintaxe incorreta.");
		return;
	}
	if(!(param = strtok(0, "")))
		param = "";
	
	if(!chanserv_get_level(sender, channel, &user_id, &channel_id, &level)){
		cs_msg("Acesso negado.");
		return;
	}

	if(!strcmp(cmd, "list")) {

		fprintf(socket_fp, ":ChanServ NOTICE %s :"
			"\2*** Access de %s ***\2\n", sender, channel);

		dbexec(
			"select access.level,nicks.name from access"
			" left join nicks on nicks.user=access.user"
			" where access.channel=%d group by access.user"
			" order by access.level desc;",
			chanserv_access_cb, sender, 0,
			channel_id
		      );

		cs_msg("\2*** Fim da access ***\2");
		
	}
	else {
		
		if(level < 3) {
			cs_msg("Acesso negado.");
			return;
		}
		
		if(!strcmp(cmd, "add")) {
			
			char *nick = strtok(param, " ");
			char *slev = strtok(0, " ");
			int cur_lev = 0, lev, nick_id = -1;

			if(!nick || !slev || !*nick || !*slev) {
				cs_msg("Sintaxe incorreta.");
				return;
			}

			if((lev = atoi(slev)) < 0) {
				cs_msg("Valor inválido de nível de acesso.");
				return;
			}

			if(lev > level) {
				cs_msg("Você não pode dar a alguém um nível de"
					" acesso maior que o seu.");
				return;
			}

			dbexec("select user from nicks where name='%q';",
				getint_cb, &nick_id, 0, nick);

			if(nick_id == -1) {
				cs_msg("Nick inválido.");
				return;
			}
			
			if(nick_id == user_id) {
				cs_msg("Você não pode alterar seu próprio nível"
					" de acesso.");
				return;
			}

			dbexec("select level from access where "
				"channel=%d and user=%d;",
				getint_cb, &cur_lev, 0,
				channel_id, nick_id);

			if(level <= cur_lev) {
				cs_msg("Você só pode alterar o nível de acesso "
					"de alguém com o nível menor que "
					"o seu.");
				return;
			}
				
			dbexec("insert into access (channel,user,level)"
				"values (%d,%d,%d);",
				0, 0, 0,
				channel_id, nick_id, lev);

			if(sqlite3_changes(db)) {
				cs_msg("Nível adicionado na lista de acesso.");
			}
			
		}
		else if(!strcmp(cmd, "del")) {

			if(!param || !*param)
				return;

			dbexec(
				"delete from access where channel=%d"
				" and user=(select user from nicks"
				"  where name='%q') and (level<%d"
				"   or user=%d);",
				0, 0, 0,
				channel_id, param, level, user_id
			      );
	
			if(sqlite3_changes(db)) {
				cs_msg("Usuário removido da lista de acesso.");
			}
			else {
				cs_msg("Não foi possível efetuar a remoção.");
			}
		
		}

	}
	
}

static int chanserv_akick_cb(void *a, int argc, char **argv, char **colname) {
	fprintf(socket_fp, ":ChanServ NOTICE %s :"
		"\2%s\2 por \2%s\2: %s\n",
		(char *)a, argv[0], argv[1], argv[2]);
	return 0;
}

void chanserv_akick(char *sender, char *cmd, char *args) {

	char *channel, *param;
	int user_id, channel_id, level;
	
	strtolower(args);
	if(!(channel = strtok(args, " ")) || !(cmd = strtok(0, " "))) {
		cs_msg("Sintaxe incorreta.");
		return;
	}
	if(!(param = strtok(0, "")))
		param = "";

	if(!chanserv_get_level(sender, channel, &user_id, &channel_id, &level)){
		cs_msg("Acesso negado.");
		return;
	}

	if(!strcmp(cmd, "list")) {

		fprintf(socket_fp, ":ChanServ NOTICE %s :"
			"\2*** Akick de %s ***\2\n", sender, channel);

		dbexec(
			"select akick.mask,nicks.name,akick.reason from akick"
			" left join nicks on nicks.user=akick.user"
			" where akick.channel=%d group by akick.mask"
			" order by nicks.name;",
			chanserv_akick_cb, sender, 0,
			channel_id
		      );
		
		cs_msg("\2*** Fim da akick ***\2");
		
	}
	else {
		
		if(level < 100) {
			cs_msg("Acesso negado.");
			return;
		}
		
		if(!strcmp(cmd, "add")) {
			
			char *mask = strtok(param, " ");
			char *reason = strtok(0, "");

			if(!mask || !reason || !*mask || !*reason) {
				cs_msg("Sintaxe incorreta.");
				return;
			}
			
			dbexec(
				"insert into akick (channel,mask,reason,user)"
				" values (%d,'%q','%q',%d);",
				0, 0, 0,
				channel_id, mask, reason, user_id
			      );

			if(sqlite3_changes(db)) {
				cs_msg("Akick adicionada.");
			}
			
		}
		else if(!strcmp(cmd, "del")) {

			if(!param || !*param)
				return;

			dbexec(
				"delete from akick where channel=%d"
				" and mask='%q';",
				0, 0, 0,
				channel_id, param
			      );
	
			if(sqlite3_changes(db)) {
				cs_msg("Akick removida.");
			}
			else {
				cs_msg("Akick inexistente.");
			}
		
		}

	}

}

void chanserv_drop(char *sender, char *cmd, char *args) {

	if(!(args = strtolower(strtok(args, " ")))) {
		cs_msg("Sintaxe incorreta.");
		return;
	}

	dbexec(
		"delete from channels where name='%q' and"
		" founder=(select user from nicks where"
		"  name='%q' and identified=1);",
		0, 0, 0,
		args, sender
	      );

	if(sqlite3_changes(db)) {
		fprintf(socket_fp, ":ChanServ NOTICE %s :"
			"O registro do canal \2%s\2 foi apagado.\n",
			sender, args);
	}
	else {

		int perms = 0;
		
		dbexec("select perms from permissions where user=(select user"
			" from nicks where name='%q' and identified=1);",
			getint_cb, &perms, 0, sender);

		if(!(perms & SVSADMIN_PERM_DROP)) {
			cs_msg("Acesso negado.");
			return;
		}

		dbexec("delete from channels where name='%q';",
		       0, 0, 0, args);
		if(sqlite3_changes(db)) {
			fprintf(socket_fp, ":ChanServ NOTICE %s :"
				"O registro do canal \2%s\2 foi apagado.\n",
				sender, args);
		}
		else {
			cs_msg("Canal inexistente.");
		}
	}
	
}

static int chanserv_forbid_cb(void *a, int argc, char **argv, char **colname){
	fprintf(socket_fp, ":ChanServ NOTICE %s :"
		"%s\n",
		(char *)a, argv[0]);
	return 0;
}

static void chanserv_forbid(char *sender, char *cmd, char *args) {
	
	int perms = 0;
	strtolower(args);
	
	if(!(cmd = strtok(args, " "))) {
		cs_msg("Sintaxe incorreta.");
		return;
	}
	
	dbexec("select perms from permissions where user=(select user"
		" from nicks where name='%q' and identified=1);",
		getint_cb, &perms, 0, sender);
	
	if(!perms) {
		cs_msg("Acesso negado.");
		return;
	}
	
	if(!strcmp(cmd, "list")) {
		cs_msg("\2*** Lista de canais proibidos ***\2");
		dbexec("select mask from forbidden_channels;",
			chanserv_forbid_cb, sender, 0);
		cs_msg("\2*** Fim da lista ***\2");
	}
	else {
		
		if(!(perms & SVSADMIN_PERM_DROP)) {
			cs_msg("Acesso negado.");
			return;
		}
		if(!(args = strtok(0, " "))) {
			cs_msg("Sintaxe incorreta.");
			return;
		}
		
		if(!strcmp(cmd, "add")) {
			dbexec("insert into forbidden_channels (mask)"
				" values ('%q');",
				0, 0, 0, args);
			if(sqlite3_changes(db)) {
				cs_msg("Máscara de proibição adicionada.");
			}
		}
		else if(!strcmp(cmd, "del")) {
			dbexec("delete from forbidden_channels"
				" where mask='%q';",
				0, 0, 0, args);
			if(sqlite3_changes(db)) {
				cs_msg("Máscara de proibição removida.");
			}
			else {
				cs_msg("Máscara de proibição inexistente.");
			}
		}
		else {
			cs_msg("Comando inválido.");
		}
		
	}
}


void chanserv_help(char *sender, char *cmd, char *args) {
	
	cs_msg("\2*** Ajuda do ChanServ ***\2");
	cs_msg("/ChanServ \2access\2 #canal \2add\2 nick level");
	cs_msg("    Define o nível de acesso de alguém no canal.");
	cs_msg("/ChanServ \2access\2 #canal \2del\2 nick");
	cs_msg("    Remove alguém da lista de acesso.");
	cs_msg("/ChanServ \2access\2 #canal \2list\2");
	cs_msg("    Exibe a lista de acesso do canal.");
	cs_msg("/ChanServ \2akick\2 #canal \2add\2 mask razao");
	cs_msg("    Adiciona uma máscara na akick do canal.");
	cs_msg("/ChanServ \2akick\2 #canal \2del\2 mask");
	cs_msg("    Remove uma máscara da akick do canal.");
	cs_msg("/ChanServ \2akick\2 #canal \2list\2");
	cs_msg("    Exibe as máscaras de akick do canal.");
	cs_msg("/ChanServ \2drop\2 #canal");
	cs_msg("    Remove o registro de um canal no servidor.");
	cs_msg("/ChanServ \2forbid\2 {\2add\2|\2del\2 mask}|{\2list\2}");
	cs_msg("    Ativa/desativa proibição de uso de canais.");
	cs_msg("/ChanServ \2info\2 #canal");
	cs_msg("    Exibe informações sobre um canal.");
	cs_msg("/ChanServ \2op\2 #canal");
	cs_msg("    Dá op para você no canal.");
	cs_msg("/ChanServ \2register\2 #canal");
	cs_msg("    Registra um canal no servidor.");
	cs_msg("/ChanServ \2resetmodes\2 #canal");
	cs_msg("    Redefine os modos originais no canal.");
	cs_msg("/ChanServ \2set\2 #canal \2founder\2 nick");
	cs_msg("    Altera o fundador de um canal.");
	cs_msg("/ChanServ \2unban\2 #canal");
	cs_msg("    Anula qualquer ban contra você no canal.");
	cs_msg("/ChanServ \2voice\2 #canal");
	cs_msg("    Dá voice para você no canal.");
	cs_msg("\2*** Fim da ajuda ***\2");

}

static int chanserv_info_cb(void *arg, int argc, char **argv, char **colname) {

	char buf[128];
	time_t ts = atoll(argv[2]);
	char *to = (char *)arg;

	
	fprintf(socket_fp, ":ChanServ NOTICE %s :"
			"\2*** Info de %s ***\2\n", to, argv[0]);
	fprintf(socket_fp, ":ChanServ NOTICE %s :"
			"Fundador: %s\n", to, argv[1] ? argv[1] : "(nenhum)");

	strftime(buf, 128, "%d %b %H:%M:%S %Y GMT", gmtime(&ts));
	
	fprintf(socket_fp, ":ChanServ NOTICE %s :"
			"Registrado em: %s\n", to, buf);
	
	if(argv[3]) {
		ts = atoll(argv[3]);
		strftime(buf, 128, "%d %b %H:%M:%S %Y GMT", gmtime(&ts));

		fprintf(socket_fp, ":ChanServ NOTICE %s :"
				"Último join em: %s\n", to, buf);
	}

	if(argv[4]) {
		fprintf(socket_fp, ":ChanServ NOTICE %s :"
				"Tópico: %s\n", to, argv[4]);
	}

	if(argv[5]) {
		fprintf(socket_fp, ":ChanServ NOTICE %s :"
				"Trava de modos: %s\n", to, argv[5]);
	}

	fprintf(socket_fp, ":ChanServ NOTICE %s :"
			"\2*** Fim da info ***\2\n", to);

	return 1;

}

void chanserv_info(char *sender, char *cmd, char *args) {
	
	if(!(args = strtolower(strtok(args, " "))))
		return;

	dbexec(
		"select channels.name,nicks.name,channels.regtime,"
		"channels.lastjoin,channels.topic,channels.modes"
		" from channels"
		" left join nicks on nicks.user=channels.founder"
		" where channels.name='%q' limit 1;",
		chanserv_info_cb, sender, 0, args
	      );
	
}

void chanserv_op(char *sender, char *cmd, char *args) {

	int res = 0;

	if(!(args = strtolower(strtok(args, " ")))) {
		cs_msg("Sintaxe incorreta.");
		return;
	}
	
	dbexec(
		"select count(*) from access"
		" where channel=(select id from channels"
		"  where name='%q') and"
		" user=(select user from nicks"
		"  where name='%q' and identified=1) and"
		" level>=2;",
		getint_cb, &res, 0,
		args, sender
	      );

	if(res) {
		fprintf(socket_fp, ":ChanServ MODE %s +o %s\n",
				args, sender);
	}
	else {
		cs_msg("Acesso negado.");
	}
	
}

void chanserv_register(char *sender, char *cmd, char *args) {

	int i;
	
	args = strtolower(strtok(args, " ,"));
	
	if(!args || (args[0] != '#')) {
		cs_msg("Nome de canal inválido.");
		return;
	}

	for(i = 1; i < CONFIG_CHANNEL_NAMEMAX && args[i]; i++)
		if(!isprint(args[i])) {
			cs_msg("Nome de canal inválido.");
			return;
		}

	args[i] = 0;
	
	dbexec(
		"insert into channels (name,modes,founder,regtime)"
		" select '%q','nt',user,%d from nicks where"
		"  name='%q' and identified=1;",
		0, 0, 0,
		args, time(0), sender
	      );

	if(sqlite3_changes(db)) {
		fprintf(socket_fp, ":ChanServ NOTICE %s :"
			"O canal %s foi registrado em seu nome.\n",
			sender, args);
	}
	else {
		cs_msg("Não foi possível registrar o canal.");
	}
	
}

void chanserv_resetmodes(char *sender, char *cmd, char *args) {

	int res = 0;

	if(!(args = strtolower(strtok(args, " ")))) {
		cs_msg("Sintaxe incorreta.");
		return;
	}
	
	dbexec(
		"select count(*) from access"
		" where channel=(select id from channels"
		"  where name='%q') and"
		" user=(select user from nicks"
		"  where name='%q' and identified=1) and"
		" level>=2;",
		getint_cb, &res, 0,
		args, sender
	      );

	if(res) {
		dbexec("update channels set modes='nt' where name='%q';",
				0, 0, 0, args);
		fprintf(socket_fp, ":ChanServ MODE %s -imscMRSp\n", args);
		fprintf(socket_fp, ":ChanServ MODE %s -l *\n", args);
		fprintf(socket_fp, ":ChanServ MODE %s -k *\n", args);
	}
	else {
		cs_msg("Acesso negado.");
	}
	
}

void chanserv_set(char *sender, char *cmd, char *args) {

	char *channel, *param;
	
	strtolower(args);
	if(!(channel = strtok(args, " ")) || !(cmd = strtok(0, " ")) || 
			!(param = strtok(0, " "))) {
		cs_msg("Sintaxe incorreta.");
		return;
	}
		

	if(!strcmp(cmd, "founder")) {

		dbexec(
			"update channels set founder=(select user from nicks"
			"  where name='%q') where name='%q' and"
			" founder=(select user from nicks where name='%q' and"
			"  identified=1);",
			0, 0, 0,
			param, channel, sender
		      );
		
		if(sqlite3_changes(db)) {
			fprintf(socket_fp, ":ChanServ NOTICE %s :"
				"O fundador do canal %s foi alterado "
				"para %s.\n",
				sender, channel, param);
		}
		else {
			cs_msg("Acesso negado.");
		}

	}
	else {
		cs_msg("Comando inválido.");
	}

	
}

void chanserv_unban(char *sender, char *cmd, char *args) {

	int res = 0;
	
	if(!(args = strtolower(strtok(args, " ")))) {
		cs_msg("Sintaxe incorreta.");
		return;
	}
	
	dbexec(
		"select count(*) from access"
		" where channel=(select id from channels"
		"  where name='%q') and"
		" user=(select user from nicks"
		"  where name='%q' and identified=1);",
		getint_cb, &res, 0,
		args, sender
	      );
	
	if(res) {
		fprintf(socket_fp, ":ChanServ MODE %s +e %s!*@*\n",
				args, sender);
		fprintf(socket_fp, ":ChanServ MODE %s +I %s!*@*\n",
				args, sender);
		fprintf(socket_fp, ":ChanServ NOTICE %s :"
				"Foi marcado exempt para você no canal %s.\n",
				sender, args);
	}
	else {
		cs_msg("Acesso negado.");
	}
	
}

void chanserv_voice(char *sender, char *cmd, char *args) {

	int res = 0;
	
	if(!(args = strtolower(strtok(args, " ")))) {
		cs_msg("Sintaxe incorreta.");
		return;
	}
	
	dbexec(
		"select count(*) from access"
		" where channel=(select id from channels"
		"  where name='%q') and"
		" user=(select user from nicks"
		"  where name='%q' and identified=1) and"
		" level>=1;",
		getint_cb, &res, 0,
		args, sender
	      );

	if(res) {
		fprintf(socket_fp, ":ChanServ MODE %s +v %s\n",
				args, sender);
	}
	else {
		cs_msg("Acesso negado.");
	}
	
}

/* must be in alphabetical order */
static trap_entry chanserv_pvt_table[] = {
	{ "access",     chanserv_access     },
	{ "akick",      chanserv_akick      },
	{ "drop",       chanserv_drop       },
	{ "forbid",     chanserv_forbid     },
	{ "help",       chanserv_help       },
	{ "info",       chanserv_info       },
	{ "op",         chanserv_op         },
	{ "register",   chanserv_register   },
	{ "resetmodes", chanserv_resetmodes },
	{ "set",        chanserv_set        },
	{ "unban",      chanserv_unban      },
	{ "voice",      chanserv_voice      },
};


void chanserv_pvt(char *sender, char *cmd, char *args) {
	if(!(cmd = strtolower(strtok(args, " "))))
		return;
	if(!(args = strtok(0, "")))
		args = "";
	if(!binsearch(chanserv_pvt_table,
			sizeof(chanserv_pvt_table)/sizeof(trap_entry)-1,
			sender, cmd, args)) {
		cs_msg("Comando inválido.");
	}
	fflush(socket_fp);
}

static int chanserv_tb_cb(void *arg, int argc, char **argv, char **colname) {
	if(argv[1]) {
		/* Use very low false timestamp to guarantee topic is updated.
		 * Thanks to cryogen from OFTC for solving the issue!
		 */
		fprintf(socket_fp, ":" CONFIG_SVS_SERVER " TBURST "
				"1 %s 1 %s :%s\n",
				(char *)arg,
				argv[0] ? argv[0] : "dropped-nick",
				argv[1]);
	}
	return 1;
}

static int chanserv_mode_cb(void *arg, int argc, char **argv, char **colname) {
	strncpy((char *)arg, argv[0], 63);
	((char *)arg)[63] = 0;
	return 1;
}

void chanserv_sjoin(char *sender, char *cmd, char *args) {

	char *channel, *nicks, *joinmode;
	int channel_id = -1, forbid = -1;
	
	strtolower(args);
	
	strtok(args, " ");
	channel = strtok(0, " ");
	joinmode = strtok(0, " ");
	nicks = strtok(0, "") + 1;

	dbexec("select 1 from forbidden_channels"
			" where '%q' glob mask limit 1;",
			getint_cb, &forbid, 0, channel);
	
	if(forbid != -1) {
		for(nicks = strtok(nicks, " "); nicks; nicks = strtok(0, " ")) {
			if(*nicks == '@' || *nicks == '+') {
				++nicks;
			}
			fprintf(socket_fp, ":ChanServ "
				"KICK %s %s :O uso deste canal foi proibido.\n",
				channel, nicks);
		}
		fflush(socket_fp);
		return;
	}
	
	dbexec("select id from channels where name='%q';",
		getint_cb, &channel_id, 0, channel);

	if(channel_id != -1) {
		for(nicks = strtok(nicks, " "); nicks; nicks = strtok(0, " ")) {
			if(*nicks == '@') {
				fprintf(socket_fp, ":ChanServ "
					"MODE %s -o %s\n",
					channel, ++nicks);
			}
			else if(*nicks == '+') {
				++nicks;
			}
			usercache_queue(nicks, channel_id);
		}
		if(strcmp(joinmode, "+")) {
			char mode[64];
			dbexec("select nicks.name,channels.topic from channels "
				"left join nicks on nicks.user=channels.topicby"
				" where channels.id=%d limit 1;",
				chanserv_tb_cb, channel, 0, channel_id);
			dbexec("select modes from channels where id=%d;",
				chanserv_mode_cb, &mode, 0, channel_id);
			fprintf(socket_fp, ":ChanServ MODE %s +%s\n",
				channel, mode);	
		}
		dbexec("update channels set lastjoin=%d where id=%d;",
				0, 0, 0, time(0), channel_id);
	}
	
	fflush(socket_fp);

}

void chanserv_topic(char *sender, char *cmd, char *args) {
	
	char *channel = strtolower(strtok(args, " "));
	char *topic = strtok(0, "") + 1;
	int channel_id = -1, user_id = -1, ret = 0;
	
	dbexec("select id from channels where name='%q';",
			getint_cb, &channel_id, 0, channel);

	if(channel_id == -1)
		return;
	
	dbexec("select user from nicks where name='%q' and identified=1;",
			getint_cb, &user_id, 0, sender);

	dbexec("select count(*) from access where channel=%d and user=%d;",
			getint_cb, &ret, 0, channel_id, user_id);

	if(ret) {
		dbexec("update channels set topic='%q',topicby=%d"
				" where id=%d;",
				0, 0, 0, topic, user_id, channel_id);
	}
	else {
		dbexec(
			"select nicks.name,channels.topic from channels"
			" left join nicks on nicks.user=channels.topicby"
			" where channels.id=%d limit 1;",
			chanserv_tb_cb, channel, 0, channel_id
		      );
	}

	fflush(socket_fp);
	
}

void chanserv_mode(char *sender, char *cmd, char *args) {
	
	char *channel = strtolower(strtok(args, " "));
	char *mode = strtok(0, "");
	char oldmode[64], *p, buf[2];
	char tmpmode[64] = { 0 };
	char *notstored = "volkbeI";
	int channel_id = -1, user_id = -1, ret = 0;
	
	dbexec("select id from channels where name='%q';",
			getint_cb, &channel_id, 0, channel);

	if(channel_id == -1)
		return;
	
	dbexec("select modes from channels where id=%d;",
			chanserv_mode_cb, &oldmode, 0, channel_id);
	
	dbexec("select user from nicks where name='%q' and identified=1;",
			getint_cb, &user_id, 0, sender);

	dbexec("select count(*) from access where channel=%d and user=%d;",
			getint_cb, &ret, 0, channel_id, user_id);
	
	if(ret) {
		buf[1] = 0;
		if((p = strchr(mode, ' ')))
			*p = 0;
		if(*mode == '+') {
			for(p = mode+1; *p; p++) {
				if(!strchr(oldmode, *p) &&
						!strchr(notstored, *p)) {
					buf[0] = *p;
					strncat(oldmode, buf, 63);
				}
			}
		}
		else {
			for(p = oldmode; *p; p++) {
				if(!strchr(mode, *p)) {
					buf[0] = *p;
					strcat(tmpmode, buf);
				}
			}
			strcpy(oldmode, tmpmode);
		}
		dbexec("update channels set modes='%q' where id=%d;",
				0, 0, 0, oldmode, channel_id);
	}
	else {
		if(strncmp(mode,"-o ",3) || strcmp(mode+3,sender)) {
			*mode = (*mode == '+') ? '-' : '+';
			fprintf(socket_fp, ":ChanServ MODE %s %s\n",
					channel, mode);
			fprintf(socket_fp, ":ChanServ MODE %s +%s\n",
					channel, oldmode);
		}
	}

	fflush(socket_fp);
	
}

static int chanserv_akickprocess_cb(
		void *arg, int argc, char **argv, char **colname) {
	
	fprintf(socket_fp, ":ChanServ KICK %s %s :Akick: %s\n",
			argv[0], (char *)arg, argv[2]);
	fprintf(socket_fp, ":ChanServ MODE %s +b %s\n",
			argv[0], argv[1]);
	fflush(socket_fp);
	return 1;

}

void chanserv_akickprocess(char *nick, char *userhost, chanent *channels) {
	char *mask = (char *)malloc(strlen(nick)+strlen(userhost)+2);
	sprintf(mask, "%s!%s", nick, userhost);
	for(; channels; channels = channels->next)
		 dbexec(
			"select channels.name,akick.mask,akick.reason"
			" from akick"
			" left join channels on channels.id=akick.channel"
			" where akick.channel=%d and '%q' glob akick.mask"				" limit 1;",
			chanserv_akickprocess_cb, nick, 0,
			channels->channel, mask
		       );
}

