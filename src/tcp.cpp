#include "tcp.h"
#include <chrono>
#include <cstdio>
#include <cstring>
#include <mutex>
#include <thread>
#include <sstream>
#include <unordered_map>
#include <unordered_set>

using namespace std;

tcp_address_t::tcp_address_t() {
	addr_info = 0;
	addr = 0;
	len = 0;
	ownership = 0;
	proto = 0;
	port = -1;
}
/*
tcp_address_t::tcp_address_t(tcp_address_t* a) {
	addr_info = 0;
	addr = 0;
	len = a->len;
	ownership = 0;
	proto = a->proto;
	port = a->port;
	if (a->ownership) {
		addr = (sockaddr*)malloc(len);
		memcpy(addr, a->addr, len);
		ownership = 2;
	}
}
*/
tcp_address_t::~tcp_address_t() {
	if (ownership == 1) {
		if (addr_info) freeaddrinfo(addr_info);
	}
	if (ownership == 2) {
		if (addr_info) free(addr_info);
		if (addr) free(addr);
	}
}

string tcp_address_t::ToString() const {
	string r = "tcp://";
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

Ref<tcp_address_t> tcp_address_t::Lookup(const char* hostname, int port) {
	//init();
	auto rp = new tcp_address_t();
	tcp_address_t& r = *rp;
	r.port = port;

	sockaddr_in src4;
	src4.sin_family = AF_INET;
	src4.sin_port = htons(port);
	if (inet_pton(AF_INET, hostname, &src4.sin_addr.s_addr) == 1) {
		r.len = sizeof(src4);
		r.addr = (sockaddr*)malloc(r.len);
		memcpy(r.addr, &src4, r.len);
		r.ownership = 2;
		r.proto = AF_INET;
		return rp;
	}
	
	sockaddr_in6 src6;
	src6.sin6_family = AF_INET6;
	src6.sin6_port = htons(port);
	if (inet_pton(AF_INET6, hostname, &src6.sin6_addr) == 1) {
		r.len = sizeof(src6);
		r.addr = (sockaddr*)malloc(r.len);
		memcpy(r.addr, &src6, r.len);
		r.ownership = 2;
		r.proto = AF_INET6;
		return rp;
	}

	struct addrinfo* result = 0;
	auto err = getaddrinfo(hostname, NULL, NULL, &result);
	if (err == 0) {
		r.ownership = 1;
		r.addr_info = result;
		r.proto = result->ai_family;
		r.len = result->ai_addrlen;
		r.addr = result->ai_addr;
		return rp;
	}
	
	return rp;
}

TCPConnection::TCPConnection(Ref<tcp_address_t> ad) {
	addey = ad;
	fd = socket(AF_INET, SOCK_STREAM, 0);
	valid = (connect(fd, addey->addr, addey->len) < 0);
}

TCPConnection::TCPConnection(int fd_) {
	addey = 0;
	fd = fd_;
	valid = (fd >= 0);
}
TCPConnection::~TCPConnection() {
	if (fd >= 0) close(fd);
}
int TCPConnection::Write(const void* buffer, int length) {
	int e = send(fd, (char*)buffer, length, 0);
	if (e < 0) {
		valid = false;
		return 0;
	}
	return e;
}

int TCPConnection::Read(void* buffer, int length) {
	int e = recv(fd, (char*)buffer, length, 0);
	if (e < 0) {
		valid = false;
		return 0;
	}
	return e;
}
bool TCPConnection::Valid() {
	return valid;
}

TCPListener::TCPListener(int port_) {
	port = port_;
	valid = true;
	worker = new thread([=](){ workerfn(); });
}
TCPListener::~TCPListener() {
	valid = false;
	if (worker) {
		worker->join();
		delete worker;
		worker = 0;
	}
}
void TCPListener::Wait() {
	if (worker) {
		worker->join();
		delete worker;
		worker = 0;
	}
}
void TCPListener::workerfn() {
	sockaddr_in src;
	src.sin_family = AF_INET;
	src.sin_port = htons(port);
	inet_pton(src.sin_family, "0.0.0.0", &src.sin_addr.s_addr);
	int ls = socket(src.sin_family, SOCK_STREAM, 0);
	int reuse = 1;
	setsockopt(ls, SOL_SOCKET, SO_REUSEADDR | SO_REUSEPORT, &reuse, 4);
	if (bind(ls, (sockaddr*)&src, sizeof(src)) < 0) {
		valid = false;
		printf("bind failed\n");
	}
	if (listen(ls, 16) < 0) {
		valid = false;
		printf("listen failed\n");
	}
	while (valid) {
		Ref<tcp_address_t> ta = new tcp_address_t();
		ta->ownership = 2;
		ta->proto = AF_INET;
		ta->port = port;
		ta->len = sizeof(struct sockaddr_in);
		ta->addr = (sockaddr*)malloc(ta->len);
		int cs = accept(ls, ta->addr, &ta->len);
		if (cs < 0) continue;
		thread* t = new thread([=](){
			Ref<TCPConnection> tc = new TCPConnection(cs);
			OnConnect(tc, ta);
		});
		t->detach();
		delete t;
	}
}
