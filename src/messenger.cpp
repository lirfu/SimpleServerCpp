#include <iostream>
#include <string>

#include "DuplexSocketCommunication.h"
#include "Utils.h"

#define DEFAULT_PORT 3000

comms::DuplexSocketCommunication com;

void read()
{
	RegularTicker ticker(33 * 1000);  // 33ms ticker
	ticker.Start();
	for (int i=1; i<=100 && com.Input().Initialize(); i++)
	{
		std::cerr << "Cannot initialize input channel! Attempt " << i << " / 100" << std::endl;
		ticker.Sleep();
	}

	uint size;
	std::shared_ptr<char> buffer;
	while (true)
	{
		if ( com.Input().Read(buffer, size) )
		{
			std::cerr << "Closing connection." << std::endl;
			return;
		}

		std::cout << ">>> " << buffer << std::endl;
	}
}

void write()
{
	uint L = 1024;
	char* input = new char[L];
	com.Output().SetBufferLength(1024);
	com.Output().SetDataLength(L);
	if ( com.Output().Initialize() )
	{
		std::cerr << "Cannot initialize output channel!" << std::endl;
		return;
	}

	std::cout << std::endl << "   Conversation started   " << std::endl << "--------------------------" << std::endl;

	while (true)
	{
		std::cin >> input;
		if ( com.Output().Write(input, L) )
		{
			std::cerr << "Closing connection!" << std::endl;
			return;
		}
	}
}

int main(int argc, char** argv)
{
	if (argc == 1)
	{
		com.Input().SetPort(DEFAULT_PORT);
		com.Output().SetPort(DEFAULT_PORT);
		std::cout << "Connecting to localhost on port " << DEFAULT_PORT << std::endl;
	}
	else if (argc == 2)
	{
		int port = std::stoi(argv[1]);
		com.Input().SetPort(port);
		com.Output().SetPort(port);
		std::cout << "Connecting to localhost on port " << port << std::endl;
	}
	else if (argc == 3)
	{
		int port = std::stoi(argv[2]);
		std::string addr(argv[1]);
		com.Input().SetAddress(addr);
		com.Output().SetAddress(addr);
		com.Input().SetPort(port);
		com.Output().SetPort(port);
		std::cout << "Connecting to " << addr << " on port " << port << std::endl;
	}
	else
	{
		std::cerr << "Please provide a port or IP address and port!" << std::endl;
		return 1;
	}

	com.Start(read, write);
	com.JoinThreads();

	return 0;
}