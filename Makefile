CC = g++
CFLAGS = -Wall -g -Werror -std=c++11

COMMON_OBJ = send_recv_file.o

all: server subscriber

server: server.cpp $(COMMON_OBJ)
	$(CC) $(CFLAGS) -o server server.cpp $(COMMON_OBJ)

subscriber: subscriber.cpp $(COMMON_OBJ)
	$(CC) $(CFLAGS) -o subscriber subscriber.cpp $(COMMON_OBJ)

send_recv_file.o: send_recv_file.cpp send_recv_file.h
	$(CC) $(CFLAGS) -c send_recv_file.cpp

clean:
	rm -f *.o server subscriber 325CA_Stoica_Mihai-Bogdan_Tema2.zip

pack:
	zip -r 325CA_Stoica_Mihai-Bogdan_Tema2.zip Makefile server.cpp subscriber.cpp send_recv_file.cpp send_recv_file.h utils.h README.md readme.txt

.PHONY: clean
