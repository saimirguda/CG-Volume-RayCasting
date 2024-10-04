#ifndef INCLUDED_TASK2
#define INCLUDED_TASK2

#pragma once

#include "Camera.h"
#include "ColorBuffer.h"
#include "Transferfunction.h"
#include "Volume.h"
#include "math/vector.h"

void createTransferfunctionLookupTable(std::vector<float4>& lut, const std::vector<ControlPointSet>& controlpoint_sets);

float4 lookupTransferFunction(float density, float opacity_correction, const std::vector<float4>& lut);

bool intersectRayWithAABB(const float3& ray_start, const float3& ray_dir, const float3& aabb_min, const float3& aabb_max, float& hit_distance_near, float& hit_distance_far);

float interpolatedDensity(const Volume& volume, const float3& sample_position);

float3 calcGradient(const Volume& volume, const float3& sample_position);

void volumeRaycasting(std::vector<float3> &ray_directions, int left, int top, int right, int bottom, const Camera& camera);

uchar4 colorPixel(const float3& camera_position, const float3& ray_direction, const Volume& volume, float step_size, float opacity_correction, float opacity_threshold, const float3& background_color, const float3& light_color, float k_ambient, float k_diffuse, float k_specular, float shininess, bool shading, const std::vector<float4>& tf_lut);

#endif // INCLUDED_TASK2
