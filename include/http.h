#include "tcp.h"
#include <mutex>
#include <vector>
#include <string>
#include <map>

struct HTTPRequest : public RCObj {
	std::string cmd, path, proto;
	std::map<std::string, std::vector<std::string>> headers;
	Ref<TCPConnection> con;
};

struct HTTPListener : public TCPListener {
	HTTPListener(int port) : TCPListener(port) {}
	void OnConnect(Ref<TCPConnection> con, Ref<tcp_address_t> from_hint);
	virtual bool OnRequest(Ref<HTTPRequest> req);
};
