#pragma once

#include <skypch.h>

namespace sky
{
class LogsPanel
{
  public:
	void render();

  private:
	struct LogEntry
	{
		std::string type;
		std::string timestamp;
		std::string message;
	};
	LogEntry extractLogEntryFromString(std::string &log);
};
}