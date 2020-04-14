// Client side C/C++ program to demonstrate Socket programming
#include <iostream>
#include <fstream>
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

        int server = socket_descriptor;

        char msg[] = "Ok! Send me the image.\0";
        uint length = strlen(msg);

        ensure(SendHandshake(server, length, 1024));
        ensure(ReadHandshake(server));


        ensure(SendMessage(server, msg, length));

        uint size;
        char * buffer;
        ensure(ReadMessage(server, buffer, size));

        std::cout << "Got (length " << size << "): " << std::endl;

        std::ofstream outfile("../out.png", std::ofstream::binary);
        outfile.write(buffer, size);
        outfile.close();

        Close();
        std::cout << "Exiting." << std::endl;
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
    for(int i=0; i<1; i++)
    client.DoWork();
    return 0;
}
