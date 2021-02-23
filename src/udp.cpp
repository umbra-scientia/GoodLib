#include "udp.h"
#include <chrono>
#include <cstdio>
#include <cstring>
#include <mutex>
#include <thread>
#include <unordered_map>
#include <unordered_set>

using namespace std;
using namespace std::chrono;
using namespace std::literals::chrono_literals;

//const steady_clock::time_point y2k { January / 1 / 2000 };

/*FIXME*/ const steady_clock::time_point y2k = std::chrono::steady_clock::now();
/*FIXME*/ static bool USE_RAW_SOCKETS = false;

namespace goodlib {
	namespace udp {
		struct CallbackData {
			udp_callback_t callback;
			void* userdata;

			bool operator==(const CallbackData& rhs) const {
				return callback == rhs.callback && userdata == rhs.userdata;
			}
		};
		struct CallbackDataHash {
			std::size_t operator()(const CallbackData& v) const {
				auto hash = std::hash<udp_callback_t>()(v.callback);
				hash ^= std::hash<void*>()(v.userdata) + 0x9e3779b9 + (hash << 6) + (hash >> 2);
				return hash;
			}
		};

		/// guards thread_handle, cbs, and end
		mutex udpmutex;
		thread* thread_handle = nullptr;
		unordered_set<CallbackData, CallbackDataHash> cbs;
#ifdef _WIN32
		WSAEVENT end_event = 0;
#else
		int end_event[2] { 0, 0 };
#endif

		void init() {
#ifdef _WIN32
			lock_guard<mutex> lock(udpmutex);
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

udp_address_t::udp_address_t() {
	len = 0;
	addr = 0;
	crap = 0;
}

udp_address_t::~udp_address_t() {
	//if (owner && crap) freeaddrinfo(crap);
}

udp_address_t UDP::Lookup(const char* hostname) {
	init();
	udp_address_t r;
	struct addrinfo* result = 0;

	// inet_pton(AF_INET, "0.0.0.0", &src.sin_addr.s_addr);

	auto err = getaddrinfo(hostname, NULL, NULL, &result);
	if (err != 0) {
		fprintf(stderr, "error: %s\n", gai_strerror(err));
		exit(1);
	}
	r.crap = result;
	r.proto = AF_INET;
	r.len = result->ai_addrlen;
	r.addr = result->ai_addr;
	return r;
}

u64 UDP::Send(const void* buffer, int length, udp_address_t addr) {
	static auto out = ([] {
		auto out = socket(AF_INET, SOCK_DGRAM, IPPROTO_UDP);
		if (out == -1) { handleError(); }
		return out;
	})();

	struct sockaddr_in adr;
	memset(&adr, 0, sizeof(adr));
	adr.sin_family = AF_INET;
	adr.sin_port = htons(27255);
	adr.sin_addr.s_addr = INADDR_ANY;

	sendto(out, (char*)buffer, length, 0, (const sockaddr*)&adr, sizeof(adr));
	return duration_cast<milliseconds>(steady_clock::now() - y2k).count();
}

void UDP::Listen(udp_callback_t callback, void* userdata) {
	init();
	lock_guard<mutex> lock(udpmutex);

	cbs.insert({ callback, userdata });
	if (thread_handle) { return; }

	int in = -1;
	if (USE_RAW_SOCKETS) {
		in = socket(PF_INET, SOCK_RAW, IPPROTO_UDP);
	}
	if (in == -1) {
		in = socket(AF_INET, SOCK_DGRAM, IPPROTO_UDP);
	}
	if (in == -1) {
		handleError();
	}

#ifdef _WIN32
	if (!end_event) { end_event = WSACreateEvent(); }
#else
	if (!end_event[0]) { pipe(end_event); }
#endif

	thread_handle = new thread([=] {
		sockaddr_in src;
		src.sin_family = AF_INET;
		src.sin_port = htons(USE_RAW_SOCKETS ? 0 : 27255);
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
			printf("recvfrom returned\n");
			if (len == -1) {
				handleError();
			} else if (len == 0) {
				break;
			}
#endif

			u64 timestamp = duration_cast<milliseconds>(steady_clock::now() - y2k).count();

			int offset = 0;
			if (USE_RAW_SOCKETS) {
				if (from.sin_family == AF_INET) {
					offset = 28;
				} else if (from.sin_family == AF_INET6) {
					offset = 56;
				} else {
					continue;
				}
			}
			lock_guard<mutex> lock(udpmutex);
			for (auto [callback, userdata] : cbs) {
				udp_address_t address;
				// address { (sockaddr*)&from, fromSize };
				callback(userdata, &buf[offset], len - offset, timestamp, address);
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

void UDP::UnListen(udp_callback_t callback, void* userdata) {
	lock_guard<mutex> lock(udpmutex);
	cbs.erase({ callback, userdata });
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
	if (thread_handle) {
		thread_handle->join();
		delete thread_handle;
	}
	thread_handle = nullptr;
}
