
.PHONY : all
all : send_image messenger

bindir :
	mkdir -p bin

.PHONY : send_image
send_image : bindir src/send_image.cpp
	g++ src/send_image.cpp -o bin/send_image -Iinclude -pthread

.PHONY : messenger
messenger : bindir src/messenger.cpp
	g++ src/messenger.cpp -o bin/messenger -Iinclude -pthread
