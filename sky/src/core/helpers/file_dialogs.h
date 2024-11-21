#pragma once

#include <skypch.h>

namespace sky
{
namespace helper
{
std::string openFile(const char *filter);
std::string openDirectory();
std::string saveFile(const char *filter);
}
}