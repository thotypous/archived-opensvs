create table config (key text primary key, value integer);
insert into config values ('access_limit', 20);
insert into config values ('akick_limit', 20);
insert into config values ('info_aliases', 3);

create table users (id integer primary key autoincrement, password text, email text, killprot integer, lastseen integer, quitmsg text);
create trigger users_ondelete after delete on users begin
delete from access where user = old.id;
delete from akick where user = old.id;
delete from permissions where user = old.id;
update channels set founder=(select user from access where channel=id order by level desc limit 1) where founder = old.id;
update channels set topicby = -1 where topicby = old.id;
end;

create table nicks (name text primary key on conflict rollback, user integer, regtime integer, identified integer);
create index nicks_index on nicks(user);
create trigger nicks_ondelete after delete on nicks when (select count(*) from nicks where user = old.user) = 0 begin
delete from users where id = old.user;
end;

create table channels (id integer primary key autoincrement, name text unique, topic text, topicby integer, modes text, founder integer not null, regtime integer, lastjoin integer);
create trigger channels_ondelete after delete on channels begin
delete from access where channel = old.id;
delete from akick where channel = old.id;
end;
create trigger channels_founderaccess after insert on channels begin
insert into access (channel,user,level) values (new.id,new.founder,9999);
end;

create table access (channel integer, user integer, level integer, primary key(channel, user) on conflict replace);
create trigger access_limit after insert on access when (select count(*) from access where channel = new.channel) > (select value from config where key='access_limit') begin
select raise(rollback, 'Access list limit exceeded.');
end;
create trigger access_chandrop after delete on access when (select count(*) from access where channel = old.channel) = 0 begin
delete from channels where id=old.channel;
end;

create table akick (channel integer, mask text, reason text, user integer, primary key(channel, mask) on conflict replace);
create trigger akick_limit after insert on akick when (select count(*) from akick where channel = new.channel) > (select value from config where key='akick_limit') begin
select raise(rollback, 'Akick list limit exceeded.');
end;

create table forbidden_channels (mask text primary key);
create table forbidden_nicks (mask text primary key);

create table permissions (user integer primary key on conflict replace, perms integer);

