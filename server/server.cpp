// Server side C/C++ program to demonstrate Socket programming
#include <iostream>
#include <fstream>
#include <csignal>

#include "../Communications.h"

class MyServer : public Server
{
private:
	int img_size;
	char* img;

public:
	MyServer() {

		std::ifstream infile("../fig.png");
        infile.seekg(0, std::ios::end);
        img_size = infile.tellg();
        infile.seekg(0, std::ios::beg);

		img = new char[img_size] {0};
        infile.read(img, img_size);
	}

	void Serve()
	{
		signal(SIGPIPE, MyServer::CatchSignal);

		if(Initialize(3000, 1))
		{
			std::cerr << "Closing server." << std::endl;
			return;
		}

		std::cout << "Waiting on connection..." << std::endl;

		int client;
		if (Connect(client))
		{
			std::cerr << "Closing server." << std::endl;
			return;
		}

		std::cout << "Connection established!" << std::endl;

		std::cout << "Image size: " << img_size << std::endl;

		ensure(ReadHandshake(client));
		ensure(SendHandshake(client, img_size, 1024));  // Also sets up the sending buffers.

		uint size;
		char * buffer;
		ensure(ReadMessage(client, buffer, size));

		std::cout << "Got message (length " << size << "): " << buffer << std::endl;

		ensure(SendMessage(client, img, img_size));

		Close();
		std::cout << "Done!" << std::endl;
	}

private:

	static void CatchSignal(int signum)
	{
		std::cerr << "--> Caught signal: " << signum << std::endl;
	}
};

int main(int argc, char const *argv[])
{
	MyServer server;
	while (true) {
		server.Serve();
	}
    return 0;
}
