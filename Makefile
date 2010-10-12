CC=gcc

all: libtcpstats.so.1.0.1

prel.o: prel.c
	$(CC) -fPIC -rdynamic -g -Wall -c $<

libtcpstats.so.1.0.1: prel.o
	$(CC) -shared -Wl,-soname,libtcpstats.so.1 \
		-o $@ \
		$< -lc -ldl

clean:
	rm -f *.o *.so *.so.[0-9].[0-9].[0-9]
