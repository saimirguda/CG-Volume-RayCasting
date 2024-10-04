#ifndef INCLUDED_RENDERER
#define INCLUDED_RENDERER

#pragma once

#include "Camera.h"
#include "ColorBuffer.h"
#include "Transferfunction.h"
#include "VolumeRenderer.h"
#include "framework/BasicRenderer.h"
#include "framework/imgui_renderer.h"
#include "math/vector.h"
#include <GL/gl.h>
#include <map>

class Renderer : public BasicRenderer
{
	ImGuiRenderer imgui_renderer;

	int viewport_width;
	int viewport_height;

	VolumeRenderer volume_renderer;
	ColorBuffer volume_texture;

	bool do_rendering = true;

public:
	Renderer(const Renderer&) = delete;
	Renderer& operator=(const Renderer&) = delete;

	Renderer(GL::platform::Window& window, Camera& camera, int num_render_threads, const std::string& config_file);
	~Renderer();

	void resize(int width, int height);
	void render();

	static bool gui_mouse_hoveres_render_image;
};

#endif // INCLUDED_RENDERER
