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
        std::cerr << "!!! Socket caught signal: " << signum << " - " << std::strerror(errno) << std::endl;
        // TODO: Handle socket closing.
    }

public:
    void Close() override
    {
        channel.Close();
    }

    void SetPort(int port_num)
    {
        port = port_num;
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

    virtual int Initialize() override
    {
        signal(SIGPIPE, CatchSignal);

		if ( channel.Initialize(port) )
		{
			std::cerr << "Input initialization failed!" << std::endl;
			return -1;
		}

		std::cout << "Waiting on input connection..." << std::endl;
		if ( channel.Connect() )
		{
			std::cerr << "Input connection failed!" << std::endl;
			return -1;
		}
		std::cout << "Input connected!" << std::endl;

		// Handshakes.
        if ( channel.ReadHandshake(channel.GetSocket()) )
        {
            std::cerr << "Error reading input handsake." << std::endl;
            return -1;
        }
        if ( channel.SendHandshake(channel.GetSocket(), data_length, buffer_size) )
		{
			std::cerr << "Error sending input handsake." << std::endl;
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

    virtual int Initialize() override
    {
        signal(SIGPIPE, CatchSignal);

		if ( channel.Initialize(port) )
		{
			std::cerr << "Output initialization failed!" << std::endl;
			return -1;
		}

		std::cout << "Waiting on output connection..." << std::endl;
		if ( channel.Connect() )
		{
			std::cerr << "Output connection failed!" << std::endl;
			return -1;
		}
		std::cout << "Output connected!" << std::endl;

		// Handshakes.
        if ( channel.SendHandshake(channel.GetClientSocket(), data_length, buffer_size) )
        {
            std::cerr << "Error sending output handsake." << std::endl;
            return -1;
        }
        if ( channel.ReadHandshake(channel.GetClientSocket()) )
        {
            std::cerr << "Error reading output handsake." << std::endl;
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
