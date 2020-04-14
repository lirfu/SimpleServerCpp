#pragma once

#include <iostream>
#include <vector>

#include <cerrno>  // Get error number
#include <cstring>  // Ger error string

#include <unistd.h>  // Defines for functions for stream reading, closing and sleep
#include <sys/socket.h>  // Defines for functions for sockets.
#include <netinet/in.h>  // Defines for structs like sockaddr_in
#include <arpa/inet.h>  // For function inet_pton

#define MESSAGES_VERSION 00010001  // Lowest four digits are minor, rest is major. If major isn't same, messages aren't compatible.

#define ensure(B) ({ if (B) { Close(); return;} })

/** Handshake. */
typedef struct M
{
    uint msg_version;
    uint block_size;
    uint data_length;
    // char data[1];
} Message;

class ICommunicator
{
private:

    /*uint CalculateMessageSize(uint data_length)
    {
        return sizeof(Message) + data_length * sizeof(char) - 1;
    }

    Message * ReserveEmptyDataMsg(uint block_size, uint data_length)
    {
        Message * m = (Message *) malloc( CalculateMessageSize(data_length) );
        m->msg_version = 0; // FIXME: MESSAGES_VERSION;
        m->block_size = block_size;
        m->data_length = data_length;
        return m;
    }*/

protected:

    int socket_descriptor = -1;
	struct sockaddr_in server_address;

	const uint RETRY_ATTEMPTS;
    const uint RETRY_PERIOD_MS;
	int read_attempts = 0, write_attempts = 0;

    Message read_desc_;
    Message write_desc_;
    char * read_buffer_;

public:

    ICommunicator(uint retry_num = 5, uint retry_period_ms = 500)
        : RETRY_ATTEMPTS(retry_num), RETRY_PERIOD_MS(retry_period_ms * 1000)
    {
        read_desc_ = {.msg_version=MESSAGES_VERSION, .block_size=sizeof(Message), .data_length=sizeof(Message) / sizeof(char)};  // ReserveEmptyDataMsg(1024, 0);
        write_desc_ = {.msg_version=MESSAGES_VERSION, .block_size=sizeof(Message), .data_length=0};  // ReserveEmptyDataMsg(1024, 0);
        read_buffer_ = new char[sizeof(Message) / sizeof(char)];
    }

    ~ICommunicator()
    {
        delete read_buffer_;
    }

protected:

    int ReadBlock(int socket, char * buffer, uint buffer_lenght)
	{
		do {
			int read_bytes = read(socket, buffer, buffer_lenght);
            //std::cout<<"---------- Reading block: "<<read_bytes<<'B'<<std::endl;
			if (read_bytes >= 0)
			{
				read_attempts = 0;
				return read_bytes;
			}
			std::cerr << "Error reading - attempt " << ++read_attempts << '/' << RETRY_ATTEMPTS << std::endl;
			usleep(RETRY_PERIOD_MS);
		} while (read_attempts < RETRY_ATTEMPTS);

		return -1;
	}

	int SendBlock(int socket, const char * buffer, uint buffer_lenght)
	{
		do {
			int write_bytes = send(socket , buffer , buffer_lenght , 0);  // Zero means no flags.
            //std::cout<<"---------- Sending block: "<<write_bytes<<'B'<<std::endl;
			if (write_bytes >= 0)
			{
				write_attempts = 0;
				return write_bytes;
			}
			std::cerr << "Error sending - attempt " << ++write_attempts << '/' << RETRY_ATTEMPTS << std::endl;
			usleep(RETRY_PERIOD_MS);
		} while (write_attempts < RETRY_ATTEMPTS);

		return -1;
	}

public:

    int ReadMessage(int socket, char *& msg, uint & length)
    {
        uint block_size = read_desc_.block_size;
        uint data_max_size = read_desc_.data_length * sizeof(char);

        uint pck_num;
        ReadBlock(socket, (char *) &pck_num, sizeof(uint));
        pck_num++;
        //std::cout << " = Reading packages #" << pck_num << std::endl;

        uint read_size = 0;
        uint read_total = 0;
        int di = block_size / sizeof(char);  // NOTE: Block size must be multiple of char size.
        for(int i = 0; i < pck_num; i++)
        {
            read_size = ReadBlock(socket, read_buffer_ + i * di, std::min(block_size, data_max_size - i * block_size));
            if(read_size < 0) return -1;
            read_total += read_size;
            //std::cout << "  --> package #" << i << " = B" << read_size << std::endl;
        }

        // TODO Recognize handshake message and automatically adjust buffers.

        msg = read_buffer_;
        length = read_total / sizeof(char);

        return 0;
    }

    int SendMessage(int socket, const char * msg, uint msg_len)
    {
        uint block_size = write_desc_.block_size;
        uint data_size = msg_len * sizeof(char);

        if (msg_len > write_desc_.data_length)
        {
            //std::cerr << "Too much data to send, the overflow will be ignored (" << msg_len << '>' << write_desc_.data_length << ')' << std::endl;
            data_size = write_desc_.data_length * sizeof(char);
        }

        // Send the package number.
        uint pck_num = data_size / block_size;
        //std::cout << " = Sending packages #" << pck_num << std::endl;
        //std::cout << "  --> package # " << " = B" << sizeof(uint) << std::endl;
        SendBlock(socket, (char *) &pck_num, sizeof(uint));

        // Send message by blocks.
        int di = block_size / sizeof(char);  // NOTE: Block size must be multiple of char size.
        int i = 0;
        int sent_bytes;
        for(i = 0; sent_bytes >= 0 && i < pck_num; i++)
        {
            //std::cout << "  --> package #" << i << " = B" << block_size << std::endl;
            sent_bytes = SendBlock(socket, (char *) msg + i * di, block_size);
        }

        // Send rest (smaller than the block).
        //std::cout << "  --> package #" << i << " = B" << data_size - i * block_size << std::endl;
        sent_bytes = SendBlock(socket, (char *) msg + i * di, data_size - i * block_size);

        return 0;
    }

    int ReadHandshake(int socket)
    {
        uint length;
        if (ReadMessage(socket, read_buffer_, length) < 0) return -1;

        Message * desc = (Message *) read_buffer_;
        bool update = desc->data_length >= read_desc_.data_length;

        read_desc_.block_size = desc->block_size;
        read_desc_.data_length = desc->data_length;
        if (update)  // Correct buffer size.
        {
            read_buffer_ = (char *) realloc(read_buffer_, desc->data_length * sizeof(char));
        }

        //std::cout<<"H Received: block="<<read_desc_.block_size<< " data_len="<<read_desc_.data_length<<std::endl;

        return 0;
    }

    int SendHandshake(int socket, uint data_length, uint block_size=1024)
    {
        write_desc_.block_size = block_size;
        write_desc_.data_length = data_length;
        if (SendMessage(socket, (char *) &write_desc_, sizeof(Message)) < 0) return -1;
        //std::cout<<"H Sent: block="<<write_desc_.block_size<< " data_len="<<write_desc_.data_length<<std::endl;
        return 0;
    }

    virtual void Close()
    {
		close(socket_descriptor);
    }

    // static const bool CheckMessageCompat(const HandshakeMessage &msg)
    // {
    //     return MESSAGES_VERSION / 10000 != msg.msg_version / 10000;
    // }
};


class Client : public ICommunicator
{
public:
    int Initialize(int port)
    {
        // Type of socket: AF_UNIX for processes on the same system, AF_INET for the Internet domain.
		// Type of channel: SOCK_STREAM reads data as continuous streams, SOCK_DGRAM reads using chunks (datagrams)
		// Protocol: 0 means automatic (TCP for streams, UDP for datagrams)
		socket_descriptor = socket(AF_INET, SOCK_STREAM, 0);
		if(socket_descriptor == 0)
		{
			std::cerr << "Failed creating socket! - " << std::strerror(errno) << std::endl;
			return -1;
		}

	    server_address.sin_family = AF_INET;  // Always use this.
	    server_address.sin_port = htons( port );  // Port, converted to network-order short.

        // Convert IPv4 and IPv6 addresses from text to binary form
        if(inet_pton(AF_INET, "127.0.0.1", &server_address.sin_addr) <= 0)
        {
            std::cerr << "Invalid address! - " << std::strerror(errno) << std::endl;
            return -1;
        }

		return 0;
    }

    const int Connect()
    {
        if (connect(socket_descriptor, (struct sockaddr *) & server_address, sizeof(server_address)) < 0)
        {
            std::cerr << "Failed connecting to server! - " << std::strerror(errno) << std::endl;
			return -1;
        }

        return 0;
    }
};


class Server : public ICommunicator
{
public:
    int Initialize(int port, int connection_queue_size)
    {
        // Type of socket: AF_UNIX for processes on the same system, AF_INET for the Internet domain.
		// Type of channel: SOCK_STREAM reads data as continuous streams, SOCK_DGRAM reads using chunks (datagrams)
		// Protocol: 0 means automatic (TCP for streams, UDP for datagrams)
		socket_descriptor = socket(AF_INET, SOCK_STREAM, 0);
		if(socket_descriptor == 0)
		{
			std::cerr << "Failed creating socket! - " << std::strerror(errno) << std::endl;
			return -1;
		}

		// Forcefully attaching socket to the address and port.
		int opt = 1;
	    if (setsockopt(socket_descriptor, SOL_SOCKET, SO_REUSEADDR, &opt, sizeof(opt)) ||
			setsockopt(socket_descriptor, SOL_SOCKET, SO_REUSEPORT, &opt, sizeof(opt)))
	    {
			std::cerr << "Failed setting socket options! - " << std::strerror(errno) << std::endl;
			return -1;
	    }


	    server_address.sin_family = AF_INET;  // Always use this.
	    server_address.sin_addr.s_addr = INADDR_ANY;  // For localhost.
	    server_address.sin_port = htons( port );  // Port, converted to network-order short.
	    if (bind(socket_descriptor, (struct sockaddr *) &server_address, sizeof(server_address)) < 0)
	    {
			std::cerr << "Failed binding socket! - " << std::strerror(errno) << std::endl;
			return -1;
	    }

        // Listen to port.
        if (listen(socket_descriptor, connection_queue_size) < 0)
	    {
			std::cerr << "Failed listening socket! - " << std::strerror(errno) << std::endl;
			return -1;
	    }

		return 0;
    }

    /** Returns client socket descriptor or -1 if error. Stores descriptor to member variable as well. */
    int Connect(int & client)
    {
		int addr_len = sizeof(server_address);
        client = accept(socket_descriptor, (struct sockaddr *) &server_address, (socklen_t *) &addr_len);
		if (client < 0)
		{
			std::cerr << "Failed starting connection! - " << std::strerror(errno) << std::endl;
			return -1;
		}
		return 0;
    }
};
