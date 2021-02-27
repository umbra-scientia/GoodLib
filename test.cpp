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
	
	struct omgponies : public HTTPListener {
		omgponies(int port) : HTTPListener(port) {}
		bool OnRequest(Ref<HTTPRequest> req) {
			printf("HTTP Request:\ncmd=%s\npath=%s\nproto=%s\nheaders.size()=%d\n", req->cmd.c_str(), req->path.c_str(), req->proto.c_str(), req->headers.size());
			for(auto i: req->headers) {
				if (!i.second.size()) {
					printf("headers[\"%s\"]=[]\n", i.first.c_str());
					continue;
				}
				printf("headers[\"%s\"]=[\n", i.first.c_str());
				for(auto j: i.second) {
					printf("\t%s\n", j.c_str());
				}
				printf("]\n", i.first.c_str());
			}
			printf("\n");
			
			req->con->Write("HTTP/1.1 200 OK\n\n", 17);
			req->con->Write("Hello, World!", 13);
			return true;
		}
	};
	omgponies ponieeessss(8080);
	ponieeessss.Wait();

	return 0;
}
