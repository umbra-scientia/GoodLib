#include "util/types.h"
#ifdef _WIN32
#include <WinSock2.h>
#include <Ws2tcpip.h>
#else
#include <arpa/inet.h>
#include <netinet/in.h>
#include <sys/socket.h>
#include <unistd.h>
#endif // #ifdef _WIN32
#include <thread>

using namespace std;

int main(int argc, char* argv[]) {
#ifdef _WIN32
	WSAData data;
	WSAStartup(MAKEWORD(2, 2), &data);
#endif // #ifdef _WIN32

	thread recvThread([] {
		SOCKET in = socket(AF_INET, SOCK_DGRAM, IPPROTO_UDP);

		sockaddr_in addr;
		addr.sin_family = AF_INET;
		addr.sin_port = htons(25565);
		addr.sin_addr.s_addr = htonl(INADDR_ANY);
		bind(in, (SOCKADDR*)&addr, sizeof(addr));

		char buf[1024];
		sockaddr_in from;
		int fromSize = sizeof(from);
		auto len = recvfrom(in, buf, 1024, 0, (SOCKADDR*)&from, &fromSize);
		if (len == SOCKET_ERROR) { handleError(); }
		printf("received %i bytes", len);

#ifdef _WIN32
		closesocket(in);
#else
		close(in);
#endif // #ifdef _WIN32
	});

	SOCKET out = socket(AF_INET, SOCK_DGRAM, IPPROTO_UDP);

	sockaddr_in dest;
	dest.sin_family = AF_INET;
	dest.sin_port = htons(25565);
	inet_pton(AF_INET, "127.0.0.1", &dest.sin_addr.s_addr);

	auto msg = "ping";
	sendto(out, msg, strlen(msg), 0, (sockaddr*)&dest, sizeof(dest));

	recvThread.join();

#ifdef _WIN32
	closesocket(out);
#else
	close(out);
#endif // #ifdef _WIN32

#ifdef _WIN32
	WSACleanup();
#endif // #ifdef _WIN32
}

void handleError() {
#ifdef _WIN32
	wprintf(L"recvfrom failed with error %d\n", WSAGetLastError());
	wchar_t* s = NULL;
	FormatMessageW(FORMAT_MESSAGE_ALLOCATE_BUFFER | FORMAT_MESSAGE_FROM_SYSTEM | FORMAT_MESSAGE_IGNORE_INSERTS,
		NULL, WSAGetLastError(), MAKELANGID(LANG_NEUTRAL, SUBLANG_DEFAULT), (LPWSTR)&s, 0, NULL);
	printf("%S\n", s);
	LocalFree(s);
#endif // #ifdef _WIN32
}
