#include <thread>
#include <stdio.h>
#include <glew/glew.h>
#include "udp.h"
#include "tcp.h"
#include "channel.h"
#include "window.h"
#include "window_sdl.h"
#include "http.h"

using namespace std;

void onUDP(void* userdata, void* data, int length, u64 timestamp, udp_address_t* from_hint) {
	printf("onUDP(%d bytes, timestamp=%lu, from_hint=%s)\n", length, timestamp, from_hint->ToString().c_str());
}

struct TestWindowEventHandler : public WindowEventHandler {
	bool knitted;
	void OnResize(int w, int h) {}
	void OnMotion(int axis, float x, float y) {}
	void OnButton(ButtonCode btn, int state) {}
	void OnRender(RenderContext*) {
		if (!knitted) {
			glewInit();
			knitted = true;
		}
		glClearColor(0,0,0,0);
		glClear(GL_COLOR_BUFFER_BIT);
	}
};

int main() {
	//printf("Hello, World!\n");
	
	/*
	UDP::Listen(onUDP);
	this_thread::sleep_for(1s);
	
	auto localhost = UDP::Lookup("127.0.0.1");
	UDP::Send("test", 4, localhost);
	this_thread::sleep_for(1s);
	
	UDP::UnListen(onUDP);
	*/
	
	/*
	Ref<Window> win = new WindowSDL(OpenGL, "test");
	win->SetSize(640, 480);
	TestWindowEventHandler hannler;
	win->Handler = &hannler;
	win->Open();
	win->Wait();
	*/
	
	/*
	auto ta = tcp_address_t::Lookup("127.0.0.1", 1234);
	auto tc = new TCPConnection(ta);
	tc->Write("test\n", 5);
	delete tc;
	*/
	
	/*
	struct TestLissner : public TCPListener {
		TestLissner(int port) : TCPListener(port) {}
		void OnConnect(Ref<TCPConnection> con, Ref<tcp_address_t> addr) {
			printf("Incoming TCP(%p) from %s\n", con, addr->ToString().c_str());
			con->Write("pony\n", 5);
		}
	};
	TestLissner tl(1235);
	tl.Wait();
	*/
	
	HTTPListener hl(8080);
	hl.Wait();

	return 0;
}
