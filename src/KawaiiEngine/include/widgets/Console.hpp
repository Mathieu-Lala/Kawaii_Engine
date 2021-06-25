#pragma once

#include <spdlog/spdlog.h>
#include <spdlog/sinks/stdout_color_sinks.h>
#include <spdlog/pattern_formatter.h>

#include "graphics/deps.hpp"

namespace kawe {

struct Console : public spdlog::sinks::stdout_color_sink_mt, public spdlog::logger {
    Console() :
        spdlog::logger{"console", spdlog::sink_ptr{this, [](auto) {}}},
        formatter{std::make_unique<spdlog::pattern_formatter>("[%H:%M:%S.%F] [%^%l%$] [T=%t] %v")}
    {
        commands.push_back("HELP");
        commands.push_back("HISTORY");
        commands.push_back("CLEAR");
        commands.push_back("CLASSIFY");
        spdlog::initialize_logger(std::shared_ptr<spdlog::logger>{this, [](auto) {}});
        spdlog::get("console")->set_level(spdlog::level::trace);
        clear();
        info("Welcome to Kawe Debug Console!");
    }

    auto clear() -> void
    {
        messages.clear();
        critical("If you have specified a log level too high, you might miss hint");
    }

    std::unique_ptr<spdlog::pattern_formatter> formatter;

    auto log(const spdlog::details::log_msg &msg) -> void override
    {
        // spdlog::sinks::stdout_color_sink_mt::log(msg);
        msg.color_range_start = 0;
        msg.color_range_end = 0;
        spdlog::memory_buf_t formatted;
        formatter->format(msg, formatted);

        messages.emplace_back(
            std::string{formatted.data(), formatted.data() + msg.color_range_start},
            std::string{formatted.data() + msg.color_range_start, formatted.data() + msg.color_range_end},
            std::string{formatted.data() + msg.color_range_end, formatted.data() + formatted.size() - 1},
            msg.level);
    }

    auto draw() -> void
    {
        if (!ImGui::Begin("KAWE: Console")) { return ImGui::End(); }

        ImGui::TextWrapped("Completion (TAB key) and history (Up/Down keys).");
        ImGui::TextWrapped("Enter 'HELP' for help.");

        draw_select_log_level();

        if (ImGui::SmallButton("Add Debug Text")) {
            spdlog::get("console")->trace("trace");
            spdlog::get("console")->debug("debug");
            spdlog::get("console")->info("hello");
            spdlog::get("console")->warn("warn");
            spdlog::get("console")->error("error");
            spdlog::get("console")->critical("critical");
        }
        ImGui::SameLine();
        if (ImGui::SmallButton("Add Debug Error")) { error("[error] something went wrong"); }
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

        // todo : the style of the message can be improved

        ImGui::PushStyleVar(ImGuiStyleVar_ItemSpacing, ImVec2(4, 1));
        if (copy_to_clipboard) ImGui::LogToClipboard();
        for (const auto &i : messages) {
            if (!filter.PassFilter(i.message.data())) continue;

            ImGui::TextUnformatted(i.before.data());
            ImGui::SameLine();

            ImGui::PushStyleColor(ImGuiCol_Text, [](auto &level) {
                return std::array<ImVec4, spdlog::level::level_enum::n_levels>{
                    ImVec4{1.0f, 1.0f, 1.0f, 1.0f},
                    ImVec4{0.0f, 1.0f, 1.0f, 1.0f},
                    ImVec4{0.0f, 1.0f, 0.0f, 1.0f},
                    ImVec4{1.0f, 1.0f, 0.0f, 1.0f},
                    ImVec4{1.0f, 0.0f, 0.0f, 1.0f},
                    ImVec4{0.5f, 0.0f, 0.0f, 1.0f},
                    ImVec4{0.0f, 0.0f, 0.0f, 1.0f},
                }[level];
            }(i.level));
            ImGui::TextUnformatted(i.message.data());
            ImGui::PopStyleColor();
            ImGui::SameLine();

            const auto color = get_color(i.after);
            if (color.has_value()) ImGui::PushStyleColor(ImGuiCol_Text, color.value());
            ImGui::TextUnformatted(i.after.data());
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
        info("# {}\n", command_line);

        history_position = -1;
        std::erase(history_message, command_line);
        history_message.push_back(command_line);

        if (command_line == "CLEAR") {
            clear();
        } else if (command_line == "HELP") {
            info("Commands:");
            for (const auto &i : commands) { info("- {}", i); }
        } else if (command_line == "HISTORY") {
            for (auto i = static_cast<std::size_t>(
                     std::max(static_cast<std::int32_t>(history_message.size()) - 10, 0));
                 i < history_message.size();
                 i++) {
                info("{}: {}\n", i, history_message[i]);
            }
        } else {
            warn("Unknown command: '{}'\n", command_line);
        }

        scroll_to_bottom = true;
    }

private:
    struct Message {
        std::string before;
        std::string message;
        std::string after;
        spdlog::level::level_enum level;
    };
    std::vector<Message> messages;

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
            std::copy_if(commands.begin(), commands.end(), std::back_inserter(candidates), [&word_start](auto &i) {
                return i.find(word_start) != std::string::npos;
            });

            if (candidates.empty()) {
                warn("No match for \"{}\"!\n", std::string_view{word_start, word_end});
            } else if (candidates.size() == 1) {
                data->DeleteChars(
                    static_cast<int>(word_start - data->Buf), static_cast<int>(word_end - word_start));
                data->InsertChars(data->CursorPos, candidates[0].data());
            } else {
                const auto match_len = [&]() {
                    auto len = static_cast<std::size_t>(word_end - word_start);
                    int c = std::toupper(candidates[0ul][len]);
                    while (c != 0 && std::all_of(candidates.begin(), candidates.end(), [&len, &c](auto &i) {
                               return c == std::toupper(i[len]);
                           })) {
                        len++;
                    }

                    return len;
                }();

                if (match_len != 0) {
                    data->DeleteChars(
                        static_cast<int>(word_start - data->Buf), static_cast<int>(word_end - word_start));
                    data->InsertChars(data->CursorPos, candidates[0].data(), candidates[0].data() + match_len);
                }

                info("Possible matches:\n");
                for (const auto &i : candidates) { info("- {}\n", i); }
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
        auto log_level = spdlog::get("console")->sinks().front()->level();
        if (ImGui::BeginCombo(
                "##combo_vao_mode",
                fmt::format("{} = {}", enum_name.data(), magic_enum::enum_name(log_level)).data())) {
            for (const auto &[value, name] : magic_enum::enum_entries<spdlog::level::level_enum>()) {
                if (value == spdlog::level::n_levels) continue;
                const auto is_selected = log_level == value;
                if (ImGui::Selectable(name.data(), is_selected)) {
                    log_level = value;
                    spdlog::get("console")->sinks().front()->set_level(log_level);
                }
                if (is_selected) { ImGui::SetItemDefaultFocus(); }
            }
            ImGui::EndCombo();
        }
    }
};

} // namespace kawe
