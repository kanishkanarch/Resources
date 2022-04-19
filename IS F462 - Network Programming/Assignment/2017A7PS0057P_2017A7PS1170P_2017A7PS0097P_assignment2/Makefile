CC = gcc

all: multicast synflood error_packets

multicast: src/P2/multicast.c
	$(CC) src/P2/multicast.c -o bin/multicast -Wall -lrt

synflood: src/P3/synflood.c src/P3/synflood.h
	$(CC) src/P3/synflood.c -o bin/synflood -Wall -lpcap 

error_packets: src/P4/error_packets.c 
	$(CC) src/P4/error_packets.c -o bin/error_packets -Wall

server:  src/P1/webserver.c src/P1/webserver.h src/P1/helper.c src/P1/helper.h src/P1/hashtable.c src/P1/hashtable.h
	$(CC) src/P1/webserver.c src/P1/hashtable.c src/P1/helper.c -o bin/webserver -Wall -pthread

clean:
	find -type f -name "*.o" -delete 
	find -type f -name "*.out" -delete 
	find -type f -wholename "./bin/*" -delete 
	touch bin/.gitkeep
