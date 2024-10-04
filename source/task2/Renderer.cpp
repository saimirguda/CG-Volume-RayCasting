#include "Renderer.h"
#include "Volume.h"
#include <iostream>
#include <string>

#include "imgui_transferfunction.h"

#include <imgui.h>
#define IMGUI_DEFINE_MATH_OPERATORS
#include <imgui_internal.h>

bool Renderer::gui_mouse_hoveres_render_image = false;

Renderer::Renderer(GL::platform::Window& window, Camera& _camera, int num_render_threads, const std::string& config_file)
    : BasicRenderer(window), volume_renderer(_camera, num_render_threads, config_file)
{
	glClearColor(0.1f, 0.1f, 0.1f, 1.f);

	// setup ImGui
	imgui_renderer.init();
	imgui_renderer.setDisplaySize(_camera.getViewport().x, _camera.getViewport().y);
	window.attach(this);
}

Renderer::~Renderer()
{
	volume_renderer.shutdown();
	imgui_renderer.terminate();
	ImGui::DestroyContext();
}

void Renderer::resize(int width, int height)
{
	{
		std::lock_guard<std::mutex> lock(volume_renderer.resource_lock);
		viewport_width = width;
		viewport_height = height;
		imgui_renderer.setDisplaySize(viewport_width, viewport_height);
	}
}

void Renderer::render()
{
	static bool changed = false;

	// send volume texture to GPU
	volume_texture.load();

	// gui calls
	ImGui::NewFrame();

	ImGui::SetNextWindowPos(ImVec2(5.f, 5.f), ImGuiCond_FirstUseEver);
	ImGui::Begin("Settings", NULL, ImVec2(425.f, 180.f));
	changed |= ImGui::ColorEdit3("Background", &volume_renderer.background_color.x);
	changed |= ImGui::DragFloat("Step size", &volume_renderer.step_size, 0.001f, 0.001f, 10.f);
	changed |= ImGui::DragFloat("Alpha threshold", &volume_renderer.alpha_threshold, 0.01f, 0.01f, 1.f);
	changed |= ImGui::DragFloat("Opacity correction", &volume_renderer.transfer_function.opacity_correction, 0.005f, 0.01f, 1.f);
	changed |= ImGui::Checkbox("Render", &do_rendering);
	ImGui::SameLine();
	changed |= ImGui::Checkbox("Shading", &volume_renderer.shading);
	bool save_config = false | ImGui::Button("Save Config");
	if (save_config)
		volume_renderer.saveConfig();
	ImGui::End();

	ImGui::SetNextWindowPos(ImVec2(465.f, 10.f), ImGuiCond_FirstUseEver);
	ImGui::Begin("Shading parameters", NULL, ImVec2(300.f, 150.f));
	changed |= ImGui::DragFloat("k_ambient", &volume_renderer.k_ambient, 0.01f, 0.0f, 1.f);
	changed |= ImGui::DragFloat("k_diffuse", &volume_renderer.k_diffuse, 0.01f, 0.0f, 1.f);
	changed |= ImGui::DragFloat("k_specular", &volume_renderer.k_specular, 0.01f, 0.0f, 1.f);
	changed |= ImGui::DragFloat("shininess", &volume_renderer.shininess, 0.05f, 0.0f, 10.f);
	changed |= ImGui::ColorEdit3("light color", &volume_renderer.light_color.x);
	ImGui::End();

	ImGui::SetNextWindowPos(ImVec2(5.f, 515.f), ImGuiCond_FirstUseEver);
	ImGui::Begin("Datasets", NULL, ImVec2(300.f, 80.f));
	if (volume_renderer.gui_dataset_not_found)
		ImGui::Text("File not found! Check your Working Directory!");
	static int volume_combo_item = volume_renderer.currentVolumeIndex();
	ImGui::Combo(" ", &volume_combo_item, "Cross\0Box\0Engine\0Male Head\0Stag Beetle\0\0");
	ImGui::End();

	ImGui::SetNextWindowPos(ImVec2(5.f, 195.f), ImGuiCond_FirstUseEver);
	ImGui::Begin("Transfer Function", NULL, ImVec2(425.f, 315.f));
	changed |= ImGuiTransferFunction("Transfer Function", volume_renderer.background_color, volume_renderer.transfer_function, volume_renderer.volume.getMaxIsoValue());
	ImGui::End();

	ImGui::SetNextWindowPos(ImVec2(475.f, 205.f), ImGuiCond_FirstUseEver);
	ImGui::Begin("Volume rendering", NULL, ImVec2(300.f, 300.f));
	ImVec2 window_size = ImGui::GetContentRegionAvail();
	const uint2& render_window_size = {static_cast<unsigned int>(window_size.x), static_cast<unsigned int>(window_size.y)};
	const uint2& texture_size = volume_texture.getSize();
	ImGui::Image(reinterpret_cast<ImTextureID>(static_cast<size_t>(volume_texture.volume)), ImVec2(static_cast<float>(texture_size.x), static_cast<float>(texture_size.y)));
	gui_mouse_hoveres_render_image = ImGui::IsItemHovered();
	ImGui::End();

	changed |= volume_renderer.camera.getNavigator().hasChanged();

	// render volume
	if (do_rendering)
	{
		bool resize_texture = render_window_size.x != texture_size.x || render_window_size.y != texture_size.y;
		bool switch_volume = volume_combo_item != volume_renderer.currentVolumeIndex();
		if (resize_texture || switch_volume)
		{
			if (volume_renderer.ready())
			{
				if (resize_texture)
				{
					volume_renderer.camera.setViewport((unsigned int)window_size.x, (unsigned int)window_size.y);
					volume_texture.resize((unsigned int)std::max(0.f, window_size.x), (unsigned int)std::max(0.f, window_size.y));
				}
				if (switch_volume)
					volume_renderer.switchVolume(volume_combo_item);
				changed = true;
			}
		}
		else if (changed)
		{
			if (volume_renderer.renderNonBlocking(volume_texture))
				changed = false;
		}
	}

	// clear window
	glDisable(GL_SCISSOR_TEST);
	glPolygonMode(GL_FRONT_AND_BACK, GL_FILL);
	glClear(GL_COLOR_BUFFER_BIT);

	// render gui and swap buffers
	ImGui::Render();

	swapBuffers();
}
