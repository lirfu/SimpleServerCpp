#pragma once

#include <iostream>
#include <vector>

#include <cerrno>  // Get error number
#include <cstring>  // Ger error string

#include <unistd.h>  // Defines for functions for stream reading, closing and sleep
#include <sys/socket.h>  // Defines for functions for sockets.
#include <netinet/in.h>  // Defines for structs like sockaddr_in
#include <arpa/inet.h>  // For function inet_pton

//#define DEBUG_SOCKETSERVER

namespace comms {

#define MESSAGES_VERSION 00010001  // Lowest four digits are minor, rest is major. If major isn't same, messages aren't compatible.

/** Handshake. */
typedef struct M
{
    uint msg_version;
    uint block_size;
    uint data_length;
} Message;

class Client;
class Server;

class ICommunicator
{
protected:
    int socket_descriptor = -1;
	struct sockaddr_in server_address;

	const uint RETRY_ATTEMPTS;
    const uint RETRY_PERIOD_MS;
	uint read_attempts = 0, write_attempts = 0;

    Message read_desc_;
    Message write_desc_;
    std::shared_ptr<char> read_buffer_;

public:

    ICommunicator(uint retry_num = 5, uint retry_period_ms = 500)
        : RETRY_ATTEMPTS(retry_num), RETRY_PERIOD_MS(retry_period_ms * 1000)
    {
        read_desc_ = {.msg_version=MESSAGES_VERSION, .block_size=sizeof(Message), .data_length=sizeof(Message) / sizeof(char)};
        write_desc_ = {.msg_version=MESSAGES_VERSION, .block_size=sizeof(Message), .data_length=sizeof(Message) / sizeof(char)};
        read_buffer_ = std::shared_ptr<char>(new char[sizeof(Message) / sizeof(char)], std::default_delete<char[]>());
    }

    ~ICommunicator()
    {}


protected:


    /** Read a single block of given length from given socket into the given buffer. If it doesn't succeed, it retries several times with pauses (both values given in construction). */
    int ReadBlock(int socket, char * buffer, uint buffer_lenght)
	{
        read_attempts = 0;
		do {
			int read_bytes = read(socket, buffer, buffer_lenght);
#ifdef DEBUG_SOCKETSERVER
            std::cout << "---------- Reading block: " << read_bytes << 'B' << std::endl;
#endif
			if (read_bytes >= 0)
			{
				return read_bytes;
			}
            ++read_attempts;
			std::cerr << "Error reading block - attempt " << read_attempts << '/' << RETRY_ATTEMPTS << std::endl;
			usleep(RETRY_PERIOD_MS);
		} while (read_attempts < RETRY_ATTEMPTS);

		return -1;
	}


    /** Send a single block of given length to given socket from the given buffer. If it doesn't succeed, it retries several times with pauses (both values given in construction). */
	int SendBlock(int socket, const char * buffer, uint buffer_lenght)
	{
        write_attempts = 0;
		do {
			int write_bytes = send(socket , buffer , buffer_lenght , 0);  // Zero means no flags.
#ifdef DEBUG_SOCKETSERVER
            std::cout << "---------- Sending block: " << write_bytes << 'B' << std::endl;
#endif
			if (write_bytes >= 0)
			{
				return write_bytes;
			}
            ++write_attempts;
			std::cerr << "Error sending block - attempt " << write_attempts << '/' << RETRY_ATTEMPTS << std::endl;
			usleep(RETRY_PERIOD_MS);
		} while (write_attempts < RETRY_ATTEMPTS);

		return -1;
	}


public:

    /** Read a message block-by-block from a given socket into the internal buffer.
      * The internal buffer reference and message length in char number is returned through given arguments.
      * Internally it first reads the number of packages, then reads the packages
      * and finally ends when reading the final (empty) package.
      * @return 0 on success, -1 on error */
    int ReadMessage(int socket, std::shared_ptr<char>& msg, uint & length)
    {
        int block_size = read_desc_.block_size;
        int data_max_size = read_desc_.data_length * sizeof(char);

        // Read the expected number of packages. Add one for the last package (the fragmented one or empty).
        int data_size;

        if (ReadBlock(socket, (char *) &data_size, sizeof(data_size)) <= 0) return -1;  // Be strict about package size (when connection breaks, could be a random number).
        data_size = std::min(data_size, data_max_size);

#ifdef DEBUG_SOCKETSERVER
        std::cout << " = Reading packages ~#" << (int)(data_size / block_size) << "  (length " << data_size << ")" << std::endl;
#endif

        int read_bytes = 0;
        int total_read = 0;
        while (total_read < data_size)
        {
            read_bytes = ReadBlock(socket, read_buffer_.get() + total_read, std::min(block_size, data_size - total_read));
            if (read_bytes < 0) return -1;  // On error, return.
            total_read += read_bytes;
#ifdef DEBUG_SOCKETSERVER
            std::cout << "   --> " << data_size - total_read << "B remaining to read" << std::endl;
#endif
        }

        msg = read_buffer_;
        length = total_read;
        return 0;
    }


    int SendMessage(int socket, const char * msg, uint msg_len)
    {
        int block_size = write_desc_.block_size;
        int data_size = msg_len * sizeof(char);

        if (msg_len > write_desc_.data_length)
        {
#ifdef DEBUG_SOCKETSERVER
            std::cerr << "! Too much data to send, the overflow will be ignored (" << msg_len << '>' << write_desc_.data_length << ')' << std::endl;
#endif
            data_size = write_desc_.data_length * sizeof(char);
        }

        // Send the package number and length.
#ifdef DEBUG_SOCKETSERVER
        int pck_num = data_size / block_size;
        std::cout << " = Sending packages #" << pck_num << "  (length " << data_size << ")" << std::endl;
        std::cout << "  --> package (initial)" << " = B" << sizeof(data_size) << std::endl;
#endif
        if (SendBlock(socket, (char *) &data_size, sizeof(data_size)) < 0) return -1;

        int sent_bytes = 0;
        int total_sent = 0;
        while (total_sent < data_size)
        {
            sent_bytes = SendBlock(socket, (char *) msg + total_sent, std::min(block_size, data_size - total_sent));
            if (sent_bytes < 0) return -1;  // On error, return.
            total_sent += sent_bytes;
#ifdef DEBUG_SOCKETSERVER
            std::cout << "   --> " << data_size - total_sent << "B remaining to send" << std::endl;
#endif
        }

        return 0;
    }


    int ReadHandshake(int socket)
    {
#ifdef DEBUG_SOCKETSERVER
        std::cout<<"H Waiting on handshake: block="<<read_desc_.block_size<< " data_len="<<read_desc_.data_length<<std::endl;
#endif
        uint length;
        if (ReadMessage(socket, read_buffer_, length) < 0) return -1;
        Message * desc = (Message *) read_buffer_.get();

#ifdef DEBUG_SOCKETSERVER
        std::cout<<"H Received: block="<<read_desc_.block_size<< " data_len="<<read_desc_.data_length<<std::endl;
#endif

        bool update = desc->data_length != read_desc_.data_length;
        read_desc_.block_size = desc->block_size;
        read_desc_.data_length = desc->data_length;

        if (update && desc->data_length > 0)  // Correct buffer size.
        {
            read_buffer_ = std::shared_ptr<char>(new char[desc->data_length * sizeof(char)], std::default_delete<char[]>());
        }
        return 0;
    }


    int SendHandshake(int socket, uint data_length, uint block_size=1024)
    {
        Message tmp = {.msg_version=MESSAGES_VERSION, .block_size = block_size, .data_length = data_length};
#ifdef DEBUG_SOCKETSERVER
        std::cout<<"H Sending handshake: block="<<tmp.block_size<< " data_len="<<tmp.data_length<<std::endl;
#endif
        if (SendMessage(socket, (char *) &tmp, sizeof(Message)) < 0) return -1;
        write_desc_.block_size = block_size;
        write_desc_.data_length = data_length;
#ifdef DEBUG_SOCKETSERVER
        std::cout<<"H Sent: block="<<tmp.block_size<< " data_len="<<tmp.data_length<<std::endl;
#endif
        return 0;
    }

    void Close()
    {
#ifdef DEBUG_SOCKETSERVER
        std::cout << "Closing socket: " << socket_descriptor << std::endl;
#endif
        shutdown(socket_descriptor, SHUT_RDWR);  // To kill the accept if it hangs.
		close(socket_descriptor);
    }

    constexpr int GetSocket()
    {
        return socket_descriptor;
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
		if(socket_descriptor < 0)
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

    int Connect()
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
private:
    const int client_queue_size_ = 1;
    int client_descriptor = -1;
public:

    constexpr int GetClientSocket()
    {
        return client_descriptor;
    }

    int Initialize(int port)
    {
        // Type of socket: AF_UNIX for processes on the same system, AF_INET for the Internet domain.
		// Type of channel: SOCK_STREAM reads data as continuous streams, SOCK_DGRAM reads using chunks (datagrams)
		// Protocol: 0 means automatic (TCP for streams, UDP for datagrams)
		socket_descriptor = socket(AF_INET, SOCK_STREAM, 0);
		if(socket_descriptor < 0)
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
        if (listen(socket_descriptor, client_queue_size_) < 0)
	    {
			std::cerr << "Failed listening socket! - " << std::strerror(errno) << std::endl;
			return -1;
	    }

		return 0;
    }

    /** Stores descriptor to member variable. Returns -1 on error. */
    int Connect()
    {
		int addr_len = sizeof(server_address);
        client_descriptor = accept(socket_descriptor, (struct sockaddr *) &server_address, (socklen_t *) &addr_len);
		if (client_descriptor < 0)
		{
			std::cerr << "Failed connecting to client! - " << std::strerror(errno) << std::endl;
			return -1;
		}
		return 0;
    }

    void Close()
    {
        shutdown(client_descriptor, SHUT_RDWR);  // To kill the accept if it hangs.
        close(client_descriptor);
        ICommunicator::Close();
    }
};

}
