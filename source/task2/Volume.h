#ifndef INCLUDED_VOLUME
#define INCLUDED_VOLUME

#pragma once

#include "math/vector.h"
#include <iostream>
#include <memory>
#include <string>
#include <vector>

class Volume
{
public:
	enum Axes
	{
		NO_SWITCH,
		SWITCH_XY,
		SWITCH_YZ
	};

private:
	uint3 size;
	float3 aabb_min;
	float3 aabb_max;
	float3 ratio;
	float3 inv_ratio;
	uint32_t max_iso_value;
	std::vector<uint8_t> data;
	uint8_t component_size;
	bool volume_loaded;

public:
	Volume();

	bool load(const std::string& path_to_file, uint32_t component_size, const uint3& size, const float3& scaling, bool invert_y, Axes axes);
	bool isLoaded() const;
	uint32_t getMaxIsoValue() const;

	const float3& getAABBMin() const;
	const float3& getAABBMax() const;
	const uint3& getVolumeSize() const;
	const float3& getRatio() const;
	const float3& getRatioInv() const;

	uint32_t operator[](const math::uint3& index) const
	{
		size_t i = index.x + index.y * size.x + index.z * size.x * size.y;
		switch (component_size)
		{
			case 1:
				return ((uint8_t*)data.data())[i];
			case 2:
				return ((uint16_t*)data.data())[i];
			default:
				throw std::runtime_error("Volume::operator[]: Invalid Component Size");
		}
	}
};

#endif // INCLUDED_VOLUME
