#include "Transferfunction.h"
#include "imgui_transferfunction.h"

#include <string>

#include <imgui.h>
#define IMGUI_DEFINE_MATH_OPERATORS
#include <imgui_internal.h>

namespace
{
	ImVec2 CPToPxCoord(const ImVec2& cp, const ImVec2& bb_min, const ImVec2& scale, const ImVec2& max)
	{
		return bb_min + ImVec2(cp.x, max.y - cp.y) / scale;
	}

	ImVec2 CPToPxCoord(const float2& cp, const ImVec2& bb_min, const ImVec2& scale, const ImVec2& max)
	{
		return CPToPxCoord(ImVec2(cp.x, cp.y), bb_min, scale, max);
	}

	ImVec2 PxCoordToCP(const ImVec2& pxcoord, const ImVec2& bb_min, const ImVec2& scale, const ImVec2& max)
	{
		return ImVec2((pxcoord.x - bb_min.x) * scale.x, max.y - (pxcoord.y - bb_min.y) * scale.y);
	}

	ImVec2 PxCoordToCP(const float2& pxcoord, const ImVec2& bb_min, const ImVec2& scale, const ImVec2& max)
	{
		return PxCoordToCP(ImVec2(pxcoord.x, pxcoord.y), bb_min, scale, max);
	}
}

bool ImGuiTransferFunction(const char* label, const float3& background_color, TransferFunction& tf, const uint32_t max_iso_volume)
{
	const float CIRCLE_RADIUS_PX = 5;

	ImGuiIO& io = ImGui::GetIO();
	ImGuiStyle& style = ImGui::GetStyle();

	ImDrawList* draw_list = ImGui::GetWindowDrawList();
	ImGuiWindow* Window = ImGui::GetCurrentWindow();
	if (Window->SkipItems)
		return false;

	// Lookup generation button/checkbox
	ImGui::Checkbox("Restrict to max iso-value", &tf.gui_restrict_to_max_iso_value);
	if (ImGui::Button("Generate Transfer Function"))
		tf.gui_req_update_tf_lookup_table = true;
	ImGui::SameLine();

	// add/delete button for controlpoint sets
	bool new_tf = ImGui::Button("New");
	ImGui::SameLine();
	bool delete_tf = ImGui::Button("Delete");
	ImGui::SameLine();
	ImGui::Checkbox("Auto Update", &tf.gui_auto_update_tf_lookup_table);

	const uint2 umax(tf.gui_restrict_to_max_iso_value ? max_iso_volume : tf.getRange().x - 1, tf.getRange().y);
	const ImVec2 fmax((float)umax.x, (float)umax.y);

	// add controlpointset
	if (new_tf)
	{
		tf.newControlPointSet();
	}

	// delete controlpointset
	if (delete_tf && tf.gui_active_cps != -1)
	{
		tf.deleteControlPointSet(tf.gui_active_cps);
		tf.gui_active_cps = -1;
	}

	// controlpointset color picker
	if (tf.gui_active_cps != -1)
	{
		if (ImGui::ColorEdit3("Color", &tf.getControlPointSet(tf.gui_active_cps).color.x))
			tf.changed();
	}

	// draw transferfunction
	for (size_t i = 0; i < tf.sizeCPS(); i++)
	{
		ControlPointSet& cps = tf.getControlPointSet(i);
		ImVec4 color(cps.color.x, cps.color.y, cps.color.z, 1.f);

		ImGui::PushStyleColor(ImGuiCol_FrameBg, color);
		ImGui::PushStyleColor(ImGuiCol_FrameBgHovered, color);
		ImGui::PushStyleColor(ImGuiCol_FrameBgActive, ImVec4(color));
		ImGui::PushStyleColor(ImGuiCol_CheckMark, ImVec4(0.f, 0.f, 0.f, 1.f));
		ImGui::Checkbox(std::to_string(i).c_str(), &cps.gui_active);
		ImGui::PopStyleColor(4);

		// deactivate other transfer functions
		if (cps.gui_active && !cps.gui_h_active)
		{
			tf.gui_active_cps = i;
			for (size_t j = 0; j < tf.sizeCPS(); j++)
				if (i != j)
					tf.getControlPointSet(j).gui_active = tf.getControlPointSet(j).gui_h_active = false;
		}
		else if (!cps.gui_active && cps.gui_h_active)
		{
			tf.gui_active_cps = -1;
		}
		cps.gui_h_active = cps.gui_active;

		ImGui::SameLine();
	}
	ImGui::NewLine();

	// prepare graph
	ImVec2 margin_min(30, 30);
	ImVec2 margin_max(20, 50);
	ImVec2 size(ImGui::GetContentRegionAvail() - ImVec2(margin_min.x + margin_max.x, margin_min.y + margin_max.y));
	ImVec2 cursor_pos_bb_min = Window->DC.CursorPos + margin_min;
	ImRect bb(Window->DC.CursorPos + margin_min, Window->DC.CursorPos + margin_min + size);
	ImVec2 scale(fmax.x / size.x, fmax.y / size.y);

	// draw graph area and labels
	ImGui::RenderFrame(bb.Min, bb.Max, ImGui::GetColorU32(ImGuiCol_FrameBg, 1), true, style.FrameRounding);
	const ImGuiID id = Window->GetID(label);
	bool graph_hovered = ImGui::ItemHoverable(bb, id);

	// draw iso-value/color mapping
	ImVec2 bb_iso_color_min = cursor_pos_bb_min + ImVec2(0.f, size.y + 30.f);
	ImRect bb_iso_color(bb_iso_color_min, bb_iso_color_min + ImVec2(size.x, 25));
	draw_list->AddRectFilled(bb_iso_color.Min, bb_iso_color.Max, ImGui::GetColorU32(ImVec4(0.f, 0.f, 0.f, 1.f)));
	for (uint32_t w = 0; w < size.x; w++)
	{
		uint32_t iso_value = (uint32_t)(w * scale.x);
		float3 c = tf.lookup(static_cast<float>(iso_value)).xyz();
		float a = tf.lookup(static_cast<float>(iso_value)).w;
		c = a * c + (1 - a) * background_color;
		draw_list->AddRectFilled(bb_iso_color.Min + ImVec2((float)w, 0), bb_iso_color.Min + ImVec2((float)w + 1, 25), ImGui::GetColorU32(ImVec4(c.x, c.y, c.z, 1.f)));
	}

	// opacity axis labels
	ImGui::SetCursorScreenPos(cursor_pos_bb_min + ImVec2(-20.f, -20.f));
	ImGui::Text("Opacity");
	ImGui::SetCursorScreenPos(cursor_pos_bb_min + ImVec2(-20.f, size.y - 10.f));
	ImGui::Text("0");
	ImGui::SetCursorScreenPos(cursor_pos_bb_min + ImVec2(-margin_min.x, 0.f));
	ImGui::Text("255");
	// iso-value axis labels
	ImGui::SetCursorScreenPos(cursor_pos_bb_min + ImVec2(size.x / 2.f - 15.f, size.y + 10.f));
	ImGui::Text("Iso-value");
	ImGui::SetCursorScreenPos(cursor_pos_bb_min + ImVec2(0.f, size.y + 10.f));
	ImGui::Text("0");
	ImGui::SetCursorScreenPos(cursor_pos_bb_min + ImVec2(size.x - 15.f, size.y + 10.f));
	ImGui::Text(std::to_string(umax.x).c_str());

	// draw graph grid
	int grid_x = 10;
	int grid_y = 5;
	ImVec2 grid_spacing(size.x / grid_x, size.y / grid_y);
	for (float i = 0; i <= grid_x; i++)
	{
		draw_list->AddLine(
		    ImVec2(bb.Min.x + i * grid_spacing.x, bb.Min.y),
		    ImVec2(bb.Min.x + i * grid_spacing.x, bb.Max.y),
		    ImGui::GetColorU32(ImGuiCol_TextDisabled));
	}
	for (int i = 0; i <= grid_y; i++)
	{
		draw_list->AddLine(
		    ImVec2(bb.Min.x, bb.Min.y + i * grid_spacing.y),
		    ImVec2(bb.Max.x, bb.Min.y + i * grid_spacing.y),
		    ImGui::GetColorU32(ImGuiCol_TextDisabled));
	}

	// add controlpoints as invisible button for interaction
	if (tf.gui_active_cps != -1)
	{
		ControlPointSet& cps = tf.getControlPointSet(tf.gui_active_cps);

		bool any_cp_hovered = false;
		auto cp_it = cps.control_points.begin();
		while (cp_it != cps.control_points.end())
		{
			ImVec2 cp((float)cp_it->first, (float)cp_it->second);
			ImVec2 px_coord = CPToPxCoord(cp, bb.Min, scale, fmax);
			ImGui::SetCursorScreenPos(px_coord - ImVec2(CIRCLE_RADIUS_PX, CIRCLE_RADIUS_PX));
			ImGui::InvisibleButton("", ImVec2(2 * CIRCLE_RADIUS_PX, 2 * CIRCLE_RADIUS_PX));

			bool is_hovered = false;
			if (ImGui::IsItemHovered())
				any_cp_hovered = is_hovered = true;
			bool is_active = ImGui::IsItemActive();

			// show tooltip when hovered
			if (is_active || is_hovered)
			{
				ImGui::SetTooltip("(%3.0f, %3.0f)", cp.x, cp.y);
			}

			// delete controlpoint
			if (is_hovered && ImGui::IsMouseClicked(1))
			{
				if (std::prev(cp_it) != cps.control_points.end() && std::next(cp_it) != cps.control_points.end())
				{
					cps.control_points.erase(cp_it);
					tf.changed();
					break;
				}
			}

			// drag controlpoint
			if (is_hovered && ImGui::IsMouseDown(0))
				cps.gui_cp_drag = std::distance(cps.control_points.begin(), cp_it);
			if (!ImGui::IsMouseDown(0))
				cps.gui_cp_drag = -1;

			if (std::distance(cps.control_points.begin(), cp_it) == cps.gui_cp_drag)
			{
				float cp_min_x, cp_max_x;
				if (std::prev(cp_it) == cps.control_points.end())
				{
					cp_min_x = cp_max_x = 0;
				}
				else if (std::next(cp_it) == cps.control_points.end())
				{
					cp_min_x = cp_max_x = fmax.x;
				}
				else
				{
					cp_min_x = std::prev(cp_it)->first + 1.f;
					cp_max_x = std::next(cp_it)->first - 1.f;
				}

				ImVec2 new_px_coord;
				new_px_coord.x = ImGui::GetIO().MousePos.x;
				new_px_coord.y = ImGui::GetIO().MousePos.y;

				ImVec2 changed_cp = PxCoordToCP(new_px_coord, bb.Min, scale, fmax);
				changed_cp.x = std::min(cp_max_x, std::max(cp_min_x, changed_cp.x));
				changed_cp.y = std::min(fmax.y, std::max(0.f, changed_cp.y));

				cps.control_points.erase(cp_it->first);
				cps.control_points[(uint32_t)changed_cp.x] = (uint32_t)changed_cp.y;
				tf.changed();
				break;
			}
			cp_it++;
		}

		// add new control point
		if (graph_hovered && !any_cp_hovered && ImGui::IsMouseClicked(0))
		{
			ImVec2 new_cp = PxCoordToCP(ImGui::GetMousePos(), bb.Min, scale, fmax);
			uint32_t new_cp_x = (uint32_t)new_cp.x;
			uint32_t new_cp_y = (uint32_t)new_cp.y;
			if (cps.control_points.find(new_cp_x) == cps.control_points.end())
			{
				cps.control_points[new_cp_x] = new_cp_y;
				tf.changed();
			}
		}
	}

	// draw control points and lines
	for (size_t i = 0; i < tf.sizeCPS(); i++)
	{
		ControlPointSet& cps = tf.getControlPointSet(i);
		ImColor color(cps.color.x, cps.color.y, cps.color.z, 0.9f);
		auto cps_it = cps.control_points.begin();
		while (cps_it != cps.control_points.end())
		{
			ImVec2 cp_in_px = CPToPxCoord(ImVec2((float)cps_it->first, (float)cps_it->second), bb.Min, scale, fmax);
			if (tf.gui_active_cps == i)
				draw_list->AddCircleFilled(cp_in_px, CIRCLE_RADIUS_PX, color);

			auto cps_prev = std::prev(cps_it);
			if (cps_prev != cps.control_points.end())
			{
				ImVec2 last_cp_in_px = CPToPxCoord(ImVec2((float)cps_prev->first, (float)cps_prev->second), bb.Min, scale, fmax);
				draw_list->AddLine(last_cp_in_px, cp_in_px, color, 1.5f);
			}
			cps_it++;
		}
	}
	return tf.hasChanged();
}
