#include <cassert>
#include <cmath>
#include "task2.h"

void createTransferfunctionLookupTable(std::vector<float4>& lut, const std::vector<ControlPointSet>& controlpoint_sets)
{
    // TODO: fill the Lookup Table 'lut'
    //
    // The lookup-table already has the right size.
    // Each entry consists of four float values:
    //  - xyz are the rgb values (range [0,1])
    //  - w is the alpha value (range [0,1])
    //
    // For each lookup-table entry determine the rgba values of the controlpoint-sets at and mix them together.
    // Ues TransferFunction::mixColors() to mix the values.
    //
    // Each controlpoint-set has a color and some controlpoints stored in a std::map (you can ignore the gui variables).
    // The map key corresponds to the density and the map value corresponds to the alpha value.
    // Since the values are stored in a map, the control-points are already sorted by their density values.
    // Each controlpoint-set contains at least two controlpoints for the min and max density (which corresponds to
    // the first and last entry of the lookup-table).
    // The color is the same for all density values!
    // The alpha value has to be linearly interpolated between control-points!
    for (size_t i = 0; i < lut.size(); ++i) {
        float total_alpha = 0.0f;
        float alpha = 0.0f;
        float3 rgb = {0.0f, 0.0f, 0.0f};

        for (const auto& cps : controlpoint_sets) {
            auto it = cps.control_points.find(i);
            rgb = cps.color;
            if (it != cps.control_points.end()) {
                alpha = static_cast<float>(it->second);
                alpha /= 255.0f;
            } else {
                auto it_next = cps.control_points.lower_bound(i);
                    auto it_prev = std::prev(it_next);
                    float x0 = static_cast<float>(it_prev->first);
                    float x1 = static_cast<float>(it_next->first);
                    float y0 = static_cast<float>(it_prev->second);
                    float y1 = static_cast<float>(it_next->second);
                    float density = i;

                    if(y1 < y0){
                        alpha = y0 - (y0 - y1) * (density - x0) / (x1 - x0);
                    }
                    else
                    {
                        alpha = y0 + (density - x0)* (y1 - y0) /(x1 - x0);
                    }
                    alpha /= 255.0f;
                    // x0 = 66, x1 = 76, y0 = 24, y1 = 0 66 + 2*
                }
                total_alpha += alpha;
            }
        lut[i] = TransferFunction::mixColors(lut[i].xyz(), rgb, lut[i].w, alpha);

        }
}


float4 lookupTransferFunction(const float density, const float opacity_correction, const std::vector<float4>& lut)
{
    // TODO: implement the transfer function lookup
    //
    // The lookup-table vector contains the rgba values for all possible densities.
    // Linearly interpolate between the entries of the lookup-table.
    // The opacity correction is used for scaling of the opacity values --- multiply the a-value by this factor.
    // Return the computed rgba value.
    // Determine the indices of the lookup table entries that the density falls between
    int lut_size = lut.size() - 1;
    int lower_index = std::floor(density);
    int upper_index = std::ceil(density);

    lower_index = math::clamp(lower_index, 0, lut_size);
    upper_index = math::clamp(upper_index, 0, lut_size);

    if (lower_index == upper_index) {
        float4 result = lut[lower_index];
        result.w *= opacity_correction; // Apply opacity correction
        return result;
    }

    const float4& lower_value = lut[lower_index];
    const float4& upper_value = lut[upper_index];

    float t = density - static_cast<float>(lower_index);

    float4 interpolated_value;
    interpolated_value.x = lower_value.x * (1 - t) + upper_value.x * t;
    interpolated_value.y = lower_value.y * (1 - t) + upper_value.y * t;
    interpolated_value.z = lower_value.z * (1 - t) + upper_value.z * t;
    interpolated_value.w = lower_value.w * (1 - t) + upper_value.w * t;

    interpolated_value.w *= opacity_correction;

    return interpolated_value;
}


bool intersectRayWithAABB(const float3& ray_start, const float3& ray_dir, const float3& aabb_min, const float3& aabb_max, float& t_near, float& t_far)
{
    // TODO: intersect the ray with the axis-aligned bounding box.
    //
    //  Set t_near and t_far to the distance to the entry and exit points of the ray
    //  Return true if the intersection happens and false otherwise.
    float t_near_temp, t_far_temp;
    float t1 = (aabb_min.x - ray_start.x) / ray_dir.x;
    float t2 = (aabb_max.x - ray_start.x) / ray_dir.x;
    float t3 = (aabb_min.y - ray_start.y) / ray_dir.y;
    float t4 = (aabb_max.y - ray_start.y) / ray_dir.y;
    float t5 = (aabb_min.z - ray_start.z) / ray_dir.z;
    float t6 = (aabb_max.z - ray_start.z) / ray_dir.z;

    t_near_temp = std::max({std::min(t1, t2), std::min(t3, t4), std::min(t5, t6)});
    t_far_temp = std::min({std::max(t1, t2), std::max(t3, t4), std::max(t5, t6)});

    if(t_near_temp > t_far_temp || t_far_temp < 0){
        return false;
    }
    t_near = t_near_temp;
    t_far = t_far_temp;
    return t_near <= t_far && t_far >= 0;
}


float interpolatedDensity(const Volume& volume, const float3& sample_position)
{
    // TODO: use trilinear interpolation to calculate the density value for the sample position
    //
    // Useful functions:
    //  - AABB minimum corner position:  Volume::getAABBMin()
    //  - AABB maximum corner position:  Volume::getAABBMax()
    //  - Volume size in voxels:         Volume::getVolumeSize()
    //  - Volume voxel grid ratio:       Volume::getRatio() , Volume::getRatioInv()
    //
    // The voxel density values can be accessed via volume[uint3(x,y,z)].
    // The voxel grid ratio defines the spacing between the voxels in x,y,z direction.
    // e.g. ratio.x is the distance between two voxels in x-direction
    // Return the computed density.

  // Useful functions:
  //  - AABB minimum corner position:  Volume::getAABBMin()
  //  - AABB maximum corner position:  Volume::getAABBMax()
  //  - Volume size in voxels:         Volume::getVolumeSize()
  //  - Volume voxel grid ratio:       Volume::getRatio() , Volume::getRatioInv()
  //
  // The voxel density values can be accessed via volume[uint3(x,y,z)].
  // The voxel grid ratio defines the spacing between the voxels in x,y,z direction.
  // e.g. ratio.x is the distance between two voxels in x-direction

  float3 volume_size = volume.getVolumeSize();
  float3 ratio_inv = volume.getRatioInv();
  float3 aabb_min = volume.getAABBMin();

  float3 sample_pos_voxel = (sample_position - aabb_min) * ratio_inv;
  // check if sample is out of bounds


  int p_min_x = static_cast<int>(floor(sample_pos_voxel.x));
  int p_min_y = static_cast<int>(floor(sample_pos_voxel.y));
  int p_min_z = static_cast<int>(floor(sample_pos_voxel.z));

  float t_x = sample_pos_voxel.x - static_cast<float>(p_min_x);
  float t_y = sample_pos_voxel.y - static_cast<float>(p_min_y);
  float t_z = sample_pos_voxel.z - static_cast<float>(p_min_z);

    if (sample_pos_voxel.x < 0.0f || static_cast<float>(p_min_x) >= volume_size.x - 1 ||
            sample_pos_voxel.y < 0.0f || static_cast<float>(p_min_y) >= volume_size.y - 1 ||
            sample_pos_voxel.z < 0.0f    || static_cast<float>(p_min_z) >= volume_size.z - 1) {
        return 0.0f;
    }

  p_min_x = math::clamp(p_min_x, 0, static_cast<int>(volume_size.x - 2));
  p_min_y = math::clamp(p_min_y, 0, static_cast<int>(volume_size.y - 2));
  p_min_z = math::clamp(p_min_z, 0, static_cast<int>(volume_size.z - 2));

  float v000 = static_cast<float>(volume[uint3(p_min_x, p_min_y, p_min_z)]);
  float v100 = static_cast<float>(volume[uint3(p_min_x + 1, p_min_y, p_min_z)]);
  float v010 = static_cast<float>(volume[uint3(p_min_x, p_min_y + 1, p_min_z)]);
  float v110 = static_cast<float>(volume[uint3(p_min_x + 1, p_min_y + 1, p_min_z)]);
  float v001 = static_cast<float>(volume[uint3(p_min_x, p_min_y, p_min_z + 1)]);
  float v101 = static_cast<float>(volume[uint3(p_min_x + 1, p_min_y, p_min_z + 1)]);
  float v011 = static_cast<float>(volume[uint3(p_min_x, p_min_y + 1, p_min_z + 1)]);
  float v111 = static_cast<float>(volume[uint3(p_min_x + 1, p_min_y + 1, p_min_z + 1)]);

  float c00 = v000 * (1 - t_x) + v100 * t_x;
  float c10 = v010 * (1 - t_x) + v110 * t_x;
  float c01 = v001 * (1 - t_x) + v101 * t_x;
  float c11 = v011 * (1 - t_x) + v111 * t_x;

  float c0 = c00 * (1 - t_y) + c10 * t_y;
  float c1 = c01 * (1 - t_y) + c11 * t_y;

  float interpolated_value = c0 * (1 - t_z) + c1 * t_z;

  return interpolated_value;

}

void volumeRaycasting(std::vector<float3> &ray_directions,
                      const int left, const int top, const int right, const int bottom,
                      const Camera& camera)
{
    // TODO: implement the volume raycasting algorithm
    //
    // Cast a ray for every pixel and save it in ray_directions (row-major order).
    // Make sure to normalize ray directions.
    //
    // The cameras's u,v,w vectors can be retrieved with camera.getU(), camera.getV(), camera.getW()
    //

    const float3& camera_position = camera.getPosition();
    const unsigned int r_x = camera.getViewport().x;
    const unsigned int r_y = camera.getViewport().y;
    const float f = camera.getFocalLength();
    float beta = camera.getFOV();
    float h_s = 2*f* std::tan(beta / 2) ;
    float w_s = h_s * static_cast<float>(r_x) / static_cast<float>(r_y);


    float3 w = camera.getW();
    float3 u = camera.getU();
    float3 v = camera.getV();


    for (int y = top; y < bottom; ++y) {
        for (int x = left; x < right; ++x) {

          float translated_x = (2 * (static_cast<float>(x) + 0.5f) / static_cast<float>(r_x) - 1) * w_s * 0.5f;
          float translated_y = (1 - 2 * (static_cast<float>(y) + 0.5f) / static_cast<float>(r_y)) * h_s * 0.5f;
          float3 ray = translated_x * u + translated_y * v - f * w;
          float3 ray_direction = normalize(ray);
          ray_directions[y * r_x + x] = ray_direction;
        }
    }
}


float3 reflect(const float3& I, const float3& N) {
  return  I - 2.0f * dot(N, I) * N;
}

uchar4 colorPixel(const float3& camera_position, const float3& ray_direction,
                  const Volume& volume, const float step_size, const float opacity_correction,
                  const float opacity_threshold, const float3& background_color, const float3& light_color,
                  const float k_ambient, const float k_diffuse, const float k_specular, const float shininess,
                  const bool shading, const std::vector<float4>& tf_lut)
{
  // TODO: implement color accumulation and (bonus) phong shading
  //
  //  1. Intersect ray with the AABB of volume to determine entry and exit point of the ray.
  //      If the ray does not intersect the AABB: pixel color = background color
  //  2. Front-to-back composition along the ray within the volume with the given stepsize + phong model if shading = true
  //      Test for early-ray-termination.
  //  3. Blend the accumulated color with background color, depending on the used opacity and the opacity threshold
  //	4. Map the colors from float [0,1] to discretized color values {0, ..., 255}
  //  5. Return result color
  //
  // Volume:
  //    - AABB minimum corner position: volume.getAABBMin()
  //    - AABB maximum corner position: volume.getAABBMax()
  //
  // Note: This function depends on all other tasks to work correctly.
  // Intersect ray with the AABB of the volume to determine entry and exit points
  float t_near, t_far;
  float3 aabb_min = volume.getAABBMin();
  float3 aabb_max = volume.getAABBMax();

  if (!intersectRayWithAABB(camera_position, ray_direction, aabb_min, aabb_max, t_near, t_far)) {
    return {static_cast<unsigned char>(background_color.x * 255),
                  static_cast<unsigned char>(background_color.y * 255),
                  static_cast<unsigned char>(background_color.z * 255), 255};
  }
  float3 accumulated_color = {0, 0, 0};
  float accumulated_opacity = 0;

  float3 ray_pos = camera_position + ray_direction * t_near;
  for (float t = t_near; t <= t_far; t += step_size) {


    float density = interpolatedDensity(volume, ray_pos);
    float4 tf_value = lookupTransferFunction(density, opacity_correction, tf_lut);
    float3 lighting = tf_value.xyz();

    if(shading){
      float3 gradient = calcGradient(volume, ray_pos);
      float3 V = normalize(camera_position - ray_pos);
      float3 N = gradient;
      float3 L = normalize(ray_pos - camera_position);
      float3 R = normalize(reflect(L, N));
      double diffuse = fmax(fmax(dot(N, L), 0.0f), 0.0f);
      double specular = pow(fmax(dot(R, V), 0.0f), shininess);
      lighting = lighting * static_cast<float>(fmin(k_ambient + k_diffuse * diffuse + k_specular * specular, 1.0f));
    }
    accumulated_color += (1 - accumulated_opacity) * tf_value.w * lighting * light_color;

    accumulated_opacity += (1 - accumulated_opacity) * tf_value.w;
    if (accumulated_opacity >= opacity_threshold) {
      break;
    }
    ray_pos = ray_pos + ray_direction * step_size;
  }

    if(accumulated_opacity < opacity_threshold){
        accumulated_opacity = math::clamp(accumulated_opacity, 0.0f, 1.0f);
        accumulated_color.x = math::clamp(accumulated_color.x, 0.0f, 1.0f);
        accumulated_color.y = math::clamp(accumulated_color.y, 0.0f, 1.0f);
        accumulated_color.z = math::clamp(accumulated_color.z, 0.0f, 1.0f);
        float4 accumulated_color_mixed = TransferFunction::mixColors(background_color,  accumulated_color ,  opacity_threshold - accumulated_opacity, accumulated_opacity );
        accumulated_color = math::clamp(accumulated_color_mixed.xyz(), 0.0f, 1.0f);
    }
    else {
        accumulated_color = math::clamp(accumulated_color, 0.0f, 1.0f);
    }

  return {static_cast<unsigned char>(accumulated_color.x * 255),
                static_cast<unsigned char>(accumulated_color.y * 255 ),
                static_cast<unsigned char>(accumulated_color.z * 255 ),
                static_cast<unsigned char>(255)};
}

float3 calcGradient(const Volume& volume, const float3& sample_position)
{
    // TODO (Bonus): use the central difference method to calculate the gradient for the sample position
    //
    // Useful functions:
    //  - Volume voxel grid ratio: Volume::getRatio()
    //
    // The Voxel grid ratio defines the spacing between the voxels in x,y,z direction.
    // e.g. ratio.x is the distance between two voxels in x-direction
    //
    // Hint: make use of your implementation of the interpolatedDensity() function.
    // Return the computed gradient.

  float3 ratio = volume.getRatio();

  float density_x1 = interpolatedDensity(volume, sample_position + float3(ratio.x, 0, 0));
  float density_x2 = interpolatedDensity(volume, sample_position - float3(ratio.x, 0, 0));
  float density_y1 = interpolatedDensity(volume, sample_position + float3(0, ratio.y, 0));
  float density_y2 = interpolatedDensity(volume, sample_position - float3(0, ratio.y, 0));
  float density_z1 = interpolatedDensity(volume, sample_position + float3(0, 0, ratio.z));
  float density_z2 = interpolatedDensity(volume, sample_position - float3(0, 0, ratio.z));

  float3 normal;
  normal.x = (density_x1 - density_x2) / 2;
  normal.y = (density_y1 - density_y2) / 2;
  normal.z = (density_z1 - density_z2) /2;

  return normalize(normal * volume.getRatio());
}
