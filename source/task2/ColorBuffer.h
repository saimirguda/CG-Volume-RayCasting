#ifndef INCLUDED_COLOR_BUFFER
#define INCLUDED_COLOR_BUFFER

#pragma once

#include "image.h"
#include "math/vector.h"
#include "rgba8.h"
#include <GL/gl.h>
#include <string>
#include <vector>

class ColorBuffer
{
private:
	unsigned int width;
	unsigned int height;

	std::vector<math::uchar4> data;

public:
	ColorBuffer();
	~ColorBuffer();

	ColorBuffer(const ColorBuffer&) = delete;
	ColorBuffer& operator=(const ColorBuffer&) = delete;

	void load();
	void bind(unsigned target = 0) const;
	void resize(unsigned int texture_width, unsigned int texture_height);
	uint2 getSize() const;

	GLuint volume;

	void screenshot(const std::string& file_name) const;

	math::uchar4& operator[](const math::uint2& index)
	{
		return data[index.x + index.y * width];
	}
};

#endif // INCLUDED_COLOR_BUFFER
