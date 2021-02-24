#include <thread>
#include <stdio.h>
#include <glew/glew.h>
#include "udp.h"
#include "channel.h"
#include "window.h"
#include "window_sdl.h"

using namespace std;

void onUDP(void* userdata, void* data, int length, u64 timestamp, udp_address_t* from_hint) {
	printf("onUDP(%d bytes, timestamp=%lu, from_hint=%s)\n", length, timestamp, from_hint->ToString().c_str());
}

struct TestWindowEventHandler {
	virtual void OnResize(int w, int h) {}
	virtual void OnMotion(int axis, float x, float y) {}
	virtual void OnButton(ButtonCode btn, int state) {}
	virtual void OnRender(RenderContext*) {}
};

void inputMotion(float x, float y) {}
void inputButton(int btn, int state) {}
void render() {
    glClearColor(0,0,0,0);
    glClear(GL_COLOR_BUFFER_BIT);
}

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
	
	Ref<Window> win = new WindowSDL(OpenGL, "test");
	win->SetSize(640, 480);
	win->Open();
	win->Wait();
	return 0;
}
