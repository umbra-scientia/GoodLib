#pragma once
#ifdef _WIN32
#include <WinSock2.h>
#include <Ws2tcpip.h>
#else
#include <sys/types.h>
#include <sys/socket.h>
#include <netdb.h>
#include <netinet/in.h>
#include <arpa/inet.h>
#include <unistd.h>
#endif // #ifdef _WIN32

struct udp_address_t {
    struct sockaddr* addr;
    socklen_t len;
};

typedef void (*udp_callback_t)(void* data, int length, udp_address_t from_hint);

struct UDP {
    static udp_address_t Lookup(const char* hostname);
    static void Send(const void* buffer, int length, udp_address_t addr);
    static void Listen(udp_callback_t callback);
    static void UnListen(udp_callback_t callback);
};
