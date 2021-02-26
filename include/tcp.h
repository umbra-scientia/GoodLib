#pragma once
#include <string>
#include <thread>
#ifdef _WIN32
#include <winsock2.h>
#include <ws2tcpip.h>
#else
#include <arpa/inet.h>
#include <netdb.h>
#include <netinet/in.h>
#include <sys/socket.h>
#include <sys/types.h>
#include <unistd.h>
#endif // #ifdef _WIN32
#include "types.h"
#include "ref.h"

struct tcp_address_t : public RCObj {
	// FIXME: pass by value would be nice, but G++ is trash and can't call the copy constructor right. enjoy.
	static Ref<tcp_address_t> Lookup(const char* hostname, int port);

	tcp_address_t();
	//tcp_address_t(tcp_address_t* a);
	~tcp_address_t();
	std::string ToString() const;
	
	struct addrinfo* addr_info;
	struct sockaddr* addr;
	socklen_t len;
	int ownership;
	int proto;
	int port;
};

struct TCPConnection {
	TCPConnection(int fd);
	TCPConnection(Ref<tcp_address_t> ad);
	~TCPConnection();
	bool Valid();
	int Read(void* buffer, int len);
	int Write(const void* buffer, int len);
	
	tcp_address_t* addey;
	int fd;
	bool valid;
};

struct TCPListener {
	TCPListener(int port);
	~TCPListener();
	void Wait();

	virtual bool OnConnect(TCPConnection* con, Ref<tcp_address_t> from_hint) = 0;
	std::thread* worker;
	void workerfn();
	int port;
	bool valid;
};
