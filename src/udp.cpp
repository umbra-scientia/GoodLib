#include "util/types.h"
#ifdef _WIN32
#include <WinSock2.h>
#include <Ws2tcpip.h>
#else
#include <arpa/inet.h>
#include <errno.h>
#include <netinet/ip.h>
#include <sys/socket.h>
#include <unistd.h>
#endif // #ifdef _WIN32
#include <cstring>
#include <thread>

using namespace std;

void handleError() {
#ifdef _WIN32
	wprintf(L"failed with error %d\n", WSAGetLastError());
	wchar_t* s = NULL;
	FormatMessageW(FORMAT_MESSAGE_ALLOCATE_BUFFER | FORMAT_MESSAGE_FROM_SYSTEM | FORMAT_MESSAGE_IGNORE_INSERTS,
		NULL, WSAGetLastError(), MAKELANGID(LANG_NEUTRAL, SUBLANG_DEFAULT), (LPWSTR)&s, 0, NULL);
	printf("%S\n", s);
	LocalFree(s);
#else
	printf("%s\n", strerror(errno));
#endif // #ifdef _WIN32
	exit(1);
}

int main(int argc, char* argv[]) {
#ifdef _WIN32
	WSAData data;
	WSAStartup(MAKEWORD(2, 2), &data);
#endif // #ifdef _WIN32

	thread recvThread([] {
		auto in = socket(PF_INET, SOCK_RAW, IPPROTO_UDP);
		if (in == -1) { handleError(); }

		sockaddr_in src;
		src.sin_family = AF_INET;
		src.sin_port = htons(0);
		inet_pton(AF_INET, "127.0.0.1", &src.sin_addr.s_addr);
		bind(in, (sockaddr*)&src, sizeof(src));

		char buf[1024];
		sockaddr_in from;
		socklen_t fromSize = sizeof(from);
		auto len = recvfrom(in, buf, sizeof(buf), 0, (sockaddr*)&from, &fromSize);
		if (len == -1) { handleError(); }
		printf("received %li bytes\n", len);

#ifdef _WIN32
		closesocket(in);
#else
		close(in);
#endif // #ifdef _WIN32
	});

	this_thread::sleep_for(1000ms);

	auto out = socket(AF_INET, SOCK_DGRAM, IPPROTO_UDP);

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
