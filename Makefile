
.PHONY : all
all : client server

bindir :
	mkdir -p bin

.PHONY : client
client : bindir client/client.cpp
	$(dir_guard)
	g++ client/client.cpp -o bin/client

.PHONY : server
server : bindir server/server.cpp
	$(dir_guard)
	g++ server/server.cpp -o bin/server
