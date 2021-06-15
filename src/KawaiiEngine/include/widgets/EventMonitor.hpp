#pragma once

#include "EventProvider.hpp"
#include "json/SerializeEvent.hpp"

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


        ImGui::PlotLines(
            "", times_plot.data(), times_plot.size(), 0, "Average nanoseconds", 0.0f, 10'000'000.0f, ImVec2(0, 80.0f));

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
                ImGui::Text("Last Time Elapsed :\n%s", as_json.dump(4).data());
            }
        }
        ImGui::Separator();

        ImGui::Text("Number of Event pending: %ld", provider.getEventsPending().size());
        ImGui::Text("Number of Event processed: %ld", provider.getEventsProcessed().size());
        if (ImGui::Button("clear")) { provider.clear(); }


        /*
                ImGui::Text("Event mode : %s", magic_enum::enum_name(m_event_mnager.getMode()).data());
                ImGui::SameLine();

                {
                    const auto filepath = "assets/image/hud/button/record.png";
                    const auto uid_texture = entt::hashed_string{fmt::format("core::image/{}",
           filepath).data()}; if (const auto image = m_cache_texture.load<LoaderImage>(uid_texture, filepath);
           image) { if (ImGui::ImageButton(reinterpret_cast<ImTextureID>(image->id), ImVec2(20, 20))) {
                            provider.setMode(EventManager::Mode::RECORD);
                        }
                    }
                }
                ImGui::SameLine();

        {
            const auto filepath = "assets/image/hud/button/play.png";
            const auto uid_texture = entt::hashed_string{fmt::format("core::image/{}", filepath).data()};
            if (const auto image = m_cache_texture.load<LoaderImage>(uid_texture, filepath); image) {
                if (ImGui::ImageButton(reinterpret_cast<ImTextureID>(image->id), ImVec2(20, 20))) {
                    m_event_manager.setMode(EventManager::Mode::PLAYBACK);
                }
            }
        }
*/

        ImGui::Separator();
        {
            const auto last_not_time_elapsed = provider.getLastEventWhere(
                [](auto &i) { return !std::holds_alternative<event::TimeElapsed>(i); });

            if (last_not_time_elapsed.has_value()) {
                nlohmann::json as_json;
                to_json(as_json, *last_not_time_elapsed.value());
                ImGui::Text("Last event :\n%s", as_json.dump(4).data());
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
        for (const auto &i : times) { times_plot.push_back(i.elapsed.count()); }
    }
};

} // namespace kawe
