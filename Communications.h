#pragma once

#include <iostream>
#include <cerrno>
#include <cstring>
#include <string.h>

//#include <stdlib.h>
#include <unistd.h>  // Defines for functions for stream reading, closing and sleep
#include <sys/socket.h>  // Defines for functions for sockets.
#include <netinet/in.h>  // Defines for structs like sockaddr_in
#include <arpa/inet.h>  // For function inet_pton

class ICommunicator
{
protected:

    int socket_descriptor;
	struct sockaddr_in server_address;

	static const int ALLOWED_ATTEMPTS = 5;
    static const int RETRY_PERIOD_MS = 500000u;
	int read_attempts, write_attempts;

public:

    int Read(int socket, char * buffer, uint buffer_lenght)
	{
		do {
			int read_bytes = read(socket, buffer, buffer_lenght);

			if (read_bytes > 0)
			{
				read_attempts = 0;
				return read_bytes;
			}
			std::cerr << "Error reading - attempt " << ++read_attempts << '/' << ICommunicator::ALLOWED_ATTEMPTS << std::endl;
			usleep(RETRY_PERIOD_MS);
		} while (read_attempts < ICommunicator::ALLOWED_ATTEMPTS);

		return -1;
	}

	int Send(int socket, const char * buffer, uint buffer_lenght)
	{
		do {
			int write_bytes = send(socket , buffer , buffer_lenght , 0);  // Zero means no flags.
			if (write_bytes > 0)
			{
				write_attempts = 0;
				return write_bytes;
			}
			std::cerr << "Error sending - attempt " << ++write_attempts << '/' << ICommunicator::ALLOWED_ATTEMPTS << std::endl;
			usleep(RETRY_PERIOD_MS);
		} while (write_attempts < ICommunicator::ALLOWED_ATTEMPTS);

		return -1;
	}
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


    const int Connect(int & client_socket)
    {
		int addr_len = sizeof(server_address);
        client_socket = accept(socket_descriptor, (struct sockaddr *) &server_address, (socklen_t *) &addr_len);
		if (client_socket < 0)
		{
			std::cerr << "Failed starting connection! - " << std::strerror(errno) << std::endl;
			return -1;
		}

		return 0;
    }
};
