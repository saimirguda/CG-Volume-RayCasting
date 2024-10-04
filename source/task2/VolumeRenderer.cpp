#include "VolumeRenderer.h"
#include "task2.h"

#include <algorithm>
#include <fstream>
#include <functional>
#include <iostream>

#include "framework/png.h"
#include <rapidjson/document.h>
#include <rapidjson/istreamwrapper.h>
#include <rapidjson/ostreamwrapper.h>
#include <rapidjson/writer.h>

#define CFG_STEP_SIZE "step_size"
#define CFG_OPACITY_CORRECTION "opacity_correction"
#define CFG_BACKGROUND_COLOR "background_color"
#define CFG_SHADING "shading"
#define CFG_SHADING_LIGHT "light_color"
#define CFG_SHADING_AMBIENT "k_ambient"
#define CFG_SHADING_DIFFUSE "k_diffuse"
#define CFG_SHADING_SPECULAR "k_specular"
#define CFG_SHADING_SHININESS "shininess"

#define CFG_CAMERA_PHI "camera_phi"
#define CFG_CAMERA_THETA "camera_theta"
#define CFG_CAMERA_RADIUS "camera_radius"

#define CFG_VOLUME "volume"

#define CFG_TRANSFERFUNCTION_RANGE_X "transfer_function_range_x"
#define CFG_TRANSFERFUNCTION_RANGE_Y "transfer_function_range_y"
#define CFG_TRANSFERFUNCTION_COLOR "color"
#define CFG_TRANSFERFUNCTION_CP "cp"
#define CFG_TRANSFERFUNCTION_CPS "transfer_function_cps"

#define CFG_TRANSFERFUNCTION_LUT "lookup-table"

VolumeRenderer::VolumeRenderer(Camera& camera_, int _num_render_threads, const std::string& config_file) : camera(camera_),
                                                                                                           num_render_threads(_num_render_threads),
                                                                                                           frame_barrier(_num_render_threads)
{
	control_thread = std::thread(&VolumeRenderer::render_loop, this);

	for (int i = 0; i < num_render_threads; ++i)
		render_threads.push_back(std::thread(&VolumeRenderer::worker_loop, this, i));
	frame_barrier.wait();

	if (!config_file.empty())
		loadConfig(config_file);

	if (!volume.isLoaded())
		switchVolume(0);
}

void VolumeRenderer::shutdown()
{
	{
		std::lock_guard<std::mutex> lock(mutex_render);
		quit = true;
	}
	cv_render.notify_all();
	cv_finished.notify_all();
	control_thread.join();

	frame_barrier.quit();
	for (auto& render_thread : render_threads)
		render_thread.join();
}

bool VolumeRenderer::ready() const
{
	return renderer_ready;
}

bool VolumeRenderer::renderNonBlocking(ColorBuffer& texture)
{
	if (ready())
	{
		std::unique_lock<std::mutex> lock(mutex_render);
		target_texture = &texture;
		uint2 size = target_texture->getSize();
		ray_directions.resize(size.x * size.y);
		cast_rays.resize(size.x, size.y);
		renderer_ready = false;
		cv_render.notify_all();
		return true;
	}
	return false;
}

bool VolumeRenderer::renderBlocking(ColorBuffer& texture)
{
	{
		std::unique_lock<std::mutex> lock(mutex_render);
		if (!ready())
			cv_finished.wait(lock);
	}
	renderNonBlocking(texture);
	{
		std::unique_lock<std::mutex> lock(mutex_render);
		if (!ready())
			cv_finished.wait(lock);
	}
	return true;
}

void VolumeRenderer::render_loop()
{
	while (!quit)
	{
		{
			std::unique_lock<std::mutex> lock(mutex_render);
			renderer_ready = true;
			cv_finished.notify_all();
			cv_render.wait(lock);
		}
		if (quit)
			return;

		transfer_function.updateLookupTable();

		if (!volume.isLoaded())
		{
			std::cout << "VolumeRenderer::render_loop(): no volume loaded!" << std::endl;
			continue;
		}
		if (!(camera.getViewport() == target_texture->getSize()))
		{
			std::cout << "VolumeRenderer::render_loop(): viewport size does not match target texture!" << std::endl;
			continue;
		}
		frame_barrier.next_frame();
	}
}

void VolumeRenderer::worker_loop(int thread_id)
{
	while (frame_barrier.start_frame())
	{
		auto size = camera.getViewport();

		
		auto dh = (size.y + num_render_threads - 1) / num_render_threads;
		int left = 0;
		int top = std::min(thread_id * dh, size.y);
		int right = size.x;
		int bottom = std::min(thread_id * dh + dh, size.y);
		volumeRaycasting(ray_directions, left, top, right, bottom, camera);


		for (int y = top; y < bottom; ++y)
		{
			for (int x = left; x < right; ++x)
			{
				cast_rays[uint2(x, y)] = coloredRay(ray_directions[y * size.x + x]);
				(*target_texture)[uint2(x, y)] = colorPixel(camera.getPosition(), ray_directions[y * size.x + x],
				                                            volume, step_size, transfer_function.opacity_correction,
				                                            alpha_threshold, background_color,
				                                            light_color, k_ambient,
				                                            k_diffuse, k_specular,
				                                            shininess, shading, transfer_function.getLookupTable());

			}
		}

	}
}

	bool VolumeRenderer::loadVolume(const std::string& path_to_file, uint32_t component_size, const uint3& size, const float3& scaling, bool invert_y, Volume::Axes axes)
	{
		transfer_function.setRange(uint2(1u << (component_size * 8u), 255u));
		return gui_dataset_not_found = !volume.load(path_to_file, component_size, size, scaling, invert_y, axes);
	}

	bool VolumeRenderer::switchVolume(int index)
	{
		std::lock_guard<std::mutex> lock(resource_lock);
		volume_index = index;

		switch (volume_index)
		{
			case 0:
				return loadVolume("datasets/Cross.raw", 1, uint3(64, 64, 64), float3(1.f, 1.f, 1.f), false, Volume::Axes::NO_SWITCH);
			case 1:
				return loadVolume("datasets/Box.raw", 1, uint3(64, 64, 64), float3(1.f, 1.f, 1.f), false, Volume::Axes::NO_SWITCH);
			case 2:
				return loadVolume("datasets/Engine.raw", 1, uint3(256, 256, 256), float3(1.f, 1.f, 1.f), true, Volume::Axes::NO_SWITCH);
			case 3:
				return loadVolume("datasets/VisMale.raw", 1, uint3(128, 256, 256), float3(1.577740f, 0.995861f, 1.007970f), true, Volume::Axes::SWITCH_YZ);
			case 4:
				return loadVolume("datasets/Beetle.raw", 1, uint3(277, 277, 164), float3(1.f, 1.f, 1.f), false, Volume::Axes::NO_SWITCH);
			case 5:
				return loadVolume("datasets/Sphere.raw", 1, uint3(64, 64, 64), float3(1.f, 1.f, 1.f), false, Volume::Axes::NO_SWITCH);
			case 6:
				return loadVolume("datasets/Spheres.raw", 1, uint3(128, 128, 128), float3(1.f, 1.f, 1.f), false, Volume::Axes::NO_SWITCH);
			case 7:
				return loadVolume("datasets/BluntFin.raw", 1, uint3(256, 128, 64), float3(1.f, 0.75f, 1.f), false, Volume::Axes::NO_SWITCH);
			case 8:
				return loadVolume("datasets/Fuel.raw", 1, uint3(64, 64, 64), float3(1.f, 1.f, 1.f), false, Volume::Axes::NO_SWITCH);
			case 9:
				return loadVolume("datasets/C60.raw", 1, uint3(64, 64, 64), float3(1.f, 1.f, 1.f), false, Volume::Axes::NO_SWITCH);
			default:
				return false;
		}
	}

	int VolumeRenderer::currentVolumeIndex() const
	{
		return volume_index;
	}

	void VolumeRenderer::writeUpSampledSlices(const std::string& file_name, uint32_t upscale) const
	{
		std::string output_file = file_name + "-slice-*.png";
		std::cout << "Creating slice images '" << output_file << "'" << std::endl;

		using PosOffsetFunc = std::function<float3(const float3&, const uint2& px)>;

		auto slicer = [this, file_name](size_t width, size_t height, const float3& p_start, const float3& step, PosOffsetFunc getPosOffset, char axis) -> void
		{
			image<uint32_t> buffer(width, height);
			uint2 px;
			for (px.x = 0; px.x < width; px.x++)
				for (px.y = 0; px.y < height; px.y++)
				{
					float3 offset = getPosOffset(step, px);
					float3 sample_pos = p_start + offset;
					uchar4 scaled_density(static_cast<unsigned char>(interpolatedDensity(volume, sample_pos) / volume.getMaxIsoValue() * 255.f));
					scaled_density.w = 255;
					buffer(px.x, px.y) = *((uint32_t*)&scaled_density);
				}
			PNG::saveImage((file_name + "-slice-" + axis + ".png").c_str(), buffer);
		};

		const uint3 volume_size = volume.getVolumeSize();
		const float3 aabb_size = volume.getAABBMax() - volume.getAABBMin();

		{
			const size_t width = volume_size.z * upscale;
			const size_t height = volume_size.y * upscale;
			const float3 p_start = volume.getAABBMin() + float3(aabb_size.x, 0.f, 0.f) * 0.5f;
			;
			const float3 step = aabb_size * float3(0.f, 1.f / height, 1.f / width);
			auto getPosOffset = [](const float3& step, const uint2& px) -> float3
			{
				return float3(0.f, px.y * step.y, px.x * step.z);
			};
			slicer(width, height, p_start, step, getPosOffset, 'x');
		}
		{
			const size_t width = volume_size.x * upscale;
			const size_t height = volume_size.z * upscale;
			const float3 p_start = volume.getAABBMin() + float3(0.f, aabb_size.y, 0.f) * 0.5f;
			;
			const float3 step = aabb_size * float3(1.f / width, 0.f, 1.f / height);
			auto getPosOffset = [](const float3& step, const uint2& px) -> float3
			{
				return float3(px.x * step.x, 0.f, px.y * step.z);
			};
			slicer(width, height, p_start, step, getPosOffset, 'y');
		}
		{
			const size_t width = volume_size.x * upscale;
			const size_t height = volume_size.y * upscale;
			const float3 p_start = volume.getAABBMin() + float3(0.f, 0.f, aabb_size.z) * 0.5f;
			const float3 step = aabb_size * float3(1.f / width, 1.f / height, 0.f);
			auto getPosOffset = [](const float3& step, const uint2& px) -> float3
			{
				return float3(px.x * step.x, px.y * step.y, 0.f);
			};
			slicer(width, height, p_start, step, getPosOffset, 'z');
		}
	}

	void VolumeRenderer::writeAABB(const std::string& file_name) const
	{
		std::string output_file_near = file_name + "-aabb-near.png";
		std::string output_file_far = file_name + "-aabb-far.png";
		std::cout << "Creating AABB distance images '" << output_file_near << "' and '" << output_file_far << "'" << std::endl;

		const float3& aabb_min = volume.getAABBMin();
		const float3& aabb_max = volume.getAABBMax();

		const float3 aabb_size = aabb_max - aabb_min;
		const float3 ray_dir = {0.f, 0.f, 1.f};

		const unsigned int width = 400;
		const unsigned int height = 400;
		const float2 img_area = float2(aabb_size.x, aabb_size.y) * 1.2f;
		const float3 ray_start_offset = float3(-img_area.x * 0.5f, -img_area.y * 0.5f, -aabb_size.z);
		const float3 step = float3(img_area.x / width, img_area.y / height, 0.f);

		const float distance_scale = 1.75f * aabb_size.z;

		image<uint32_t> buffer_near(width, height);
		image<uint32_t> buffer_far(width, height);
		uint2 px;
		for (px.x = 0; px.x < width; px.x++)
			for (px.y = 0; px.y < height; px.y++)
			{
				float3 ray_start = ray_start_offset + float3(px.x * step.x, px.y * step.y, 0.f);
				float t_near, t_far;
				if (intersectRayWithAABB(ray_start, ray_dir, aabb_min, aabb_max, t_near, t_far))
				{
					uchar4 val_near(static_cast<unsigned char>((1.f - std::min(t_near / distance_scale, 1.f)) * 255.f));
					val_near.w = 255;
					buffer_near(px.x, px.y) = *((uint32_t*)&val_near);

					uchar4 val_far(static_cast<unsigned char>((1.f - std::min(t_far / distance_scale, 1.f)) * 255.f));
					val_far.w = 255;
					buffer_far(px.x, px.y) = *((uint32_t*)&val_far);
				}
				else
				{
					uchar4 background(255);
					buffer_near(px.x, px.y) = *((uint32_t*)&background);
					buffer_far(px.x, px.y) = *((uint32_t*)&background);
				}
			}
		PNG::saveImage(output_file_near.c_str(), buffer_near);
		PNG::saveImage(output_file_far.c_str(), buffer_far);
	}

	void VolumeRenderer::loadConfig(const std::string& config_file)
	{
		std::ifstream file(config_file, std::ios::binary);
		if (file.is_open())
		{
			rapidjson::IStreamWrapper json_is(file);
			rapidjson::Document d;
			rapidjson::ParseResult ok = d.ParseStream(json_is);
			if (ok)
			{
				// volume renderer parameters
				if (d.HasMember(CFG_STEP_SIZE))
					step_size = d[CFG_STEP_SIZE].GetFloat();

				if (d.HasMember(CFG_OPACITY_CORRECTION))
					transfer_function.opacity_correction = d[CFG_OPACITY_CORRECTION].GetFloat();

				if (d.HasMember(CFG_SHADING))
					shading = d[CFG_SHADING].GetBool();

				if (d.HasMember(CFG_SHADING_LIGHT))
				{
					rapidjson::Value& bc = d[CFG_SHADING_LIGHT];
					if (bc.IsArray() && bc.Size() == 3)
						light_color = float3(bc[0].GetFloat(), bc[1].GetFloat(), bc[2].GetFloat());
				}
				if (d.HasMember(CFG_SHADING_AMBIENT))
					k_ambient = d[CFG_SHADING_AMBIENT].GetFloat();
				if (d.HasMember(CFG_SHADING_DIFFUSE))
					k_diffuse = d[CFG_SHADING_DIFFUSE].GetFloat();
				if (d.HasMember(CFG_SHADING_SPECULAR))
					k_specular = d[CFG_SHADING_SPECULAR].GetFloat();
				if (d.HasMember(CFG_SHADING_SHININESS))
					shininess = d[CFG_SHADING_SHININESS].GetFloat();

				if (d.HasMember(CFG_BACKGROUND_COLOR))
				{
					rapidjson::Value& bc = d[CFG_BACKGROUND_COLOR];
					if (bc.IsArray() && bc.Size() == 3)
						background_color = float3(bc[0].GetFloat(), bc[1].GetFloat(), bc[2].GetFloat());
				}

				// camera parameters
				if (d.HasMember(CFG_CAMERA_PHI))
					camera.getNavigator().phi = d[CFG_CAMERA_PHI].GetFloat();

				if (d.HasMember(CFG_CAMERA_THETA))
					camera.getNavigator().theta = d[CFG_CAMERA_THETA].GetFloat();

				if (d.HasMember(CFG_CAMERA_RADIUS))
					camera.getNavigator().radius = d[CFG_CAMERA_RADIUS].GetFloat();

				// volume
				if (d.HasMember(CFG_VOLUME))
				{
					volume_index = d[CFG_VOLUME].GetInt();
					switchVolume(volume_index);
				}

				// lookup-table
				if (d.HasMember(CFG_TRANSFERFUNCTION_LUT))
				{
					auto& lut = transfer_function.getLookupTable();
					lut.clear();
					for (rapidjson::SizeType i = 0; i < d[CFG_TRANSFERFUNCTION_LUT].Size(); i += 4)
					{
						float4 entry;
						entry.x = d[CFG_TRANSFERFUNCTION_LUT][i + 0].GetFloat();
						entry.y = d[CFG_TRANSFERFUNCTION_LUT][i + 1].GetFloat();
						entry.z = d[CFG_TRANSFERFUNCTION_LUT][i + 2].GetFloat();
						entry.w = d[CFG_TRANSFERFUNCTION_LUT][i + 3].GetFloat();
						lut.push_back(entry);
					}
				}

				// transferfunction
				if (d.HasMember(CFG_TRANSFERFUNCTION_RANGE_X) && d.HasMember(CFG_TRANSFERFUNCTION_RANGE_Y) && d.HasMember(CFG_TRANSFERFUNCTION_CPS))
				{
					transfer_function.setRange(math::uint2(d[CFG_TRANSFERFUNCTION_RANGE_X].GetUint(), d[CFG_TRANSFERFUNCTION_RANGE_Y].GetUint()));

					for (rapidjson::SizeType i = 0; i < d[CFG_TRANSFERFUNCTION_CPS].Size(); i++)
					{
						rapidjson::Value& json_cps = d[CFG_TRANSFERFUNCTION_CPS][i];
						rapidjson::Value& json_color = json_cps[CFG_TRANSFERFUNCTION_COLOR];

						float3 color = float3(json_color[0].GetFloat(), json_color[1].GetFloat(), json_color[2].GetFloat());
						size_t new_cps_index = transfer_function.newControlPointSet(color);
						auto& new_cps = transfer_function.getControlPointSet(new_cps_index);

						rapidjson::Value& json_cp = json_cps[CFG_TRANSFERFUNCTION_CP];
						if (json_cp.Size() % 2 != 0)
						{
							std::cout << "ERROR: Config file '" << config_file << "': control point array length" << std::endl;
							break;
						}
						for (rapidjson::SizeType j = 0; j < json_cp.Size(); j += 2)
							new_cps.control_points[json_cp[j].GetUint()] = json_cp[j + 1].GetUint();
					}
					transfer_function.changed();
				}

				camera.getNavigator().update();
			}
			else
				std::cout << "ERROR: Config file '" << config_file << "' not valid" << std::endl;
		}
		else
			std::cout << "ERROR: Config file '" << config_file << "' not found" << std::endl;
	}

	void VolumeRenderer::saveConfig()
	{
		rapidjson::Document d;
		d.SetObject();

		// volume renderer parameters
		d.AddMember(CFG_STEP_SIZE, step_size, d.GetAllocator());
		d.AddMember(CFG_OPACITY_CORRECTION, transfer_function.opacity_correction, d.GetAllocator());
		d.AddMember(CFG_SHADING, shading, d.GetAllocator());

		rapidjson::Value lc(rapidjson::kArrayType);
		lc.PushBack(light_color.x, d.GetAllocator());
		lc.PushBack(light_color.y, d.GetAllocator());
		lc.PushBack(light_color.z, d.GetAllocator());
		d.AddMember(CFG_SHADING_LIGHT, lc, d.GetAllocator());

		d.AddMember(CFG_SHADING_AMBIENT, k_ambient, d.GetAllocator());
		d.AddMember(CFG_SHADING_DIFFUSE, k_diffuse, d.GetAllocator());
		d.AddMember(CFG_SHADING_SPECULAR, k_specular, d.GetAllocator());
		d.AddMember(CFG_SHADING_SHININESS, shininess, d.GetAllocator());

		rapidjson::Value bc(rapidjson::kArrayType);
		bc.PushBack(background_color.x, d.GetAllocator());
		bc.PushBack(background_color.y, d.GetAllocator());
		bc.PushBack(background_color.z, d.GetAllocator());
		d.AddMember(CFG_BACKGROUND_COLOR, bc, d.GetAllocator());

		// camera parameters
		d.AddMember(CFG_CAMERA_PHI, camera.getNavigator().phi, d.GetAllocator());
		d.AddMember(CFG_CAMERA_THETA, camera.getNavigator().theta, d.GetAllocator());
		d.AddMember(CFG_CAMERA_RADIUS, camera.getNavigator().radius, d.GetAllocator());

		// volume
		d.AddMember(CFG_VOLUME, volume_index, d.GetAllocator());

		// transferfunction
		d.AddMember(CFG_TRANSFERFUNCTION_RANGE_X, transfer_function.getRange().x, d.GetAllocator());
		d.AddMember(CFG_TRANSFERFUNCTION_RANGE_Y, transfer_function.getRange().y, d.GetAllocator());

		rapidjson::Value json_cps_array(rapidjson::kArrayType);
		for (int i = 0; i < transfer_function.sizeCPS(); i++)
		{
			auto& cps = transfer_function.getControlPointSet(i);

			rapidjson::Value json_cps(rapidjson::kObjectType);

			rapidjson::Value color(rapidjson::kArrayType);
			color.PushBack(cps.color.x, d.GetAllocator());
			color.PushBack(cps.color.y, d.GetAllocator());
			color.PushBack(cps.color.z, d.GetAllocator());
			json_cps.AddMember(CFG_TRANSFERFUNCTION_COLOR, color, d.GetAllocator());

			rapidjson::Value json_cp(rapidjson::kArrayType);
			for (const auto& it : cps.control_points)
			{
				json_cp.PushBack(it.first, d.GetAllocator());
				json_cp.PushBack(it.second, d.GetAllocator());
			}
			json_cps.AddMember(CFG_TRANSFERFUNCTION_CP, json_cp, d.GetAllocator());
			json_cps_array.PushBack(json_cps, d.GetAllocator());
		}
		d.AddMember(CFG_TRANSFERFUNCTION_CPS, json_cps_array, d.GetAllocator());

		// lookup-table
		rapidjson::Value values(rapidjson::kArrayType);
		for (const auto& entry : transfer_function.getLookupTable())
		{
			values.PushBack(entry.x, d.GetAllocator());
			values.PushBack(entry.y, d.GetAllocator());
			values.PushBack(entry.z, d.GetAllocator());
			values.PushBack(entry.w, d.GetAllocator());
		}
		d.AddMember(CFG_TRANSFERFUNCTION_LUT, values, d.GetAllocator());

		// write
		std::ofstream ofs("saved_config.json");
		rapidjson::OStreamWrapper osw(ofs);
		rapidjson::Writer<rapidjson::OStreamWrapper> writer(osw);
		d.Accept(writer);
	}
