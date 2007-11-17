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

#include "nickserv.h"
#include "database.h"
#include "usercache.h"
#include "killprot.h"
#include "socket.h"
#include "trap.h"
#include "util.h"

#define ns_msg(x) fprintf(socket_fp, ":NickServ NOTICE %s :" x "\n", sender)

static void nickserv_drop(char *sender, char *cmd, char *args) {
	
	char *nick = strtolower(strtok(args, " "));
	
	if(!nick || !*nick || !strcmp(sender, nick)) {
		dbexec("delete from nicks"
		       " where name='%q' and identified=1;",
		       0, 0, 0, sender);
		if(sqlite3_changes(db)) {
			fprintf(socket_fp, "SVSMODE %s :-R\n", sender);
			ns_msg("O registro do seu nick foi apagado.");
		}
		else {
			ns_msg("Permissão negada.");
		}
	}
	else {
		
		int perms = 0;
		
		dbexec("select perms from permissions where user=(select user"
			" from nicks where name='%q' and identified=1);",
			getint_cb, &perms, 0, sender);

		if(!(perms & SVSADMIN_PERM_DROP)) {
			ns_msg("Permissão negada.");
			return;
		}

		dbexec("delete from nicks where name='%q';",
		       0, 0, 0, args);
		if(sqlite3_changes(db)) {
			fprintf(socket_fp, "SVSMODE %s :-R\n", args);
			fprintf(socket_fp, ":NickServ NOTICE %s :"
				"O registro do nick \2%s\2 foi apagado.\n",
				sender, args);
		}
		else {
			ns_msg("Nick não encontrado.");
		}
		
	}
	
}

static int nickserv_forbid_cb(void *a, int argc, char **argv, char **colname){
	fprintf(socket_fp, ":NickServ NOTICE %s :"
		"%s\n",
		(char *)a, argv[0]);
	return 0;
}

static void nickserv_forbid(char *sender, char *cmd, char *args) {
	
	int perms = 0;
	strtolower(args);
	
	if(!(cmd = strtok(args, " ")))
		return;
	
	dbexec("select perms from permissions where user=(select user"
		" from nicks where name='%q' and identified=1);",
		getint_cb, &perms, 0, sender);
	
	if(!perms) {
		ns_msg("Permissão negada.");
		return;
	}
	
	if(!strcmp(cmd, "list")) {
		ns_msg("\2*** Lista de nicks proibidos ***\2");
		dbexec("select mask from forbidden_nicks;",
			nickserv_forbid_cb, sender, 0);
		ns_msg("\2*** Fim da lista ***\2");
	}
	else {
		
		if(!(perms & SVSADMIN_PERM_DROP)) {
			ns_msg("Permissão negada.");
			return;
		}
		if(!(args = strtok(0, " "))) {
			ns_msg("Sintaxe incorreta.");
			return;
		}
		
		if(!strcmp(cmd, "add")) {
			dbexec("insert into forbidden_nicks (mask)"
				" values ('%q');",
				0, 0, 0, args);
			if(sqlite3_changes(db)) {
				ns_msg("Máscara de proibição adicionada.");
			}
		}
		else if(!strcmp(cmd, "del")) {
			dbexec("delete from forbidden_nicks"
				" where mask='%q';",
				0, 0, 0, args);
			if(sqlite3_changes(db)) {
				ns_msg("Máscara de proibição removida.");
			}
			else {
				ns_msg("Máscara de proibição inexistente.");
			}
		}
		else {
			ns_msg("Comando inválido.");
		}
		
	}
}

static void nickserv_ghost(char *sender, char *cmd, char *args) {
	
	char *nick = strtolower(strtok(args, " "));
	char *password = strtok(0, " ");

	if(!nick || !password) {
		ns_msg("Sintaxe incorreta.");
		return;
	}

	dbexec(
		"update nicks set identified=0 where"
		" name='%q' and"
		" (select count(*) from users where"
		"  id=user and password='%q')=1;",
		0, 0, 0,
		nick, md5(password)
	      );
	
	if(sqlite3_changes(db)) {
		
		fprintf(socket_fp, ":NickServ KILL %s :NickServ (Ghost (%s))\n",
				nick, sender);
		
		if(*cmd == 'r') {   /* regain */
			dbexec("update nicks set identified=1 where"
			       " name='%q';", 0,0,0, nick);
			fprintf(socket_fp, "SVSNICK %s %s\n"
					"SVSMODE %s :+R\n",
					sender, nick, nick);
		} else {
			ns_msg("O nick que você solicitou foi derrubado.");
		}

	}
	else {
		ns_msg("Senha incorreta.");
	}

}


static void nickserv_group(char *sender, char *cmd, char *args) {

	char *nick = strtolower(strtok(args, " "));
	char *password = strtok(0, " ");
	
	if(!nick || !password)
		return;
	
	dbexec(
		"insert into nicks (name,user,regtime,identified)"
		" select '%q',user,%d,1 from nicks where"
		"  name='%q' and"
		"  (select count(*) from users where"
		"   id=user and password='%q')=1",
		0, 0, 0,
		sender, time(0), nick, md5(password)
	      );

	if(sqlite3_changes(db)) {
		ns_msg("Nick agrupado.");
		fprintf(socket_fp, "SVSMODE %s :+R\n", sender);
	}
	else {
		ns_msg("Não foi possível agrupar o nick.");
	}
	
}

static void nickserv_help(char *sender, char *cmd, char *args) {
	
	ns_msg("\2*** Ajuda do NickServ ***\2");
	ns_msg("/NickServ \2drop\2 [nick]");
	ns_msg("    Apaga o registro de um nick.");
	ns_msg("/NickServ \2forbid\2 {\2add\2|\2del\2 mask}|{\2list\2}");
	ns_msg("    Ativa/desativa proibição de uso de nicks.");
	ns_msg("/NickServ \2ghost\2 nick senha");
	ns_msg("    Derruba alguém que estiver usando seu nick.");
	ns_msg("/NickServ \2group\2 nick senha");
	ns_msg("    Agrupa seu nick atual a um já registrado.");
	ns_msg("/NickServ \2identify\2 senha");
	ns_msg("    Identifica você no servidor.");
	ns_msg("/NickServ \2info\2 nick");
	ns_msg("    Exibe informações sobre um nick.");
	ns_msg("/NickServ \2regain\2 nick senha");
	ns_msg("    Troca seu nick já identificando no servidor.");
	ns_msg("/NickServ \2register\2 senha [email]");
	ns_msg("    Registra seu nick atual no servidor.");
	ns_msg("/NickServ \2set email\2 email");
	ns_msg("    Troca o seu email.");
	ns_msg("/NickServ \2set killprot\2 segundos");
	ns_msg("    Impede que seu nick seja usado depois de certo tempo se");
	ns_msg("    ele não for identificado.");
	ns_msg("/NickServ \2set password\2 senha");
	ns_msg("    Troca a senha dos seus nicks.");
	ns_msg("/NickServ \2svsadmin\2 {\2add\2|\2del\2 nick}|{\2list\2}");
	ns_msg("    Adiciona/remove um administrador dos services.");
	ns_msg("\2*** Fim da ajuda ***\2");

}

static void nickserv_identify(char *sender, char *cmd, char *args) {

	char *password = strtok(args, " ");

	if(!password) {
		ns_msg("Sintaxe incorreta.");
		return;
	}

	dbexec(
		"update nicks set identified=1 where"
		" name='%q' and"
		" (select count(*) from users where"
		"  id=user and password='%q')=1;",
		0, 0, 0,
		sender, md5(password)
	      );
	
	if(sqlite3_changes(db)) {
		ns_msg("Você agora está identificado.");
		fprintf(socket_fp, "SVSMODE %s :+R\n", sender);
		killprot_unsched(sender);
	}
	else {
		ns_msg("Senha incorreta.");
	}
	
}

static int nickserv_info_aliases_cb(void *arg, int argc, char **argv,
		char **colname) {

	char *buf = (char *)arg;

	strncat(buf, argv[0], 127);
	strncat(buf, ", ", 127);

	return buf[126];
	
}


static int nickserv_info_cb(void *arg, int argc, char **argv, char **colname) {
	
	char buf[128];
	time_t ts = atoll(argv[1]);
	char *to = (char *)arg;
	
	strftime(buf, 128, "%d %b %H:%M:%S %Y GMT", gmtime(&ts));
	
	fprintf(socket_fp, ":NickServ NOTICE %s :"
			"\2*** Info de %s ***\2\n", to, argv[0]);
	fprintf(socket_fp, ":NickServ NOTICE %s :"
			"Horário de registro: %s\n", to, buf);

	if(argv[4] && argv[5]) {
		fprintf(socket_fp, ":NickServ NOTICE %s :"
				"Última msg de quit: %s\n", to, argv[5]);
		ts = atoll(argv[4]);
		strftime(buf, 128, "%d %b %H:%M:%S %Y GMT", gmtime(&ts));
		fprintf(socket_fp, ":NickServ NOTICE %s :"
				"Em: %s\n", to,	buf);
	}

	if(argv[3] && *argv[3]) {
		fprintf(socket_fp, ":NickServ NOTICE %s :"
				"Email: %s\n", to, argv[3]);
	}
	
	memset(buf, 0, sizeof(buf));
	dbexec("select name from nicks where user=%q and name!='%q'"
			" limit (select value from config"
			"  where key='info_aliases');",
			nickserv_info_aliases_cb, buf, 0, argv[2], argv[0]);

	if(*buf) {
		buf[strlen(buf) - 2] = 0;
		fprintf(socket_fp, ":NickServ NOTICE %s :"
				"Aliases: %s\n", to, buf);
	}
	
	fprintf(socket_fp, ":NickServ NOTICE %s :"
			"\2*** Fim da info ***\2\n", to);

	return 1;

}

static void nickserv_info(char *sender, char *cmd, char *args) {

	if(!(args = strtolower(strtok(args, " "))))
		return;

	dbexec(
		"select nicks.name,nicks.regtime,nicks.user,"
		"users.email,users.lastseen,users.quitmsg from nicks"
		" left join users on users.id=nicks.user"
		" where nicks.name='%q';",
		nickserv_info_cb, sender, 0, args
	      );
	
}

static void nickserv_register(char *sender, char *cmd, char *args) {
	
	char *password = strtok(args, " ");
	char *email = strtok(0, "");
	
	if(!password) {
		ns_msg("Informe uma senha para registar o nick.");
		return;
	}
	
	if(!email)
		email = "";
	
	dbexec(
		"begin;"
		"insert into users (password,email,killprot)"
		" values('%q','%q',0);"
		"insert into nicks (name,user,regtime,identified)"
		" values('%q',last_insert_rowid(),%d,1);"
		"commit;",
		0, 0, 0,
		md5(password), email, sender, time(0)
	      );

	if(sqlite3_changes(db)) {
		ns_msg("Nick registrado.");
		fprintf(socket_fp, "SVSMODE %s :+R\n", sender);
	}
	else {
		ns_msg("O nick já está registrado para outra pessoa.");
	}

}

static void nickserv_set(char *sender, char *cmd, char *args) {

	if(!(cmd = strtolower(strtok(args, " ")))) {
		ns_msg("Sintaxe incorreta.");
		return;
	}
	if(!(args = strtok(0, " ")))
		args = "";

	if(!strcmp(cmd, "password") && *args) {
		dbexec(
			"update users set password='%q' where"
			" id=(select user from nicks"
			"  where name='%q' and identified=1);",
			0, 0, 0,
			md5(args), sender
		      );
		if(sqlite3_changes(db)) {
			ns_msg("A senha dos seus nicks foi alterada.");
		}
	}
	else if(!strcmp(cmd, "email")) {
		dbexec(
			"update users set email='%q' where"
			" id=(select user from nicks"
			"  where name='%q' and identified=1);",
			0, 0, 0,
			args, sender
		      );
		if(sqlite3_changes(db)) {
			ns_msg("O email dos seus nicks foi alterado.");
		}
	}
	else if(!strcmp(cmd, "killprot")) {
		int killprot = atoi(args);
		if(killprot < 10)
			killprot = 0;
		else if(killprot > 60)
			killprot = 60;
		
		dbexec(
			"update users set killprot=%d where"
			" id=(select user from nicks"
			"  where name='%q' and identified=1);",
			0, 0, 0,
			killprot, sender
		      );
		
		if(sqlite3_changes(db)) {
			if(killprot) {
				fprintf(socket_fp, ":NickServ NOTICE %s :"
					"A proteção de kill foi alterada para"
					" %d segundos.\n", sender, killprot);
			}
			else {
				ns_msg("A proteção de kill foi desabilitada.");
			}
		}

	}
	else {
		ns_msg("Comando inválido.");
	}
	
}

static int nickserv_svsadmin_cb(void *a, int argc, char **argv, char **colname){
	fprintf(socket_fp, ":NickServ NOTICE %s :"
		"\2%-2s\2 %s\n",
		(char *)a, argv[0], argv[1]);
	return 0;
}

static void nickserv_svsadmin(char *sender, char *cmd, char *args) {
	
	int perms = 0;
	strtolower(args);
	
	if(!(cmd = strtok(args, " "))) {
		ns_msg("Sintaxe incorreta.");
		return;
	}
	
	dbexec("select perms from permissions where user=(select user"
		" from nicks where name='%q' and identified=1);",
		getint_cb, &perms, 0, sender);
	
	if(!perms) {
		ns_msg("Acesso negado.");
		return;
	}
	
	if(!strcmp(cmd, "list")) {
		ns_msg("\2*** Lista de svsadmins ***\2");
		dbexec("select permissions.perms,nicks.name from permissions"
			" left join nicks on nicks.user=permissions.user"
			" group by permissions.user order by nicks.name;",
			nickserv_svsadmin_cb, sender, 0);
		ns_msg("\2*** Fim da lista ***\2");
	}
	else {
		
		if(!(perms & SVSADMIN_PERM_ADMINLIST))
		{
			ns_msg("Acesso negado.");
			return;
		}
		if(!(args = strtok(0, " "))) {
			ns_msg("Sintaxe incorreta.");
			return;
		}
		
		if(!strcmp(cmd, "add")) {
			dbexec("insert into permissions (user,perms)"
				" select user,3 from nicks"
				" where name='%q';",
				0, 0, 0, args);
			if(sqlite3_changes(db)) {
				ns_msg("Usuário adicionado como svsadmin.");
			}
		}
		else if(!strcmp(cmd, "del")) {
			dbexec("delete from permissions where user=("
				"select user from nicks where name='%q');",
				0, 0, 0, args);
			if(sqlite3_changes(db)) {
				ns_msg("Retirado svsadmin do usuário.");
			}
			else {
				ns_msg("Usuário não cadastrado como svsadmin.");
			}
		}
		else {
			ns_msg("Comando inválido.");
		}
		
	}
}

/* must be in alphabetical order */
static trap_entry nickserv_pvt_table[] = {
	{ "drop",     nickserv_drop     },
	{ "forbid",   nickserv_forbid   },
	{ "ghost",    nickserv_ghost    },
	{ "group",    nickserv_group    },
	{ "help",     nickserv_help     },
	{ "identify", nickserv_identify },
	{ "info",     nickserv_info     },
	{ "regain",   nickserv_ghost    },
	{ "register", nickserv_register },
	{ "set",      nickserv_set      },
	{ "svsadmin", nickserv_svsadmin },
};

void nickserv_pvt(char *sender, char *cmd, char *args) {
	if(!(cmd = strtolower(strtok(args, " "))))
		return;
	if(!(args = strtok(0, "")))
		args = "";
	if(!binsearch(nickserv_pvt_table,
			sizeof(nickserv_pvt_table)/sizeof(trap_entry)-1,
			sender, cmd, args)) {
		ns_msg("Comando inválido.");
	}
	fflush(socket_fp);
}

void nickserv_quit(char *sender, char *cmd, char *args) {
	dbexec(
		"update users set quitmsg='%q',lastseen=%d"
		" where id=(select user from nicks"
		"  where name='%q' and identified=1);"
		"update nicks set identified=0 where name='%q';",
		0, 0, 0,
		args+1, time(0), sender, sender
	      );
	usercache_del(sender);
	killprot_unsched(sender);
}

static int nickserv_nick_cb(void *arg, int argc, char **argv, char **colname) {
	
	int killprot = atoi(argv[2]?argv[2]:"0");
	int *identified = (int *)arg;

	*identified = atoi(argv[1]);
	
	if(!*identified) {
		if(killprot) {
			killprot_sched(argv[0], time(0) + killprot);
		}
		fprintf(socket_fp, ":NickServ NOTICE %s :Por favor, digite "
				"\2/NickServ identify senha\2 "
				"para autenticar seu nick.\n", argv[0]);
	}
	
	return 1;

}

static int nickserv_doforbid_cb(void *a, int argc, char **argv, char **colname){
	
	char *newnick = (char *)a;

	sprintf(newnick, "Ghost%d", rand()%9999999);
	
	fprintf(socket_fp, "SVSNICK %s %s\n",
		argv[0], newnick);

	fflush(socket_fp);
	
	return 1;
}

void nickserv_nick(char *sender, char *cmd, char *args) {
	
	char *nick = strtolower(strtok(args, " "));
	char newnick[13] = { 0 };
	int identified;

	dbexec("select '%q' from forbidden_nicks"
			" where '%q' glob mask limit 1;",
			nickserv_doforbid_cb, newnick, 0, nick, nick);

	if(*newnick)
		nick = newnick;
	
	if(sender) {

		dbexec(
			"update nicks set identified=1"
			" where name='%q' and user=(select user from nicks"
			"  where name='%q' and identified=1);",
			0, 0, 0,
			nick, sender
		      );
		
		identified = sqlite3_changes(db);
		
		dbexec(
			"update nicks set identified=0"
			" where name='%q';",
			0, 0, 0,
			sender
		      );
		
		if(identified) {
			fprintf(socket_fp, "SVSMODE %s :+R\n", nick);
			fflush(socket_fp);
			return;
		}

		usercache_del(sender);
		killprot_unsched(sender);

	}
	else {
		char *userhost;
		strtok(0, " ");
		strtok(0, " ");
		strtok(0, " ");
		userhost = strtok(0, " ");
		*(strtok(0, " ")-1) = '@';
		usercache_add(nick, strtolower(userhost));
	}

	dbexec(
		"select nicks.name,nicks.identified,users.killprot from nicks"
		" left join users on users.id=nicks.user"
		" where nicks.name='%q';",
		nickserv_nick_cb, &identified, 0,
		nick
	      );

	if(sender && !identified)
		fprintf(socket_fp, "SVSMODE %s :-R\n", nick);

	fflush(socket_fp);
	
}
