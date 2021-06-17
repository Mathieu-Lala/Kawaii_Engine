#pragma once

#include <nlohmann/json.hpp>
#include <magic_enum.hpp>
#include "helpers/overloaded.hpp"

#include "json/std_serializer.hpp"

#include "Event.hpp"

namespace nlohmann {

using namespace kawe::event;

template<>
struct adl_serializer<Joystick::Axis> {
    static void to_json(nlohmann::json &j, const Joystick::Axis &axis)
    {
        j = magic_enum::enum_name(axis).data();
    }

    static void from_json(const nlohmann::json &j, Joystick::Axis &axis)
    {
        std::string value = j.at("axis");
        axis = magic_enum::enum_cast<Joystick::Axis>(value).value_or(Joystick::AXES_MAX);
    }
};

template<>
struct adl_serializer<Joystick::Buttons> {
    static void to_json(nlohmann::json &j, const Joystick::Buttons &axis)
    {
        j = magic_enum::enum_name(axis).data();
    }

    static void from_json(const nlohmann::json &j, Joystick::Buttons &button)
    {
        std::string value = j.at("button");
        button = magic_enum::enum_cast<Joystick::Buttons>(value).value_or(Joystick::BUTTONS_MAX);
    }
};

template<>
struct adl_serializer<Key::Code> {
    static void to_json(nlohmann::json &j, const Key::Code &keycode)
    {
        j = magic_enum::enum_name(keycode).data();
    }

    static void from_json(const nlohmann::json &j, Key::Code &keycode)
    {
        std::string value = j.at("keycode");
        keycode = magic_enum::enum_cast<Key::Code>(value).value_or(Key::Code::KEY_UNKNOWN);
    }
};

} // namespace nlohmann

namespace kawe {

namespace event {

template<typename EventType, typename... Param>
void serialize(nlohmann::json &j, const Param &... param)
{
    auto make_inner = [&] {
        nlohmann::json innerObj;
        [[maybe_unused]] std::size_t index = 0;

        (innerObj.emplace(EventType::elements[index++], param), ...);

        return innerObj;
    };

    nlohmann::json outerObj;
    outerObj.emplace(EventType::name, make_inner());
    j = outerObj;
}

template<typename EventType, typename... Param>
void deserialize(const nlohmann::json &j, Param &... param)
{
    // annoying conversion to string necessary for key lookup with .at?
    const auto &top = j.at(std::string{EventType::name});

    if (top.size() != sizeof...(Param)) { std::logic_error("Deserialization size mismatch"); }

    std::size_t cur_elem = 0;
    (top.at(std::string{EventType::elements[cur_elem++]}).get_to(param), ...);
}

template<typename EventType>
void to_json(nlohmann::json &j, [[maybe_unused]] const EventType &event) requires(EventType::elements.empty())
{
    serialize<EventType>(j);
}

template<typename EventType>
void to_json(nlohmann::json &j, const EventType &event) requires(EventType::elements.size() == 1)
{
    const auto &[elem0] = event;
    serialize<EventType>(j, elem0);
}

template<typename EventType>
void to_json(nlohmann::json &j, const EventType &event) requires(EventType::elements.size() == 2)
{
    const auto &[elem0, elem1] = event;
    serialize<EventType>(j, elem0, elem1);
}

template<typename EventType>
void to_json(nlohmann::json &j, const EventType &event) requires(EventType::elements.size() == 3)
{
    const auto &[elem0, elem1, elem2] = event;
    serialize<EventType>(j, elem0, elem1, elem2);
}

template<typename EventType>
void to_json(nlohmann::json &j, const EventType &event) requires(EventType::elements.size() == 4)
{
    const auto &[elem0, elem1, elem2, elem3] = event;
    serialize<EventType>(j, elem0, elem1, elem2, elem3);
}

template<typename EventType>
void to_json(nlohmann::json &j, const EventType &event) requires(EventType::elements.size() == 5)
{
    const auto &[elem0, elem1, elem2, elem3, elem4] = event;
    serialize<EventType>(j, elem0, elem1, elem2, elem3, elem4);
}

template<typename EventType>
void to_json(nlohmann::json &j, const EventType &event) requires(EventType::elements.size() == 6)
{
    const auto &[elem0, elem1, elem2, elem3, elem4, elem5] = event;
    serialize<EventType>(j, elem0, elem1, elem2, elem3, elem4, elem5);
}

template<typename EventType>
void from_json(const nlohmann::json &j, [[maybe_unused]] const EventType &) requires(EventType::elements.empty())
{
    deserialize<EventType>(j);
}

template<typename EventType>
void from_json(const nlohmann::json &j, EventType &event) requires(EventType::elements.size() == 1)
{
    auto &[elem0] = event;
    deserialize<EventType>(j, elem0);
}

template<typename EventType>
void from_json(const nlohmann::json &j, EventType &event) requires(EventType::elements.size() == 2)
{
    auto &[elem0, elem1] = event;
    deserialize<EventType>(j, elem0, elem1);
}

template<typename EventType>
void from_json(const nlohmann::json &j, EventType &event) requires(EventType::elements.size() == 3)
{
    auto &[elem0, elem1, elem2] = event;
    deserialize<EventType>(j, elem0, elem1, elem2);
}

template<typename EventType>
void from_json(const nlohmann::json &j, EventType &event) requires(EventType::elements.size() == 4)
{
    auto &[elem0, elem1, elem2, elem3] = event;
    deserialize<EventType>(j, elem0, elem1, elem2, elem3);
}

template<typename EventType>
void from_json(const nlohmann::json &j, EventType &event) requires(EventType::elements.size() == 5)
{
    auto &[elem0, elem1, elem2, elem3, elem4] = event;
    deserialize<EventType>(j, elem0, elem1, elem2, elem3, elem4);
}

template<typename EventType>
void from_json(const nlohmann::json &j, EventType &event) requires(EventType::elements.size() == 6)
{
    auto &[elem0, elem1, elem2, elem3, elem4, elem5] = event;
    deserialize<EventType>(j, elem0, elem1, elem2, elem3, elem4, elem5);
}

} // namespace event


template<typename... T>
void choose_variant(const nlohmann::json &j, std::variant<std::monostate, T...> &variant)
{
    bool matched = false;

    auto try_variant = [&]<typename Variant>() {
        if (!matched) {
            try {
                Variant obj{};
                from_json(j, obj);
                variant = obj;
                matched = true;
            } catch (const std::exception &) {
            }
        }
    };

    (try_variant.template operator()<T>(), ...);
}

namespace event {

inline void from_json(const nlohmann::json &j, Event &event) { choose_variant(j, event); }

inline void to_json(nlohmann::json &j, const Event &event)
{
    std::visit(
        overloaded{[]([[maybe_unused]] const std::monostate &) {}, [&j](const auto &e) { to_json(j, e); }}, event);
}

} // namespace event

} // namespace kawe
