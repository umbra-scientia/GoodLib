#include "window_sdl.h"
#include <SDL.h>
#include <stdio.h>
#include <math.h>

WindowSDL::WindowSDL(RenderType type_, std::string title_) : Window(type_) {
	title = title_;
	worker_thread = 0;
	frame_delay = 1;
}
void WindowSDL::Wait() {
	if (worker_thread) {
		worker_thread->join();
	}
	worker_thread = 0;
}
WindowSDL::~WindowSDL() {
	done = 1;
	if (worker_thread) {
		worker_thread->join();
	}
}
void WindowSDL::SetSize(int w_, int h_) {
	w = w_;
	h = h_;
	if (Handler) Handler->OnResize(w, h);
}
void WindowSDL::SetFullscreen(bool fullscreen_) {
	fullscreen = fullscreen_;
	if (win) SDL_SetWindowFullscreen(win, fullscreen ? SDL_WINDOW_FULLSCREEN : 0);
}
void WindowSDL::SetResizable(bool resizable_) {
	resizable = resizable_;
	if (win) SDL_SetWindowResizable(win, resizable ? SDL_TRUE : SDL_FALSE);
}
void WindowSDL::SetGrab(bool grab_) {
	grab = grab_;
	if (!win) return;
	if (grab) {
		SDL_GetMouseState(&omx, &omy);
		SDL_ShowCursor(SDL_DISABLE);
		SDL_WarpMouseInWindow(win, w/2, h/2);
	} else {
		SDL_WarpMouseInWindow(win, omx, omy);
		SDL_ShowCursor(SDL_ENABLE);
	}
}
void WindowSDL::SetTitle(std::string title_) {
	title = title_;
	if (win) SDL_SetWindowTitle(win, title.c_str());
}
void WindowSDL::Open() {
	done = 0;
	if (!worker_thread) {
		worker_thread = new std::thread([=](){ worker(); });
	}
}
void WindowSDL::Close() {
	done = 1;
	if (worker_thread) {
		worker_thread->join();
	}
	worker_thread = 0;
}
void WindowSDL::worker() {
	SDL_Init(SDL_INIT_EVERYTHING);
	int flags = 0;
	if (type == OpenGL) flags |= SDL_WINDOW_OPENGL;
	if (type == Vulkan) {
#ifdef USE_VULKAN
		flags |= SDL_WINDOW_VULKAN;

		VkResult err;
		VkInstanceCreateInfo ci;
		memset(&ci, 0, sizeof(ci));
		ci.sType = VK_STRUCTURE_TYPE_INSTANCE_CREATE_INFO;
		ci.enabledExtensionCount = extensions_count;
		ci.ppEnabledExtensionNames = extensions;
		auto err = vkCreateInstance(&ci, 0, &vk_instance);
		// FIXME
#else
		fprintf(stderr, "Warning: Not compiled with Vulkan support\n");
#endif
	}
	if (resizable) flags |= SDL_WINDOW_RESIZABLE;
	if (fullscreen) flags |= SDL_WINDOW_FULLSCREEN;
	win = SDL_CreateWindow(title.c_str(), 0, 0, w, h, flags);
	SDL_GetMouseState(&omx, &omy);
	SDL_GetWindowSize(win, &w, &h);
	if (type == OpenGL) {
		gl = SDL_GL_CreateContext(win);
	}
	RenderContext rctx;
	memset(&rctx, 0, sizeof(rctx));
	rctx.width = w;
	rctx.height = h;
	if (type == Framebuffer) {
		//if (!fb_render) fb_render = 
		//if (!fb_texture) fb_texture = SDL_CreateTexture(fb_render, SDL_PIXELFORMAT_ARGB8888, SDL_TEXTUREACCESS_STREAMING, w, h);
		// FIXME
	}
	if (type == OpenGL) rctx.context = &gl;
#ifdef USE_VULKAN
	if (type == Vulkan) {
		rctx.context = &vk_surface;
		auto err = SDL_Vulkan_CreateSurface(win, vk_instance, &vk_surface);
		// FIXME
	}
#endif
	while (!done) {
		SDL_Event e;
		while (SDL_PollEvent(&e)) {
			switch (e.type) {
				case SDL_WINDOWEVENT:
					if (e.window.event == SDL_WINDOWEVENT_SIZE_CHANGED) {
						w = e.window.data1;
						h = e.window.data2;
						resized = 1;
					}
					break;
				case SDL_QUIT:
					if (!grab) done = 1;
					if (Handler) Handler->OnButton(BTN_QUIT, 1);
					break;
				case SDL_MOUSEBUTTONDOWN:
					if (Handler) Handler->OnButton((ButtonCode)(BTN_MOUSE0 + e.button.button), 1);
					break;
				case SDL_MOUSEBUTTONUP:
					if (Handler) Handler->OnButton((ButtonCode)(BTN_MOUSE0 + e.button.button), 0);
					break;
				case SDL_KEYDOWN:
					if (Handler) Handler->OnButton((ButtonCode)e.key.keysym.sym, 1);
					break;
				case SDL_KEYUP:
					if (Handler) Handler->OnButton((ButtonCode)e.key.keysym.sym, 0);
					break;
			}
		}
		if (grab) {
			int mx=0, my=0;
			SDL_GetMouseState(&mx, &my);
			mx -= w/2;
			my -= h/2;
			if (mx*mx+my*my >= 8) {
				SDL_WarpMouseInWindow(win, w/2, h/2);
				if (Handler) Handler->OnMotion(0, mx, my);
			}
		}
		if (Handler) Handler->OnRender(&rctx);
		SDL_GL_SwapWindow(win);
		if (frame_delay) SDL_Delay(frame_delay);
	}
	SDL_GL_DeleteContext(gl);
	SDL_DestroyWindow(win);
}
