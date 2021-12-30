#include <iostream>
#include <string>

#include "DuplexSocketCommunication.h"
#include "Utils.h"

#define DEFAULT_PORT 3000
int IN_PORT=DEFAULT_PORT, OUT_PORT=DEFAULT_PORT;
std::string IN_ADDR(LOCALHOST);
comms::DuplexSocketCommunication com;

void read()
{
	RegularTicker ticker(1000 * 1000);  // 1s ticker
	ticker.Start();
	for (int i=1; i<=100 && com.Input().Initialize(IN_ADDR, IN_PORT); i++)
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
	if ( com.Output().Initialize(LOCALHOST, OUT_PORT) )
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
		std::cout << "Connecting to " << IN_ADDR << " on port " << IN_PORT << std::endl;
	}
	else if (argc == 3)
	{
		IN_PORT = std::stoi(argv[1]);
		OUT_PORT = std::stoi(argv[2]);
		std::cout << "Connecting to " << IN_ADDR << " on port " << IN_PORT << std::endl;
	}
	else if (argc == 4)
	{
		IN_ADDR = std::string(argv[1]);
		IN_PORT = std::stoi(argv[2]);
		OUT_PORT = std::stoi(argv[3]);
		std::cout << "Connecting to " << IN_ADDR << " on port " << IN_PORT << std::endl;
	}
	else
	{
		std::cerr << "Please provide nothing, input and output port or input IP address, port and output port!" << std::endl;
		return 1;
	}

	com.Start(read, write);
	com.JoinThreads();

	return 0;
}