#include "tcp.h"
#include <mutex>
#include <vector>

struct HTTPClient;
struct HTTPListener : public TCPListener {
	HTTPListener(int port) : TCPListener(port) {}
	void OnConnect(Ref<TCPConnection> con, Ref<tcp_address_t> from_hint);
	std::mutex m;
	std::vector<HTTPClient*> clients;
};
