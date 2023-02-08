
#ifndef NEO_IMGUI_BFGIMGUI_H_
#define NEO_IMGUI_BFGIMGUI_H_

#include "imgui.h"

#include "../idlib/math/Vector.h"

// add custom functions for imgui
namespace ImGui
{

bool DragVec3( const char* label, idVec3& v, float v_speed = 1.0f,
			   float v_min = 0.0f, float v_max = 0.0f,
			   const char* display_format = "%.1f",
			   float power = 1.0f, bool ignoreLabelWidth = true );

bool DragVec3fitLabel( const char* label, idVec3& v, float v_speed = 1.0f,
					   float v_min = 0.0f, float v_max = 0.0f,
					   const char* display_format = "%.1f", float power = 1.0f );

}

#endif /* NEO_IMGUI_BFGIMGUI_H_ */
