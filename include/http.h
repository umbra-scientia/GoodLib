#include "tcp.h"
#include <mutex>
#include <vector>
#include <string>
#include <map>
#include <string.h>

struct HTTPRequest : public RCObj {
	std::string cmd, path, proto;
	std::map<std::string, std::vector<std::string>> headers;
};

struct HTTPResponse : public RCObj {
	int code;
	std::string status, proto;
	std::map<std::string, std::vector<std::string>> headers;
	std::vector<char> body;
	
	void Append(const char* str) { body.insert(body.end(), str, str+strlen(str)); }
	template <class T> void Append(T data) { body.insert(body.end(), data.data(), data.data()+data.size()); }
};

struct HTTPListener : public TCPListener {
	HTTPListener(int port) : TCPListener(port) {}
	void OnConnect(Ref<TCPConnection> con, Ref<tcp_address_t> from_hint);
	virtual Ref<HTTPResponse> OnRequest(Ref<HTTPRequest> req, Ref<TCPConnection> con);
};
