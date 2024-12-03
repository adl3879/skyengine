#pragma once

#include <imgui.h>
#include <glm/glm.hpp>
#include "graphics/vulkan/vk_types.h"

namespace sky
{
namespace helper
{
bool imguiButton(std::string text, 
	ImVec2 size, 
	bool disabled = false, 
	std::string type = "default");
void imguiCenteredText(std::string text);
void imguiDrawVec3Control(const std::string &label, glm::vec3 &values, float resetValue = 0.0f, float columnWidth = 100.0f);
void imguiCollapsingHeaderStyle(const char* label, std::function<void()> fn, bool show = true, bool closed = false);
void drawButtonImage(ImageID img, ImU32 tintNormal, ImU32 tintHovered, ImU32 tintPressed);
}
}