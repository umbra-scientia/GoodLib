#include "../include/UDP.h"
#include <cstdio>
#include <cstring>
#include <mutex>
#include <thread>
#include <unordered_map>
#include <unordered_set>

using namespace std;

#ifndef SOCKET
#define SOCKET int
#endif

namespace goodlib {
	namespace udp {
		/// guards thread_handle, cbs, and end
		mutex udpmutex;
		thread* thread_handle;
		unordered_set<udp_callback_t> cbs;
#ifdef _WIN32
		WSAEVENT end_event = 0;
#else
		int end_event[2] { 0, 0 };
#endif

		void init() {
#ifdef _WIN32
			static WSAData data {};
			if (!data.wVersion) { WSAStartup(MAKEWORD(2, 2), &data); }
#endif
		}

		void handleError() {
#ifdef _WIN32
			wprintf(L"failed with error %d\n", WSAGetLastError());
			wchar_t* s = NULL;
			FormatMessageW(FORMAT_MESSAGE_ALLOCATE_BUFFER | FORMAT_MESSAGE_FROM_SYSTEM
					| FORMAT_MESSAGE_IGNORE_INSERTS,
				NULL, WSAGetLastError(), MAKELANGID(LANG_NEUTRAL, SUBLANG_DEFAULT), (LPWSTR)&s, 0,
				NULL);
			printf("%S\n", s);
			LocalFree(s);
#else
			printf("%s\n", strerror(errno));
#endif // #ifdef _WIN32
			exit(1);
		}
	}
}

using namespace goodlib::udp;

udp_address_t UDP::Lookup(const char* hostname) {
	init();

	struct addrinfo* result;
	auto err = getaddrinfo(hostname, NULL, NULL, &result);
	if (err != 0) {
		fprintf(stderr, "error: %s\n", gai_strerror(err));
		exit(1);
	}

	auto ai_addrlen = result->ai_addrlen;
	auto ai_addr = (sockaddr*)malloc(ai_addrlen);
	memcpy(ai_addr, result->ai_addr, ai_addrlen);
	freeaddrinfo(result);

	return udp_address_t { ai_addr, (socklen_t)ai_addrlen };
}

void UDP::Send(const void* buffer, int length, udp_address_t addr) {
	static auto out = ([] {
		auto out = socket(AF_INET, SOCK_DGRAM, IPPROTO_UDP);
		if (out == -1) { handleError(); }
		return out;
	})();

	sendto(out, (char*)buffer, length, 0, addr.addr, addr.len);
}

void UDP::Listen(udp_callback_t callback) {
	lock_guard<mutex> lock(udpmutex);
	init();

	cbs.insert(callback);
	if (thread_handle) { return; }

	auto in = socket(PF_INET, SOCK_RAW, IPPROTO_UDP);
	if (in == -1) { handleError(); }

#ifdef _WIN32
	if (!end_event) { end_event = WSACreateEvent(); }
#else
	if (!end_event[0]) { pipe(end_event); }
#endif

	thread_handle = new thread([=] {
		sockaddr_in src;
		src.sin_family = AF_INET;
		src.sin_port = htons(0);
		inet_pton(AF_INET, "0.0.0.0", &src.sin_addr.s_addr);
		bind(in, (sockaddr*)&src, sizeof(src));

#ifdef _WIN32
		auto recv = WSACreateEvent();
#endif

		while (true) {
			sockaddr_in from;
			socklen_t fromSize = sizeof(from);
			char buf[1024];

#ifdef _WIN32
			WSABUF data { 1024, buf };
			DWORD len = 0;
			DWORD flags = 0;
			WSAOVERLAPPED overlapped {};
			overlapped.hEvent = recv;
			if (WSARecvFrom(in, &data, 1, &len, &flags, (sockaddr*)&from, &fromSize, &overlapped, NULL)) {
				if (WSAGetLastError() == WSA_IO_PENDING) {
					WSAEVENT events[] { recv, end_event };
					auto index = WSAWaitForMultipleEvents(2, events, FALSE, INFINITE, FALSE);
					if (index == WSA_WAIT_FAILED) {
						handleError();
					} else if (index - WSA_WAIT_EVENT_0 == 1) {
						break;
					}

					WSAResetEvent(recv);
					if (!WSAGetOverlappedResult(in, &overlapped, &len, TRUE, &flags)) {
						handleError();
					}
				} else {
					handleError();
				}
			}
#else
			fd_set rfds;
			FD_ZERO(&rfds);
			FD_SET(in, &rfds);
			FD_SET(end_event[0], &rfds);
			select(FD_SETSIZE, &rfds, nullptr, nullptr, nullptr);
			if (FD_ISSET(end_event[0], &rfds)) { break; }
			auto len = recvfrom(in, buf, sizeof(buf), 0, (sockaddr*)&from, &fromSize);
			if (len == -1) {
				handleError();
			} else if (len == 0) {
				break;
			}
#endif

			int offset;
			if (from.sin_family == AF_INET) {
				offset = 28;
			} else if (from.sin_family == AF_INET6) {
				offset = 56;
			} else {
				printf("family %i not supported", from.sin_family);
				exit(1);
			}

			lock_guard<mutex> lock(udpmutex);
			for (auto&& cb : cbs) {
				cb(&buf[offset], len - offset, udp_address_t { (sockaddr*)&from, fromSize });
			}
		}

#ifdef _WIN32
		WSACloseEvent(recv);
		WSACloseEvent(end_event);
		closesocket(in);
#else
		close(end_event[0]);
		close(in);
#endif
	});
}

void UDP::UnListen(udp_callback_t callback) {
	lock_guard<mutex> lock(udpmutex);
	cbs.erase(callback);
	if (cbs.size()) { return; }

#ifdef _WIN32
	WSASetEvent(end_event);
	end_event = 0;
#else
	write(end_event[1], "x", 1);
	close(end_event[1]);
	end_event[0] = 0;
	end_event[1] = 0;
#endif

	thread_handle->join();
	delete thread_handle;
	thread_handle = nullptr;
}
