#pragma once

#include "graphics/deps.hpp"

namespace kawe {

struct Console {
    std::vector<std::string> Items;
    std::vector<std::string> Commands;
    std::vector<std::string> History;

    int HistoryPos = -1; // -1: new line, 0..History.Size-1 browsing history.
    ImGuiTextFilter Filter;

    bool AutoScroll = true;
    bool ScrollToBottom = false;

    Console()
    {
        Commands.push_back("HELP");
        Commands.push_back("HISTORY");
        Commands.push_back("CLEAR");
        Commands.push_back("CLASSIFY");
        AddLog("Welcome to Dear ImGui!");
    }

    static int Strnicmp(const char *s1, const char *s2, int n)
    {
        int d = 0;
        while (n > 0 && (d = toupper(*s2) - toupper(*s1)) == 0 && *s1) {
            s1++;
            s2++;
            n--;
        }
        return d;
    }
    static char *Strdup(const char *s)
    {
        IM_ASSERT(s);
        size_t len = strlen(s) + 1;
        void *buf = malloc(len);
        IM_ASSERT(buf);
        return (char *) memcpy(buf, (const void *) s, len);
    }


    void ClearLog() { Items.clear(); }

    void AddLog(const char *fmt, ...) IM_FMTARGS(2)
    {
        // FIXME-OPT
        char buf[1024];
        va_list args;
        va_start(args, fmt);
        vsnprintf(buf, IM_ARRAYSIZE(buf), fmt, args);
        buf[IM_ARRAYSIZE(buf) - 1] = 0;
        va_end(args);
        Items.push_back(Strdup(buf));
    }

    void draw()
    {
        // ImGui::SetNextWindowSize(ImVec2(520, 600), ImGuiCond_FirstUseEver);
        if (!ImGui::Begin("KAWE: Console")) { return ImGui::End(); }

        // if (ImGui::BeginPopupContextItem()) {
        //     if (ImGui::MenuItem("Close Console")) *p_open = false;
        //     ImGui::EndPopup();
        // }

        ImGui::TextWrapped(
            "This example implements a console with basic coloring, completion (TAB key) and history (Up/Down keys). A more elaborate "
            "implementation may want to store entries along with extra data such as timestamp, emitter, etc.");
        ImGui::TextWrapped("Enter 'HELP' for help.");

        if (ImGui::SmallButton("Add Debug Text")) {
            AddLog("%d some text", Items.size());
            AddLog("some more text");
            AddLog("display very important message here!");
        }
        ImGui::SameLine();
        if (ImGui::SmallButton("Add Debug Error")) { AddLog("[error] something went wrong"); }
        ImGui::SameLine();
        if (ImGui::SmallButton("Clear")) { ClearLog(); }
        ImGui::SameLine();
        bool copy_to_clipboard = ImGui::SmallButton("Copy");

        ImGui::Separator();

        if (ImGui::BeginPopup("Options")) {
            ImGui::Checkbox("Auto-scroll", &AutoScroll);
            ImGui::EndPopup();
        }

        if (ImGui::Button("Options")) ImGui::OpenPopup("Options");
        ImGui::SameLine();
        Filter.Draw("Filter (\"incl,-excl\") (\"error\")", 180);
        ImGui::Separator();

        const float footer_height_to_reserve =
            ImGui::GetStyle().ItemSpacing.y + ImGui::GetFrameHeightWithSpacing();
        ImGui::BeginChild(
            "ScrollingRegion", ImVec2(0, -footer_height_to_reserve), false, ImGuiWindowFlags_HorizontalScrollbar);
        if (ImGui::BeginPopupContextWindow()) {
            if (ImGui::Selectable("Clear")) ClearLog();
            ImGui::EndPopup();
        }

        ImGui::PushStyleVar(ImGuiStyleVar_ItemSpacing, ImVec2(4, 1));
        if (copy_to_clipboard) ImGui::LogToClipboard();
        for (std::size_t i = 0; i < Items.size(); i++) {
            const auto item = Items[i];
            if (!Filter.PassFilter(item.data())) continue;

            ImVec4 color;
            bool has_color = false;
            if (item.find("[error]") != std::string::npos) {
                color = ImVec4(1.0f, 0.4f, 0.4f, 1.0f);
                has_color = true;
            } else if (item.substr(0, 2) == "# ") {
                color = ImVec4(1.0f, 0.8f, 0.6f, 1.0f);
                has_color = true;
            }
            if (has_color) ImGui::PushStyleColor(ImGuiCol_Text, color);
            ImGui::TextUnformatted(item.data());
            if (has_color) ImGui::PopStyleColor();
        }
        if (copy_to_clipboard) ImGui::LogFinish();

        if (ScrollToBottom || (AutoScroll && ImGui::GetScrollY() >= ImGui::GetScrollMaxY()))
            ImGui::SetScrollHereY(1.0f);
        ScrollToBottom = false;

        ImGui::PopStyleVar();
        ImGui::EndChild();
        ImGui::Separator();

        bool reclaim_focus = false;
        std::array<char, 256> InputBuf;
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

    void ExecCommand(const std::string command_line)
    {
        AddLog("# %s\n", command_line.data());

        HistoryPos = -1;
        for (int i = History.size() - 1; i >= 0; i--)
            if (History[i] == command_line) {
                History.erase(History.begin() + i);
                break;
            }
        History.push_back(command_line);

        if (command_line == "CLEAR") {
            ClearLog();
        } else if (command_line == "HELP") {
            AddLog("Commands:");
            for (int i = 0; i < Commands.size(); i++) AddLog("- %s", Commands[i]);
        } else if (command_line == "HISTORY") {
            int first = History.size() - 10;
            for (int i = first > 0 ? first : 0; i < History.size(); i++) AddLog("%3d: %s\n", i, History[i]);
        } else {
            AddLog("Unknown command: '%s'\n", command_line);
        }

        ScrollToBottom = true;
    }

    int TextEditCallback(ImGuiInputTextCallbackData *data)
    {
        switch (data->EventFlag) {
        case ImGuiInputTextFlags_CallbackCompletion: {
            const char *word_end = data->Buf + data->CursorPos;
            const char *word_start = word_end;
            while (word_start > data->Buf) {
                const char c = word_start[-1];
                if (c == ' ' || c == '\t' || c == ',' || c == ';') break;
                word_start--;
            }

            ImVector<const char *> candidates;
            for (int i = 0; i < Commands.size(); i++)
                if (Strnicmp(Commands[i].data(), word_start, (int) (word_end - word_start)) == 0)
                    candidates.push_back(Commands[i].data());

            if (candidates.size() == 0) {
                AddLog("No match for \"%.*s\"!\n", (int) (word_end - word_start), word_start);
            } else if (candidates.size() == 1) {
                data->DeleteChars((int) (word_start - data->Buf), (int) (word_end - word_start));
                data->InsertChars(data->CursorPos, candidates[0]);
                data->InsertChars(data->CursorPos, " ");
            } else {
                int match_len = (int) (word_end - word_start);
                for (;;) {
                    int c = 0;
                    bool all_candidates_matches = true;
                    for (int i = 0; i < candidates.Size && all_candidates_matches; i++)
                        if (i == 0)
                            c = toupper(candidates[i][match_len]);
                        else if (c == 0 || c != toupper(candidates[i][match_len]))
                            all_candidates_matches = false;
                    if (!all_candidates_matches) break;
                    match_len++;
                }

                if (match_len > 0) {
                    data->DeleteChars((int) (word_start - data->Buf), (int) (word_end - word_start));
                    data->InsertChars(data->CursorPos, candidates[0], candidates[0] + match_len);
                }

                AddLog("Possible matches:\n");
                for (int i = 0; i < candidates.Size; i++) AddLog("- %s\n", candidates[i]);
            }

            break;
        }
        case ImGuiInputTextFlags_CallbackHistory: {
            const int prev_history_pos = HistoryPos;
            if (data->EventKey == ImGuiKey_UpArrow) {
                if (HistoryPos == -1)
                    HistoryPos = History.size() - 1;
                else if (HistoryPos > 0)
                    HistoryPos--;
            } else if (data->EventKey == ImGuiKey_DownArrow) {
                if (HistoryPos != -1)
                    if (++HistoryPos >= History.size()) HistoryPos = -1;
            }

            if (prev_history_pos != HistoryPos) {
                const auto history_str = (HistoryPos >= 0) ? History[HistoryPos] : "";
                data->DeleteChars(0, data->BufTextLen);
                data->InsertChars(0, history_str.data());
            }
        }
        }
        return 0;
    }
};

} // namespace kawe
