#pragma once
#include "ref.h"
#include <SDL.h>

enum ButtonCode {
	BTN_QUIT = 256,
	BTN_MOUSE0 = 257,
	BTN_MOUSE1 = 258,
	BTN_MOUSE2 = 259,
	BTN_MOUSE3 = 260,
	BTN_MOUSE4 = 261,
	BTN_MOUSE_LEFT = 258,
	BTN_MOUSE_RIGHT = 260
};

enum PixelFormat {};

enum RenderType {
	Framebuffer = 0,
	OpenGL = 1,
	Vulkan = 2,
};

struct RenderContext {
	int width, height;
	RenderType type;
	void* context;
};

struct WindowEventHandler {
	virtual void OnResize(int w, int h) {}
	virtual void OnMotion(int axis, float x, float y) {}
	virtual void OnButton(ButtonCode btn, int state) {}
	virtual void OnRender(RenderContext*) {}
};

struct Window : public RCObj {
	Window(RenderType type_) : type(type_) {}

	virtual void SetSize(int w, int h) = 0;
	virtual void SetFullscreen(bool fullscreen) = 0;
	virtual void SetResizable(bool resizable) = 0;
	virtual void SetGrab(bool grab) = 0;
	virtual void Wait() = 0;
	virtual void Open() = 0; // Locks everything
	virtual void Close() = 0; // Unlocks everything
	WindowEventHandler* Handler;

	RenderType type;
	int w, h;
	bool fullscreen;
	bool resizable;
	bool grab;
	void* userdata;
};
