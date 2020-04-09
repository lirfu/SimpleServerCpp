// Server side C/C++ program to demonstrate Socket programming
#include <iostream>
#include <csignal>

#include "../Communications.h"

class MyServer : public Server
{

public:
	MyServer() {}

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

		char* msg = "SERVER veli bok!";

		int buffer_size = 1024;
		char buffer[buffer_size] = {0};

		while (true)
		{
			std::cout << "Waiting for input!" << std::endl;

			if (Read(client, buffer, buffer_size) < 0)
				break;

			std::cout << buffer << std::endl;

			if (Send(client, msg, strlen(msg)) < 0)
				break;

			usleep(100000u);
		}

		close(client);
		close(socket_descriptor);

		std::cout << "Exited!" << std::endl;
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
