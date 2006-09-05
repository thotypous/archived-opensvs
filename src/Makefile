CC	= gcc
CFLAGS	= -Wall -O3 -funroll-loops -march=pentium4 -fomit-frame-pointer `pkg-config --cflags sqlite3` 

LIBS	= `pkg-config --libs sqlite3`
OBJS	= main.o socket.o util.o parser.o trap.o fakeuser.o database.o usercache.o nickserv.o chanserv.o md5.o killprot.o antiflood.o

%.o: %.c
	$(CC) $(CFLAGS) -c $< -o $*.o

all: opensvs

opensvs: $(OBJS)
	$(CC) $(OBJS) $(LIBS) -o opensvs

clean:
	rm -f $(OBJS) opensvs

