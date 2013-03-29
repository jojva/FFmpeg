CC=gcc
CFLAGS= --coverage -g -I/usr/local/include
LDFLAGS= -L/usr/local/lib -lswscale -lavformat -lavcodec -lm -lavutil

%OBJ= pktdumper.o xtract.o

all: clean pktdumper xtract

pktdumper: pktdumper.o
	$(CC) $(CFLAGS) pktdumper.o $(LDFLAGS) -o pktdumper

pktdumper.o: pktdumper.c

xtract: xtract.o
	$(CC) $(CFLAGS) xtract.o $(LDFLAGS) -o xtract

xtract.o: xtract.c

clean:
	rm -f *~ *gcno *gcda $(OBJ) pktdumper xtract

#gcc --coverage -g extract.o -lswscale -lavutil -lavformat -lavcodec -lm -o extract

#gcc --coverage -g -o extract extract.c -lswscale -lavutil -lavformat -lavcodec -lavutil -lm
