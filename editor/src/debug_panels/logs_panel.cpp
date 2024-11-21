#include "logs_panel.h"

#include <imgui.h>
#include "core/log/log.h"
#include "core/helpers/string.h"

namespace sky
{
bool showInfo =     true;
bool showWarning =  true;
bool showError =    true;
bool clearOnPlay =  false;

void LogsPanel::render() 
{
	ImGui::Begin("Log   ");
    //static ImGuiTextFilter logFilter; // For filtering log messages
    //logFilter.Draw("Filter", -1.0f);
    {
        auto &logs = Log::getLogSink()->getLogEntries();

        ImGui::Dummy({0, 3});
        ImGui::BeginGroup();

        // Top controls
        if (ImGui::Button("Clear", {100, 40}))
        {
            logs.clear();
        }
        ImGui::SameLine();
        ImGui::SetCursorPosY(ImGui::GetCursorPosY() + 5);
        ImGui::Checkbox("Clear on Play", &clearOnPlay);

        // End the group for top controls
        ImGui::EndGroup();

        // Add vertical space between controls and type filters
        ImGui::SameLine();
        ImGui::SetCursorPosX(ImGui::GetWindowWidth() - 320);

        // Start the group for type filters
        ImGui::BeginGroup();

        // Type filters
        ImGui::Checkbox("Info", &showInfo);
        ImGui::SameLine();
        ImGui::Checkbox("Warning", &showWarning);
        ImGui::SameLine();
        ImGui::Checkbox("Error", &showError);

        // End the group for type filters
        ImGui::EndGroup();
        ImGui::Dummy({0, 3});

        // Logs table
        if (ImGui::BeginTable("LogTable", 3,
                              ImGuiTableFlags_RowBg | ImGuiTableFlags_Borders | ImGuiTableFlags_Resizable))
        {
            ImVec2 originalPadding = ImGui::GetStyle().CellPadding;

            // Set the padding for the header (first row)
            ImVec2 headerPadding = ImVec2(20, 10); // Increase padding for header row
            ImGui::PushStyleVar(ImGuiStyleVar_CellPadding, headerPadding);

            ImGui::TableSetupColumn("Type");
            ImGui::TableSetupColumn("Timestamp");
            ImGui::TableSetupColumn("Message");
            ImGui::TableHeadersRow();
            
            ImGui::PopStyleVar();

            ImGui::PushStyleVar(ImGuiStyleVar_CellPadding, originalPadding);

			for (const auto &entry : logs)
			{
                auto msg = std::string(entry.message);
                auto log = extractLogEntryFromString(msg);

				// Filter by type
				if ((log.type == "info" && !showInfo) || (log.type == "warning" && !showWarning) ||
					(log.type == "error" && !showError))
				{
					continue;
				}

				// Set tint for type column based on log type
				if (log.type == "warning")
					ImGui::PushStyleColor(ImGuiCol_Text, {0.9f, 0.8f, 0.3f, 1.0f}); // Yellow for warning
				else if (log.type == "error")
					ImGui::PushStyleColor(ImGuiCol_Text, {0.9f, 0.3f, 0.3f, 1.0f}); // Red for error
				else if (log.type == "info")
					ImGui::PushStyleColor(ImGuiCol_Text, {0.4f, 0.8f, 0.4f, 1.0f}); // Green for info
                else
                    ImGui::PushStyleColor(ImGuiCol_Text, {0.6f, 0.6f, 0.6f, 1.0f}); // Green for info

				// Render the table row
				ImGui::TableNextRow();
				ImGui::TableNextColumn();
				ImGui::Text("%s", helper::capitalizeString(log.type).c_str()); // Type column
				// Pop the style color to restore the default
				ImGui::PopStyleColor();
				ImGui::TableNextColumn();
				ImGui::Text("%s", log.timestamp.c_str()); // Timestamp column
				ImGui::TableNextColumn();
				ImGui::Text("%s", log.message.c_str()); // Message column

			}
            ImGui::PopStyleVar();
            ImGui::EndTable();
        }

        if (ImGui::GetScrollY() >= ImGui::GetScrollMaxY())
        {
            ImGui::SetScrollHereY(1.0f); // Auto-scroll to the bottom
        }
    }
	ImGui::End();
}

LogsPanel::LogEntry LogsPanel::extractLogEntryFromString(std::string &log) 
{
    log.erase(log.find_last_not_of(" \t\r\n") + 1); // Trim trailing whitespace

    LogEntry entry;
    // Regular expression to match the log format
    std::regex logRegex(R"(\[(.*?)\] \[(.*?)\] \[(.*?)\] (.*))");
    std::smatch match;

    if (std::regex_match(log, match, logRegex))
    {
        // Extract components of the log entry
        entry.timestamp = match[1].str();  // [timestamp]
        //entry.loggerName = match[2].str(); // [logger name]
        entry.type = match[3].str();       // [log level/type]
        entry.message = match[4].str();    // message
    }
    else
    {
        std::cerr << "Log string format is invalid: " << log << std::endl;
    }

    return entry;
}
} // namespace sky