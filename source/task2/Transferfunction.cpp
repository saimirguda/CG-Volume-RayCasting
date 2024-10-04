#include "Transferfunction.h"
#include "framework/png.h"
#include "task2.h"

TransferFunction::TransferFunction() : opacity_correction(1.f), gui_active_cps(-1), needs_update(false), gui_auto_update_tf_lookup_table(true), gui_req_update_tf_lookup_table(false), gui_restrict_to_max_iso_value(false)
{
}

void TransferFunction::changed()
{
	needs_update = true;
}

bool TransferFunction::hasChanged() const
{
	return needs_update;
}

void TransferFunction::setRange(uint2 range)
{
	this->range = range;
	lookup_table.resize(range.x);
	cps.clear();
	gui_active_cps = -1;
	changed();
}

uint2 TransferFunction::getRange() const
{
	return range;
}

size_t TransferFunction::sizeCPS() const
{
	return cps.size();
}

size_t TransferFunction::newControlPointSet(const float3& color)
{
	changed();
	cps.push_back(ControlPointSet(range.x, color));
	return cps.size() - 1;
}

void TransferFunction::deleteControlPointSet(size_t index)
{
	changed();
	if (index >= cps.size())
		throw std::runtime_error("TransferFunction::deleteTransferFunction(): Invalid transferfunction index");
	cps.erase(cps.begin() + index);
}

ControlPointSet& TransferFunction::getControlPointSet(size_t index)
{
	if (index >= cps.size())
		throw std::runtime_error("TransferFunction::getTransferFunction(): Invalid transferfunction index");
	return cps.at(index);
}

std::vector<float4>& TransferFunction::getLookupTable()
{
	return lookup_table;
}

float4 TransferFunction::lookup(float density) const
{
	return lookupTransferFunction(density, opacity_correction, lookup_table);
}

void TransferFunction::updateLookupTable()
{
	if ((needs_update && gui_auto_update_tf_lookup_table) || gui_req_update_tf_lookup_table)
	{
		createTransferfunctionLookupTable(lookup_table, cps);
		needs_update = false;
		gui_req_update_tf_lookup_table = false;
	}
}

float4 TransferFunction::mixColors(const float3& rgb1, const float3& rgb2, float alpha1, float alpha2)
{
	const float a_sum = (alpha1 + alpha2);
	float a1_w = 0.f;
	float a2_w = 0.f;
	if (a_sum > 0.f)
	{
		a1_w = alpha1 / a_sum;
		a2_w = alpha2 / a_sum;
	}
	float3 rgb_new = rgb1 * a1_w + rgb2 * a2_w;
	return float4(rgb_new.x, rgb_new.y, rgb_new.z, std::max(alpha1, alpha2));
}

void TransferFunction::writeLookupTableImage(const std::string& file_name) const
{
	std::string output_file = file_name + "-lut.png";
	std::cout << "Creating lookup-table image '" << output_file << "'" << std::endl;
	const size_t height = 25;
	const size_t width = lookup_table.size();
	image<uint32_t> buffer(width, height);
	for (size_t x = 0; x < width; x++)
	{
		float4 v = lookup_table.at(x) * 255.f;
		uchar4 c(static_cast<unsigned char>(v.x), static_cast<unsigned char>(v.y), static_cast<unsigned char>(v.z), static_cast<unsigned char>(v.w));
		for (size_t y = 0; y < height; y++)
			buffer(x, y) = *((uint32_t*)&c);
	}
	PNG::saveImage(output_file.c_str(), buffer);
}

void TransferFunction::writeTransferLookupImage(const std::string& file_name, unsigned int upscale) const
{
	std::string output_file = file_name + "-lookup.png";
	std::cout << "Creating table-lookup image '" << output_file << "'" << std::endl;
	const size_t height = 25;
	const size_t width = upscale * (lookup_table.size() - 1) + 1;
	image<uint32_t> buffer(width, height);
	for (size_t x = 0; x < width; x++)
	{
		float4 v = lookupTransferFunction(static_cast<float>(x) / static_cast<float>(upscale), 1.f, lookup_table) * 255.f;
		uchar4 c(static_cast<unsigned char>(v.x), static_cast<unsigned char>(v.y), static_cast<unsigned char>(v.z), static_cast<unsigned char>(v.w));
		for (size_t y = 0; y < height; y++)
			buffer(x, y) = *((uint32_t*)&c);
	}
	PNG::saveImage(output_file.c_str(), buffer);
}
