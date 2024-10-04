#include "Volume.h"
#include <fstream>
#include <iostream>

Volume::Volume() : max_iso_value(0), volume_loaded(false)
{
}

bool Volume::isLoaded() const
{
	return volume_loaded;
}

uint32_t Volume::getMaxIsoValue() const
{
	return max_iso_value;
}

bool Volume::load(const std::string& path_to_file, uint32_t component_size_, const uint3& size_, const float3& scaling_, bool invert_y, Axes axes)
{
	size_t size_of_data = size_.x * size_.y * size_.z * component_size_;
	data.resize(size_of_data);

	std::ifstream file(path_to_file, std::ios::binary);
	if (file.is_open())
	{
		std::vector<uint8_t> raw;
		raw.resize(size_of_data);

		file.read((char*)raw.data(), raw.size());
		file.close();

		component_size = component_size_;

		// store raw data
		uint3 index(0);
		max_iso_value = 0;
		for (index.x = 0; index.x < size_.x; index.x++)
			for (index.y = 0; index.y < size_.y; index.y++)
				for (index.z = 0; index.z < size_.z; index.z++)
				{
					size_t i_raw = index.x + index.y * size_.x + index.z * size_.x * size_.y;
					size_t i_data = 0;
					switch (axes)
					{
						case NO_SWITCH:
							i_data = index.x + (invert_y ? (size_.y - 1) - index.y : index.y) * size_.x + index.z * size_.x * size_.y;
							break;
						case SWITCH_XY:
							i_data = index.y + (invert_y ? (size_.x - 1) - index.x : index.x) * size_.y + index.z * size_.y * size_.x;
							break;
						case SWITCH_YZ:
							i_data = index.x + (invert_y ? (size_.z - 1) - index.z : index.z) * size_.x + index.y * size_.x * size_.z;
							break;
						default:
							throw std::runtime_error("Volume::load(): Invalid Axes configuration");
					}
					switch (component_size)
					{
						case 1:
							((uint8_t*)data.data())[i_data] = ((uint8_t*)raw.data())[i_raw];
							max_iso_value = std::max(max_iso_value, (uint32_t)((uint8_t*)data.data())[i_data]);
							break;
						case 2:
							// swap byte order!
							((uint16_t*)data.data())[i_data] = (uint16_t)((uint8_t*)raw.data())[i_raw * component_size] << 8 | ((uint8_t*)raw.data())[i_raw * component_size + 1];
							max_iso_value = std::max(max_iso_value, (uint32_t)((uint16_t*)data.data())[i_data]);
							break;
						default:
							throw std::runtime_error("Volume::load(): Invalid Component Size");
					}
				}

		// store axes configuration
		float3 scaling;
		switch (axes)
		{
			case NO_SWITCH:
				size = size_;
				scaling = scaling_;
				break;
			case SWITCH_XY:
				size.x = size_.y;
				size.y = size_.x;
				size.z = size_.z;
				scaling.x = scaling_.y;
				scaling.y = scaling_.x;
				scaling.z = scaling_.z;
				break;
			case SWITCH_YZ:
				size.x = size_.x;
				size.y = size_.z;
				size.z = size_.y;
				scaling.x = scaling_.x;
				scaling.y = scaling_.z;
				scaling.z = scaling_.y;
				break;
		}

		// calculate voxel grid ratio
		float voxel_spacing = 0.1f;
		ratio = voxel_spacing * scaling;
		inv_ratio = pow(ratio, -1.f);

		// calculate bounding box
		aabb_max = float3(static_cast<float>(size.x - 1), static_cast<float>(size.y - 1), static_cast<float>(size.z - 1)) * ratio * 0.5f;
		aabb_min = -aabb_max;

		volume_loaded = true;
	}
	else
	{
		std::cout << "File '" << path_to_file << "' not found!" << std::endl;
		volume_loaded = false;
	}
	return volume_loaded;
}

const float3& Volume::getAABBMin() const
{
	return aabb_min;
}

const float3& Volume::getAABBMax() const
{
	return aabb_max;
}

const uint3& Volume::getVolumeSize() const
{
	return size;
}

const float3& Volume::getRatio() const
{
	return ratio;
}

const float3& Volume::getRatioInv() const
{
	return inv_ratio;
}
