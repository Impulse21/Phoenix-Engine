#include "Widgets.h"

#include <imgui.h>

using namespace PhxEngine;

PhxEditor::ConsoleLogWidget::ConsoleLogWidget()
{
	Logger::RegisterLogCallback([this](Logger::LogEntry const& e) {
		this->LogCallback(e);
		});
}

void PhxEditor::ConsoleLogWidget::OnImGuiRender(bool displayWindow)
{
	static const ImGuiTableFlags table_flags =
		ImGuiTableFlags_RowBg |
		ImGuiTableFlags_BordersOuter |
		ImGuiTableFlags_ScrollX |
		ImGuiTableFlags_ScrollY;

	static const ImVec2 size = ImVec2(-1.0f, -1.0f);

	ImGui::Begin(WidgetTitle<ConsoleLogWidget>::Name(), &displayWindow);
	if (ImGui::BeginTable("##widget_console_content", 1, table_flags, size))
	{
		std::scoped_lock _(this->m_entriesLock);
		for (uint32_t row = 0; row < m_logEntries.size(); row++)
		{
			const Logger::LogEntry& log = this->m_logEntries[row];
			// Switch row
			ImGui::TableNextRow();
			ImGui::TableSetColumnIndex(0);

			ImGui::PushID(row);
			{
				ImVec4 styleColour;
				switch (log.Level)
				{
				default:
				case Logger::LogLevel::None:
				case Logger::LogLevel::Info:
					styleColour = ImVec4(0.76f, 0.77f, 0.8f, 1.0f);
					break;
				case Logger::LogLevel::Warning:
					styleColour = ImVec4(0.7f, 0.75f, 0.0f, 1.0f);
					break;
				case Logger::LogLevel::Error:
					styleColour = ImVec4(0.7f, 0.3f, 0.3f, 1.0f);
					break;
				}

				ImGui::PushStyleColor(ImGuiCol_Text, styleColour);
				ImGui::TextUnformatted(log.Msg.c_str());
				ImGui::PopStyleColor(1);
			}
			ImGui::PopID();
		}
		ImGui::EndTable();
	}

	ImGui::End();
}

void PhxEditor::ConsoleLogWidget::LogCallback(PhxEngine::Logger::LogEntry const& e)
{
	std::scoped_lock _(this->m_entriesLock);

	if (this->m_logEntries.size() >= MaxLogMsgs)
	{
		this->m_logEntries.pop_front();
	}

	this->m_logEntries.push_back(e);
}