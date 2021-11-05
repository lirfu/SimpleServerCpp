#pragma once

namespace comms
{

class AChannel {
public:
    virtual int Initialize() = 0;
    virtual int Read(std::shared_ptr<char> & msg, uint & length) = 0;
    virtual int Write(char * msg, uint length) = 0;
    virtual void Close() = 0;
};

}
