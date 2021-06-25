#pragma once

#include "graphics/deps.hpp"

namespace kawe {

struct Console {
    Console()
    {
        commands.push_back("HELP");
        commands.push_back("HISTORY");
        commands.push_back("CLEAR");
        commands.push_back("CLASSIFY");
        log("Welcome to Kawe Debug Console!");
    }

    auto clear() -> void { messages.clear(); }

    template<typename... Args>
    auto log(const std::string_view fmt, Args &&... args) -> void
    {
        messages.push_back(fmt::format(fmt, args...));
    }

    auto draw() -> void
    {
        if (!ImGui::Begin("KAWE: Console")) { return ImGui::End(); }

        ImGui::TextWrapped("Completion (TAB key) and history (Up/Down keys).");
        ImGui::TextWrapped("Enter 'HELP' for help.");

        draw_select_log_level();

        if (ImGui::SmallButton("Add Debug Text")) {
            log("{} some text", messages.size());
            log("some more text");
            log("display very important message here!");
        }
        ImGui::SameLine();
        if (ImGui::SmallButton("Add Debug Error")) { log("[error] something went wrong"); }
        ImGui::SameLine();
        if (ImGui::SmallButton("Clear")) { clear(); }
        ImGui::SameLine();
        bool copy_to_clipboard = ImGui::SmallButton("Copy");

        ImGui::Separator();

        if (ImGui::BeginPopup("Options")) {
            ImGui::Checkbox("Auto-scroll", &autoscroll);
            ImGui::EndPopup();
        }

        if (ImGui::Button("Options")) ImGui::OpenPopup("Options");
        ImGui::SameLine();
        filter.Draw("Filter (\"incl,-excl\") (\"error\")", 180);
        ImGui::Separator();

        const auto footer_height_to_reserve =
            ImGui::GetStyle().ItemSpacing.y + ImGui::GetFrameHeightWithSpacing();
        ImGui::BeginChild(
            "ScrollingRegion", ImVec2(0, -footer_height_to_reserve), false, ImGuiWindowFlags_HorizontalScrollbar);
        if (ImGui::BeginPopupContextWindow()) {
            if (ImGui::Selectable("Clear")) clear();
            ImGui::EndPopup();
        }

        const auto get_color = [](auto &str) -> std::optional<ImVec4> {
            if (str.find("[error]") != std::string::npos) {
                return ImVec4(1.0f, 0.4f, 0.4f, 1.0f);
            } else if (str.substr(0, 2) == "# ") {
                return ImVec4(1.0f, 0.8f, 0.6f, 1.0f);
            } else {
                return {};
            }
        };

        ImGui::PushStyleVar(ImGuiStyleVar_ItemSpacing, ImVec2(4, 1));
        if (copy_to_clipboard) ImGui::LogToClipboard();
        for (const auto &i : messages) {
            if (!filter.PassFilter(i.data())) continue;

            const auto color = get_color(i);
            if (color.has_value()) ImGui::PushStyleColor(ImGuiCol_Text, color.value());
            ImGui::TextUnformatted(i.data());
            if (color.has_value()) ImGui::PopStyleColor();
        }
        if (copy_to_clipboard) ImGui::LogFinish();

        if (scroll_to_bottom || (autoscroll && ImGui::GetScrollY() >= ImGui::GetScrollMaxY()))
            ImGui::SetScrollHereY(1.0f);
        scroll_to_bottom = false;

        ImGui::PopStyleVar();
        ImGui::EndChild();
        ImGui::Separator();

        bool reclaim_focus = false;
        std::array<char, 256> InputBuf;
        std::fill(InputBuf.begin(), InputBuf.end(), 0);
        if (ImGui::InputText(
                "Input",
                InputBuf.data(),
                InputBuf.size(),
                ImGuiInputTextFlags_EnterReturnsTrue | ImGuiInputTextFlags_CallbackCompletion
                    | ImGuiInputTextFlags_CallbackHistory,
                [](ImGuiInputTextCallbackData *data) {
                    return reinterpret_cast<Console *>(data->UserData)->TextEditCallback(data);
                },
                reinterpret_cast<void *>(this))) {
            std::string str = InputBuf.data();
            ExecCommand(str);
            reclaim_focus = true;
        }

        ImGui::SetItemDefaultFocus();
        if (reclaim_focus) ImGui::SetKeyboardFocusHere(-1);

        ImGui::End();
    }

    auto ExecCommand(const std::string command_line) -> void
    {
        log("# {}\n", command_line.data());

        history_position = -1;
        std::erase(history_message, command_line);
        history_message.push_back(command_line);

        if (command_line == "CLEAR") {
            clear();
        } else if (command_line == "HELP") {
            log("Commands:");
            for (const auto &i : commands) log("- {}", i);
        } else if (command_line == "HISTORY") {
            for (auto i = static_cast<std::size_t>(
                     std::max(static_cast<std::int32_t>(history_message.size()) - 10, 0));
                 i < history_message.size();
                 i++) {
                log("{}: {}\n", i, history_message[i]);
            }
        } else {
            log("Unknown command: '{}'\n", command_line);
        }

        scroll_to_bottom = true;
    }

private:
    std::vector<std::string> messages;
    std::vector<std::string> commands;
    std::vector<std::string> history_message;

    int history_position = -1; // -1: new line, 0..History.Size-1 browsing history.
    ImGuiTextFilter filter;

    bool autoscroll = true;
    bool scroll_to_bottom = false;

    auto TextEditCallback(ImGuiInputTextCallbackData *data) -> int
    {
        switch (data->EventFlag) {
        case ImGuiInputTextFlags_CallbackCompletion: {
            const auto word_end = data->Buf + data->CursorPos;
            auto word_start = word_end;
            while (word_start > data->Buf) {
                const auto c = word_start[-1];
                if (c == ' ' || c == '\t' || c == ',' || c == ';') break;
                word_start--;
            }

            std::vector<std::string> candidates;
            for (const auto &i : commands)
                if (i.find(word_start) != std::string::npos) { candidates.push_back(i); }

            if (candidates.empty()) {
                log("No match for \"{.*s}\"!\n", static_cast<int>(word_end - word_start), word_start);
            } else if (candidates.size() == 1) {
                data->DeleteChars(
                    static_cast<int>(word_start - data->Buf), static_cast<int>(word_end - word_start));
                data->InsertChars(data->CursorPos, candidates[0].data());
            } else {
                auto match_len = static_cast<std::size_t>(word_end - word_start);
                for (;;) {
                    int c = 0;
                    bool all_candidates_matches = true;
                    for (auto i = 0ul; i < candidates.size() && all_candidates_matches; i++)
                        if (i == 0ul)
                            c = std::toupper(candidates[i][match_len]);
                        else if (c == 0 || c != std::toupper(candidates[i][match_len]))
                            all_candidates_matches = false;
                    if (!all_candidates_matches) break;
                    match_len++;
                }

                if (match_len > 0) {
                    data->DeleteChars(
                        static_cast<int>(word_start - data->Buf), static_cast<int>(word_end - word_start));
                    data->InsertChars(data->CursorPos, candidates[0].data(), candidates[0].data() + match_len);
                }

                log("Possible matches:\n");
                for (const auto &i : candidates) { log("- {}\n", i); }
            }
        } break;
        case ImGuiInputTextFlags_CallbackHistory: {
            const int prev_history_pos = history_position;
            if (data->EventKey == ImGuiKey_UpArrow) {
                if (history_position == -1) {
                    history_position = static_cast<int>(history_message.size() - 1ul);
                } else if (history_position > 0) {
                    history_position--;
                }
            } else if (data->EventKey == ImGuiKey_DownArrow) {
                if (history_position != -1) {
                    history_position = (history_position + 2) % static_cast<int>(history_message.size() + 1) - 1;
                }
            }

            if (prev_history_pos != history_position) {
                data->DeleteChars(0, data->BufTextLen);
                data->InsertChars(
                    0,
                    ((history_position >= 0) ? history_message[static_cast<std::size_t>(history_position)] : "")
                        .data());
            }
        } break;
        }
        return 0;
    }

    auto draw_select_log_level() -> void
    {
        constexpr auto enum_name = magic_enum::enum_type_name<spdlog::level::level_enum>();
        auto log_level = spdlog::get_level();
        if (ImGui::BeginCombo(
                "##combo_vao_mode",
                fmt::format("{} = {}", enum_name.data(), magic_enum::enum_name(log_level)).data())) {
            for (const auto &[value, name] : magic_enum::enum_entries<spdlog::level::level_enum>()) {
                if (value == spdlog::level::n_levels) continue;
                const auto is_selected = log_level == value;
                if (ImGui::Selectable(name.data(), is_selected)) {
                    log_level = value;
                    spdlog::set_level(log_level);
                }
                if (is_selected) { ImGui::SetItemDefaultFocus(); }
            }
            ImGui::EndCombo();
        }
    }
};

} // namespace kawe
