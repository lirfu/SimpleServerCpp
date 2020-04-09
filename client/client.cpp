// Client side C/C++ program to demonstrate Socket programming
#include <iostream>
#include <csignal>

#include "../Communications.h"

class MyClient : public Client
{

public:

    MyClient() {}

    void DoWork()
    {
        signal(SIGPIPE, MyClient::CatchSignal);

        if(Initialize(3000))
        {
            std::cerr << "Closing server." << std::endl;
            return;
        }

        std::cout << "Waiting on connection..." << std::endl;

        if (Connect())
        {
            std::cerr << "Closing server." << std::endl;
            return;
        }

        std::cout << "Connection established!" << std::endl;

        int buffer_size = 1024;
        char buffer[buffer_size] = {0};
        char msg[] = "1 CLIENT veli bok!";

        int i = 0;
        while(++i < 10)
        {
            if (Send(socket_descriptor, msg, strlen(msg)) < 0)
				break;

            std::cout << "Waiting for input!" << std::endl;

            if (Read(socket_descriptor, buffer, buffer_size) < 0)
				break;

			std::cout << buffer << std::endl;

            msg[0]++;
            usleep(300000u);
        }

        std::cout << "Done." << std::endl;

        close(socket_descriptor);
    }

private:

    static void CatchSignal(int signum)
    {
        std::cerr << "--> Caught signal: " << signum << std::endl;
    }
};


int main(int argc, char const *argv[])
{
    MyClient client;
    client.DoWork();
    return 0;
}
