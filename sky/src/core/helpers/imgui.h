#pragma once

#include <imgui.h>

namespace sky
{
namespace helper
{
bool imguiButton(std::string text, 
	ImVec2 size, 
	bool disabled = false, 
	std::string type = "default");
}
}