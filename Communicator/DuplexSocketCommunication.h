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
