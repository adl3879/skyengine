#include "string.h"

namespace sky
{
namespace helper
{
std::string capitalizeString(const std::string &input)
{
    std::string result = input;
    if (!result.empty())
    {
        result[0] = std::toupper(result[0]); // Capitalize the first letter
        // Optional: Convert all remaining letters to lowercase (if desired)
        for (size_t i = 1; i < result.size(); ++i)
        {
            result[i] = std::tolower(result[i]);
        }
    }
    return result;
}
}
}