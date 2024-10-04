#ifndef INCLUDED_CAMERA
#define INCLUDED_CAMERA

#pragma once

#include "OrbitalNavigator.h"
#include "math/vector.h"

class Camera
{
private:
	OrbitalNavigator& navigator;

	float near_plane;
	float fov;

	int viewport_width;
	int viewport_height;

public:
	Camera(OrbitalNavigator& navigator, float near_plane, float fov, int viewport_width, int viewport_height);

	OrbitalNavigator& getNavigator();

	void setViewport(int viewport_width, int viewport_height);

	int2 getViewport() const;
	float getFOV() const;
	float getFocalLength() const;
	const float3& getPosition() const;
	const float3& getU() const;
	const float3& getV() const;
	const float3& getW() const;
};

#endif // INCLUDED_CAMERA
