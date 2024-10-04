#ifndef INCLUDED_TRANSFER_FUNCTION
#define INCLUDED_TRANSFER_FUNCTION

#pragma once

#include "math/vector.h"
#include <array>
#include <map>
#include <vector>

struct ControlPointSet
{
	std::map<uint32_t, uint32_t> control_points;
	float3 color;

	// helper variables for gui
	bool gui_active;
	bool gui_h_active;
	size_t gui_cp_drag;

	ControlPointSet(uint32_t range, const float3& color_) : color(color_), gui_active(false), gui_h_active(false), gui_cp_drag(-1)
	{
		control_points[0] = control_points[range - 1] = 0;
	}
};

class TransferFunction
{
private:
	std::vector<ControlPointSet> cps;
	std::vector<float4> lookup_table;
	uint2 range;

	bool needs_update;

public:
	TransferFunction();

	void changed();
	bool hasChanged() const;

	void setRange(uint2 range);
	uint2 getRange() const;

	size_t sizeCPS() const;
	size_t newControlPointSet(const float3& color = float3(1.f, 0.f, 0.f));
	void deleteControlPointSet(size_t index);
	ControlPointSet& getControlPointSet(size_t index);
	std::vector<float4>& getLookupTable();

	void updateLookupTable();

	float4 lookup(float density) const;

	static float4 mixColors(const float3& rgb1, const float3& rgb2, float alpha1, float alpha2);

	float opacity_correction;

	void writeLookupTableImage(const std::string& file_name) const;
	void writeTransferLookupImage(const std::string& file_name, unsigned int upscale) const;

	// helper variables for gui
	size_t gui_active_cps;
	bool gui_auto_update_tf_lookup_table;
	bool gui_req_update_tf_lookup_table;
	bool gui_restrict_to_max_iso_value;
};

#endif // INCLUDED_TRANSFER_FUNCTION
