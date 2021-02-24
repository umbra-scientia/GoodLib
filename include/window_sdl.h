#pragma once
#include "window.h"
#include <SDL.h>
#include <string>
#include <thread>

struct WindowSDL : public Window {
	WindowSDL(RenderType type, std::string title);
	~WindowSDL();

	void SetSize(int w, int h);
	void SetFullscreen(bool full);
	void SetResizable(bool resizable);
	void SetGrab(bool grab);
	void SetTitle(std::string title);
	void Wait();
	void Open();
	void Close();
	
	SDL_Window* win;
	SDL_GLContext gl;
#ifdef USE_VULKAN
	VkSurfaceKHR vk_surface;
	VkInstance vk_instance;
#endif
	SDL_Renderer* fb_render;
	SDL_Texture* fb_texture;
	void* fb_buffer;
	int done;
	int resized;
	int omx, omy;
	int frame_delay;
	std::string title;
	std::thread* worker_thread;
	void worker();
};
