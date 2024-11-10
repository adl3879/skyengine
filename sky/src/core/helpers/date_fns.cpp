#include "date_fns.h"


namespace sky
{
namespace helper
{
std::string getCurrentDate()
{
    auto now = std::chrono::system_clock::now();
    std::time_t nowTime = std::chrono::system_clock::to_time_t(now);
    const std::tm *localTime = std::localtime(&nowTime);

    std::ostringstream dateStream;
    dateStream << std::put_time(localTime, "%Y-%m-%d %H:%M:%S");
    return dateStream.str();
}
} // namespace helper
} // namespace sky