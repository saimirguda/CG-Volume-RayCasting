#ifndef INCLUDED_IMGUI_TRANSFERFUNCTION
#define INCLUDED_IMGUI_TRANSFERFUNCTION

#pragma once

#include <math/vector.h>

class TransferFunction;

bool ImGuiTransferFunction(const char* label, const float3& background_color, TransferFunction& tf, const uint32_t max_iso_volume);

#endif // INCLUDED_IMGUI_TRANSFERFUNCTION
