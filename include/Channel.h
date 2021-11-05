#pragma once

namespace comms
{

class AChannel {
public:
    virtual int Initialize(const std::string& address, int port) = 0;
    virtual int Read(std::shared_ptr<char> & msg, uint & length) = 0;
    virtual int Write(char * msg, uint length) = 0;
    virtual void Close() = 0;
};

}
