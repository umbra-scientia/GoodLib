#include <thread>
#include <stdio.h>
#include "udp.h"
#include "channel.h"

using namespace std;

void onUDP(void* userdata, void* data, int length, udp_address_t from_hint) {
	printf("onUDP(%d bytes) addrlen=%d\n", length, from_hint.len);
}

int main() {
	printf("Hello, World!\n");
	UDP::Listen(onUDP);
	this_thread::sleep_for(1s);
	
	auto localhost = UDP::Lookup("localhost");
	UDP::Send("test", 4, localhost);
	this_thread::sleep_for(1s);
	
	UDP::UnListen(onUDP);
	return 0;
}
