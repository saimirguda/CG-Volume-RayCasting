#include "Camera.h"

Camera::Camera(OrbitalNavigator& _navigator, float near_plane, float fov, int viewport_width, int viewport_height) : navigator(_navigator), near_plane(near_plane), fov(fov), viewport_width(std::max(0, viewport_width)), viewport_height(std::max(0, viewport_height))
{
}

OrbitalNavigator& Camera::getNavigator()
{
	return navigator;
}

void Camera::setViewport(int viewport_width, int viewport_height)
{
	this->viewport_width = std::max(0, viewport_width);
	this->viewport_height = std::max(0, viewport_height);
}

int2 Camera::getViewport() const
{
	return int2(viewport_width, viewport_height);
}

float Camera::getFOV() const
{
	return fov * (3.1415f / 180.f);
}

float Camera::getFocalLength() const
{
	return near_plane;
}

const float3& Camera::getU() const
{
	return navigator.u;
}

const float3& Camera::getV() const
{
	return navigator.v;
}

const float3& Camera::getW() const
{
	return navigator.w;
}

const float3& Camera::getPosition() const
{
	return navigator.position;
}
