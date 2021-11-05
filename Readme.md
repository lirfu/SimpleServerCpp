# SimpleServerCpp
A very rudimentary server-client socket based communication.

Inspired by a [linuxhowtos tutorial](http://www.linuxhowtos.org/C_C++/socket.htm).

## Building
A Makefile is provided so just run:
```
make
```
Only basic C++ tools are required, no additional dependencies.

## Components


# Demos

## Send image demo
A demonstration of sending and reading raw data (image) across a common socket. The input image is in `res/img.png` and is outputted as `res/output.png`.

## Messenger
A rudimentary messenger over the network.