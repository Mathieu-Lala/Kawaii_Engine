#pragma once

#include <ImGuiFileDialog.h>

#include "EventProvider.hpp"
#include "json/SerializeEvent.hpp"
#include "helpers/TimeToString.hpp"

namespace kawe {

struct EventMonitor {
    EventProvider &provider;

    EventMonitor(EventProvider &p, entt::registry &world) : provider{p}
    {
        world.ctx<entt::dispatcher *>()->sink<event::TimeElapsed>().connect<&EventMonitor::on_time_elapsed>(*this);
    }

    auto draw() -> void
    {
        if (!ImGui::Begin("KAWE: Events Monitor")) return ImGui::End();

        ImGuiHelper::Text("Provider State: {}", magic_enum::enum_name(provider.getState()).data());

        ImGui::PlotLines(
            "",
            times_plot.data(),
            static_cast<int>(times_plot.size()),
            0,
            "Average nanoseconds",
            0.0f,
            10'000'000.0f,
            ImVec2(0, 80.0f));

        auto v = static_cast<float>(provider.getTimeScaler());
        if (ImGui::SliderFloat("World Time Speed", &v, 0.0f, 10.0f, "%.3f", ImGuiSliderFlags_Logarithmic)) {
            provider.setTimeScaler(static_cast<double>(v));
        }
        {
            const auto last_time_elapsed = provider.getLastEventWhere(
                [](auto &i) { return std::holds_alternative<event::TimeElapsed>(i); });

            if (last_time_elapsed.has_value()) {
                nlohmann::json as_json;
                to_json(as_json, *last_time_elapsed.value());
                ImGuiHelper::Text("Last Time Elapsed :\n{}", as_json.dump(4).data());
            }
        }
        ImGui::Separator();
        ImGuiHelper::Text("Number of Event pending: {}", provider.getEventsPending().size());
        ImGuiFileDialog::Instance()->SetExtentionInfos(".json", ImVec4(1.0f, 1.0f, 0.0f, 0.9f));

        if (ImGui::Button("import"))
            ImGuiFileDialog::Instance()->OpenDialog("kawe::inspect::event::pending", "Choose File", ".json", ".");

        if (ImGuiFileDialog::Instance()->Display("kawe::inspect::event::pending")) {
            if (ImGuiFileDialog::Instance()->IsOk()) {
                const auto path = ImGuiFileDialog::Instance()->GetFilePathName();

                std::ifstream ifs(path);
                if (ifs.is_open()) {
                    const auto j = nlohmann::json::parse(ifs);

                    provider.setPendingEvents(j.get<std::vector<event::Event>>());
                    provider.setState(EventProvider::State::PLAYBACK);
                } else {
                    spdlog::warn("EventMonitor failed to open file: {}", path);
                }
            }

            ImGuiFileDialog::Instance()->Close();
        }

        ImGui::Separator();
        ImGuiHelper::Text("Number of Event processed: {}", provider.getEventsProcessed().size());
        if (ImGui::Button("clear")) { provider.clear(); }
        if (ImGui::Button("export")) {
            // todo : shoudl be in a async call ?
            nlohmann::json serialized(provider.getEventsProcessed());
            std::filesystem::create_directories("logs");
            std::ofstream f{fmt::format("logs/recorded_events_{}.json", time_to_string())};
            f << serialized;
        }
        ImGui::Separator();
        {
            const auto last_not_time_elapsed = provider.getLastEventWhere(
                [](auto &i) { return !std::holds_alternative<event::TimeElapsed>(i); });

            if (last_not_time_elapsed.has_value()) {
                nlohmann::json as_json;
                to_json(as_json, *last_not_time_elapsed.value());
                ImGuiHelper::Text("Last event :\n{}", as_json.dump(4).data());
            }
        }

        ImGui::End();
    }

private:
    std::list<event::TimeElapsed> times;
    std::vector<float> times_plot;

    auto on_time_elapsed(const event::TimeElapsed &e) -> void
    {
        times.push_back(e);
        if (times.size() > 100) { times.pop_front(); }

        times_plot.clear();
        for (const auto &i : times) { times_plot.push_back(static_cast<float>(i.elapsed.count())); }
    }
};

} // namespace kawe
