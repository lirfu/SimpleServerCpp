#pragma once

#include "ThreadedDuplex.h"
#include "SocketChannel.h"

namespace comms
{

class DuplexSocketCommunication : public ThreadedDuplex
{
private:
    SocketInputChannel input;
    SocketOutputChannel output;

public:
    DuplexSocketCommunication() {}

    DuplexSocketCommunication(int in_port, int out_port)
    {
        input.SetPort(in_port);
        output.SetPort(out_port);
    }

    DuplexSocketCommunication(std::string in_addr, int in_port, std::string out_addr, int out_port)
    {
        input.SetAddress(in_addr);
        input.SetPort(in_port);
        output.SetAddress(out_addr);
        output.SetPort(out_port);
    }

    DuplexSocketCommunication(std::string& in_addr, int in_port, std::string& out_addr, int out_port)
    {
        input.SetAddress(in_addr);
        input.SetPort(in_port);
        output.SetAddress(out_addr);
        output.SetPort(out_port);
    }

    ~DuplexSocketCommunication()
    {
        input.Close();
        output.Close();
    }

    SocketInputChannel & Input()
    {
        return input;
    }

    SocketOutputChannel & Output()
    {
        return output;
    }
};

}
