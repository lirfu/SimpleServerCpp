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
