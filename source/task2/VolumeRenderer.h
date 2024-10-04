#ifndef INCLUDED_VOLUME_RENDERER
#define INCLUDED_VOLUME_RENDERER

#pragma once

#include "Camera.h"
#include "ColorBuffer.h"
#include "Transferfunction.h"
#include "Volume.h"
#include "math/vector.h"
#include <atomic>
#include <condition_variable>
#include <mutex>
#include <string>
#include <thread>

class FrameBarrier
{
	std::mutex mutex;

	int pending;

	std::condition_variable start_frame_event;
	std::condition_variable frame_complete_event;

	bool stop = false;

	const int num_threads;

	bool completeFrame()
	{
		std::unique_lock<std::mutex> lock(mutex);
		return --pending == 0;
	}

public:
	FrameBarrier(int num_threads)
	    : pending(num_threads), num_threads(num_threads)
	{
	}

	bool start_frame()
	{
		if (completeFrame())
			frame_complete_event.notify_all();
		else
		{
			std::unique_lock<std::mutex> lock(mutex);
			while (pending > 0)
				frame_complete_event.wait(lock);
		}

		std::unique_lock<std::mutex> lock(mutex);
		while (pending == 0 && !stop)
			start_frame_event.wait(lock);

		if (stop)
			return false;

		return true;
	}

	void wait()
	{
		std::unique_lock<std::mutex> lock(mutex);
		while (pending > 0)
			frame_complete_event.wait(lock);
	}

	void next_frame()
	{
		{
			std::lock_guard<std::mutex> lock(mutex);
			pending = num_threads;
		}
		start_frame_event.notify_all();
		wait();
	}

	void quit()
	{
		{
			std::lock_guard<std::mutex> lock(mutex);
			stop = true;
		}
		start_frame_event.notify_all();
	}
};

class VolumeRenderer
{
	int volume_index = 0;
	bool loadVolume(const std::string& path_to_file, uint32_t component_size, const uint3& size, const float3& scaling, bool invert_y, Volume::Axes axes);

	int num_render_threads;
	std::vector<std::thread> render_threads;
	std::thread control_thread;
	std::condition_variable cv_render;
	std::condition_variable cv_finished;
	std::mutex mutex_render;
	bool quit = false;
	bool renderer_ready = false;
	FrameBarrier frame_barrier;

	void render_loop();
	void worker_loop(int thread_id);

	ColorBuffer* target_texture = nullptr;

public:
	VolumeRenderer(const VolumeRenderer&) = delete;
	VolumeRenderer& operator=(const VolumeRenderer&) = delete;

	VolumeRenderer(Camera& camera, int num_render_threads, const std::string& config_file);

	Camera& camera;
	TransferFunction transfer_function;
	Volume volume;


	ColorBuffer cast_rays;
	std::vector<float3> ray_directions;

	float4 coloredRay(const float3& ray_direction)
	{
		return float4(static_cast<unsigned int>(ray_direction.x * 255.f),
			                                 static_cast<unsigned int>(ray_direction.y * 255.f),
			                                 static_cast<unsigned int>(ray_direction.z * 255.f),
			                                 255);
	}


	std::mutex resource_lock;

	bool gui_dataset_not_found = false;

	float3 background_color = {1.f, 1.f, 1.f};
	float step_size = 0.1f;
	float alpha_threshold = 0.95f;

	float3 light_color = {1.f, 1.f, 1.f};
	float k_ambient = 0.1f;
	float k_diffuse = 0.9f;
	float k_specular = 0.1f;
	float shininess = 1.15f;
	bool shading = false;

	bool renderNonBlocking(ColorBuffer& texture);
	bool renderBlocking(ColorBuffer& texture);
	bool ready() const;
	void shutdown();

	bool switchVolume(int index);
	void loadConfig(const std::string& config_file);
	void saveConfig();
	int currentVolumeIndex() const;

	void writeUpSampledSlices(const std::string& file_name, uint32_t upscale) const;
	void writeAABB(const std::string& file_name) const;
};

#endif // INCLUDED_VOLUME_RENDERER
