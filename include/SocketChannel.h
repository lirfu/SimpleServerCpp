#pragma once

#include <iostream>
#include <csignal>

#include "Channel.h"
#include "SocketServer.h"

namespace comms
{

template <typename C>
class ASocketChannel : public AChannel
{
protected:
    int port;
    uint buffer_size;
    uint data_length;
    C channel;

    static void CatchSignal(int signum)
    {
        std::cerr << "[COMMS] Socket caught signal: " << signum << " - " << std::strerror(errno) << std::endl;
        // TODO: Handle socket closing.
    }

public:
    void Close() override
    {
        channel.Close();
    }

    void Setup(uint buffer_length, uint data_length)
    {
        buffer_size = buffer_length;
        data_length = data_length;
    }

    void SetBufferLength(uint length)
    {
        buffer_size = length;
    }

    void SetDataLength(uint length)
    {
        data_length = length;
    }
};

class SocketInputChannel : public ASocketChannel<Client>
{
public:
    ~SocketInputChannel()
    {
        Close();
    }

    virtual int Initialize(const std::string& address, int port) override
    {
        signal(SIGPIPE, CatchSignal);

		if ( channel.Initialize(port, address) )
		{
			std::cerr << "[COMMS] Input initialization failed!" << std::endl;
			return -1;
		}

		std::cout << "[COMMS] Waiting on input connection..." << std::endl;
		if ( channel.Connect() )
		{
			std::cerr << "[COMMS] Input connection failed!" << std::endl;
			return -1;
		}
		std::cout << "[COMMS] Input connected!" << std::endl;

		// Handshakes.
        if ( channel.ReadHandshake(channel.GetSocket()) )
        {
            std::cerr << "[COMMS] Error reading input handsake." << std::endl;
            return -1;
        }
        if ( channel.SendHandshake(channel.GetSocket(), data_length, buffer_size) )
		{
			std::cerr << "[COMMS] Error sending input handsake." << std::endl;
			return -1;
		}

        return 0;
    }

    virtual int Read(std::shared_ptr<char> & msg, uint & length) override
    {
        return channel.ReadMessage(channel.GetSocket(), msg, length);
    }

    virtual int Write(char * msg, uint length) override
    {
        return channel.SendMessage(channel.GetSocket(), msg, length);
    }
};

class SocketOutputChannel : public ASocketChannel<Server>
{
public:
    ~SocketOutputChannel()
    {
        Close();
    }

    virtual int Initialize(const std::string& ignore="", int port=3000) override
    {
        signal(SIGPIPE, CatchSignal);

		if ( channel.Initialize(port) )
		{
			std::cerr << "[COMMS] Output initialization failed!" << std::endl;
			return -1;
		}

		std::cout << "[COMMS] Waiting on output connection..." << std::endl;
		if ( channel.Connect() )
		{
			std::cerr << "[COMMS] Output connection failed!" << std::endl;
			return -1;
		}
		std::cout << "[COMMS] Output connected!" << std::endl;

		// Handshakes.
        if ( channel.SendHandshake(channel.GetClientSocket(), data_length, buffer_size) )
        {
            std::cerr << "[COMMS] Error sending output handsake." << std::endl;
            return -1;
        }
        if ( channel.ReadHandshake(channel.GetClientSocket()) )
        {
            std::cerr << "[COMMS] Error reading output handsake." << std::endl;
            return -1;
        }

        return 0;
    }

    virtual int Read(std::shared_ptr<char> & msg, uint & length) override
    {
        return channel.ReadMessage(channel.GetClientSocket(), msg, length);
    }

    virtual int Write(char * msg, uint length) override
    {
        return channel.SendMessage(channel.GetClientSocket(), msg, length);
    }
};

}
