#include <iostream>
#include <fstream>
#include <memory>

#include "DuplexSocketCommunication.h"
#include "Utils.h"

comms::DuplexSocketCommunication com(3000, 3000);  // Send and read from same socket.

void send_image()
{
	// Read image from file.
	std::ifstream infile("res/img.png");
	infile.seekg(0, std::ios::end);
	int img_size = infile.tellg();
	infile.seekg(0, std::ios::beg);
	char* img = new char[img_size] {0};
	infile.read(img, img_size);

	com.Output().SetBufferLength(1024);
	com.Output().SetDataLength(img_size);
	if ( com.Output().Initialize() )
	{
		std::cerr << "Cannot initialize output channel!" << std::endl;
		return;
	}

	std::cout << "Sending image of size: " << img_size << "B" << std::endl;
	if ( com.Output().Write(img, img_size) )
	{
		std::cerr << "Cannot send image!" << std::endl;
		return;
	}

	delete img;
}

void read_image()
{
	RegularTicker ticker(33 * 1000);  // 33ms ticker
	ticker.Start();
	for (int i=1; i<=100 && com.Input().Initialize(); i++)
	{
		std::cerr << "Cannot initialize input channel! Attempt " << i << " / 100" << std::endl;
		ticker.Sleep();
	}

	uint size;
	std::shared_ptr<char> buffer;
	if ( com.Input().Read(buffer, size) )
	{
		std::cerr << "Couldn't read image!" << std::endl;
		return;
	}
	std::cout << "Read image of size: " << size << "B" << std::endl;

	// Write image to file.
	std::ofstream outfile("res/output.png", std::ofstream::binary);
	outfile.write(buffer.get(), size);
	outfile.close();
}

int main()
{
	com.Start(read_image, send_image);
	com.JoinThreads();
	std::cout << "Done!" << std::endl;
	return 0;
}