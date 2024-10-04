#include "ColorBuffer.h"
#include "framework/png.h"
#include <iostream>

ColorBuffer::ColorBuffer() : width(0), height(0), volume(0)
{
	glGenTextures(1, &volume);
}

ColorBuffer::~ColorBuffer()
{
	if (volume)
		glDeleteTextures(1, &volume);
}

void ColorBuffer::load()
{
	if (volume)
		glDeleteTextures(1, &volume);
	glGenTextures(1, &volume);

	glBindTexture(GL_TEXTURE_2D, volume);
	glTexImage2D(GL_TEXTURE_2D, 0, GL_RGBA, width, height, 0, GL_RGBA, GL_UNSIGNED_BYTE, data.data());
	glGenerateMipmap(GL_TEXTURE_2D);

	glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_CLAMP_TO_EDGE);
	glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_CLAMP_TO_EDGE);
	glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR_MIPMAP_LINEAR);
	glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_LINEAR);
}

void ColorBuffer::bind(unsigned target) const
{
	glActiveTexture(GL_TEXTURE0 + target);
	glBindTexture(GL_TEXTURE_2D, volume);
}

void ColorBuffer::resize(unsigned int texture_width, unsigned int texture_height)
{
	width = texture_width;
	height = texture_height;
	data.resize(width * height);
}

uint2 ColorBuffer::getSize() const
{
	return uint2(width, height);
}

void ColorBuffer::screenshot(const std::string& file_name) const
{
	std::string output_file = file_name + "-rendering.png";
	std::cout << "Creating " << width << "x" << height << " rendering image '" << output_file << "'" << std::endl;
	image<uint32_t> buffer(width, height);
	for (size_t y = 0; y < height; y++)
		for (size_t x = 0; x < width; x++)
			buffer(x, y) = *((uint32_t*)&data.at(x + y * width));
	PNG::saveImage(output_file.c_str(), buffer);
}
