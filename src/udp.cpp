#include "udp.h"
#include <chrono>
#include <cstdio>
#include <cstring>
#include <mutex>
#include <thread>
#include <sstream>
#include <unordered_map>
#include <unordered_set>

using namespace std;
using namespace std::chrono;
using namespace std::literals::chrono_literals;

//const steady_clock::time_point y2k { January / 1 / 2000 };
/*FIXME*/ const steady_clock::time_point y2k = std::chrono::steady_clock::now();

static bool USE_RAW_SOCKETS = true;

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
	addr_info = 0;
	addr = 0;
	len = 0;
	ownership = 0;
	proto = 0;
	port = -1;
}
udp_address_t::udp_address_t(const udp_address_t& a) {
	addr_info = 0;
	addr = 0;
	len = a.len;
	ownership = a.ownership;
	proto = a.proto;
	port = a.port;
	if (a.ownership) {
		addr = (sockaddr*)malloc(len);
		memcpy(addr, a.addr, len);
		ownership = 2;
	}
}

udp_address_t::~udp_address_t() {
	if (ownership == 1) {
		if (addr_info) freeaddrinfo(addr_info);
	}
	if (ownership == 2) {
		if (addr_info) free(addr_info);
		if (addr) free(addr);
	}
}

string udp_address_t::ToString() {
	string r = "udp://";
	if (proto == AF_INET) {
		char buf[32];
		memset(buf, 0, sizeof(buf));
		if (addr && inet_ntop(proto, &((sockaddr_in*)addr)->sin_addr, buf, sizeof(buf)-1)) {
			r += buf;
		} else {
			r += "????";
		}
	}
	if (proto == AF_INET6) {
		char buf[256];
		memset(buf, 0, sizeof(buf));
		if (addr && inet_ntop(proto, &((sockaddr_in6*)addr)->sin6_addr, buf, sizeof(buf)-1)) {
			r += buf;
		} else {
			r += "??????";
		}
	}
	if (port >= 0) {
		stringstream ss;
		ss << ":" << port;
		r += ss.str();
	}
	return r;
}

udp_address_t UDP::Lookup(const char* hostname, int default_port) {
	init();
	udp_address_t r;

	sockaddr_in src4;
	src4.sin_family = AF_INET;
	src4.sin_port = htons(default_port);
	if (inet_pton(AF_INET, hostname, &src4.sin_addr.s_addr) == 1) {
		r.len = sizeof(src4);
		r.addr = (sockaddr*)malloc(r.len);
		memcpy(r.addr, &src4, r.len);
		r.ownership = 2;
		r.proto = AF_INET;
		return r;
	}
	
	sockaddr_in6 src6;
	src6.sin6_family = AF_INET6;
	src6.sin6_port = htons(27255);
	if (inet_pton(AF_INET6, hostname, &src6.sin6_addr) == 1) {
		r.len = sizeof(src6);
		r.addr = (sockaddr*)malloc(r.len);
		memcpy(r.addr, &src6, r.len);
		r.ownership = 2;
		r.proto = AF_INET6;
		return r;
	}

	struct addrinfo* result = 0;
	auto err = getaddrinfo(hostname, NULL, NULL, &result);
	if (err == 0) {
		r.ownership = 1;
		r.addr_info = result;
		r.proto = result->ai_family;
		r.len = result->ai_addrlen;
		r.addr = result->ai_addr;
		return r;
	}
	
	return r;
}

static unordered_map<int,int> protosocket;
static mutex protosocketm;
int GetOutSocket(int proto) {
	lock_guard<mutex> lock(protosocketm);
	auto& s = protosocket[proto];
	if (s <= 0) s = socket(proto, SOCK_DGRAM, IPPROTO_UDP);
	return s;
}


u64 UDP::Send(const void* buffer, int length, udp_address_t addr) {
	printf("Send %d bytes to %s\n", length, addr.ToString().c_str());
	int s = GetOutSocket(addr.proto);
	//printf("GetOutSocket(%s) = %d\n", (addr.proto==AF_INET)?"AF_INET":((addr.proto==AF_INET6)?"AF_INET6":"???"), s);
	sendto(s, (char*)buffer, length, 0, addr.addr, addr.len);
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
		USE_RAW_SOCKETS = false;
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
			sockaddr_in from; // FIXME
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
			udp_address_t address;
			address.proto = AF_INET;
			address.addr = (sockaddr*)&from;
			address.len = fromSize;
			for (auto [callback, userdata] : cbs) {
				// address { (sockaddr*)&from, fromSize };
				callback(userdata, &buf[offset], len - offset, timestamp, &address);
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
