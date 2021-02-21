#pragma once
#ifdef _WIN32
#include <winsock2.h>
#include <ws2tcpip.h>
#else
#include <sys/types.h>
#include <sys/socket.h>
#include <netdb.h>
#include <netinet/in.h>
#include <arpa/inet.h>
#include <netdb.h>
#include <netinet/in.h>
#include <sys/socket.h>
#include <unistd.h>
#endif // #ifdef _WIN32

struct udp_address_t {
	int proto;
	struct sockaddr* addr;
	socklen_t len;
	struct sockaddr_in* addr_in;
	socklen_t len_in;
	struct sockaddr6* addr6;
	socklen_t len6;
	struct sockaddr_in6* addr_in6;
	socklen_t len_in6;
	
	udp_address_t();
	~udp_address_t();
	struct addrinfo* crap;
};

typedef void (*udp_callback_t)(void* userdata, void* data, int length, udp_address_t from_hint);

struct UDP {
	static udp_address_t Lookup(const char* hostname);
	static void Send(const void* buffer, int length, udp_address_t addr = {});
	static void Listen(udp_callback_t callback, void* userdata=0);
	static void UnListen(udp_callback_t callback, void* userdata=0);
};
