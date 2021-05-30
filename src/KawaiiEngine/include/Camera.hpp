#pragma once

#include <glm/glm.hpp>

namespace kawe {

struct Camera {
    Camera(const Window &window, glm::dvec3 pos) :
        m_window{window}, position{std::move(pos)}, up{glm::normalize(glm::dvec3{0.0f, 1.0f, 0.0})}
    {
        getFrustrumInfo();
    }

    enum class ProjectionType {
        PRESPECTIVE,
        // ORTHOGONAL
    };

    constexpr auto getProjectionType() const noexcept { return projection_type; }

    auto setProjectionType(ProjectionType value) noexcept -> void { projection_type = value; }

    auto getProjection() -> glm::dmat4
    {
        //        const auto projection_provider = std::to_array<std::function<glm::mat4(const Window &)>>(
        //            {[](const Window &w) {
        //                 return glm::perspective(glm::radians(45.0f), w.getAspectRatio<float>(), 0.1f, 100.0f);
        //             },
        //             [](const Window &) { return glm::ortho(-10.0f, 10.0f, -10.0f, 10.0f, -5.0f, 5.0f); }});
        //        return projection_provider.at(static_cast<std::size_t>(projection_type))(window);

        return glm::perspective(glm::radians(m_fov), m_window.getAspectRatio<double>(), m_near, m_far);
    };

    constexpr auto getPosition() const noexcept -> const glm::dvec3 & { return position; }

    auto getPosition() noexcept -> glm::dvec3 & { return position; }

    auto setPosition(glm::dvec3 pos) noexcept
    {
        position = std::move(pos);
        setChangedFlag<Matrix::VIEW>(true);
    }

    constexpr auto getTargetCenter() const noexcept -> const glm::dvec3 & { return target_center; }

    constexpr auto getUp() const noexcept -> const glm::dvec3 & { return up; }


    [[nodiscard]] constexpr auto getFOV() const noexcept { return m_fov; }

    auto setFOV(double value) noexcept -> void
    {
        m_fov = value;
        setChangedFlag<Matrix::PROJECTION>(true);
    }


    [[nodiscard]] constexpr auto getNear() const noexcept { return m_near; }

    auto setNear(double value) noexcept -> void
    {
        m_near = value;
        setChangedFlag<Matrix::PROJECTION>(true);
    }


    [[nodiscard]] constexpr auto getFar() const noexcept { return m_far; }

    auto setFar(double value) noexcept -> void
    {
        m_far = value;
        setChangedFlag<Matrix::PROJECTION>(true);
    }


    template<typename T>
    auto rotate(T fractionChangeX, T fractionChangeY)
    {
        constexpr T DEFAULT_ROTATE_SPEED = 2.0 / 100.0;

        const auto setFromAxisAngle = [](const glm::vec<3, T> &axis, auto angle) -> glm::dquat {
            const auto cosAng = std::cos(angle / static_cast<T>(2.0));
            const auto sinAng = std::sin(angle / static_cast<T>(2.0));
            const auto norm = std::sqrt(axis.x * axis.x + axis.y * axis.y + axis.z * axis.z);

            return {
                static_cast<T>(sinAng) * axis.x / norm,
                static_cast<T>(sinAng) * axis.y / norm,
                static_cast<T>(sinAng) * axis.z / norm,
                static_cast<T>(cosAng)};
        };

        const auto horizRotAngle = DEFAULT_ROTATE_SPEED * fractionChangeY;
        const auto vertRotAngle = -DEFAULT_ROTATE_SPEED * fractionChangeX;

        const auto horizRot = setFromAxisAngle(m_imagePlaneHorizDir, horizRotAngle);

        const auto vertRot = setFromAxisAngle(m_imagePlaneVertDir, vertRotAngle);

        const auto totalRot = horizRot * vertRot;

        const auto viewVec = totalRot * (position - target_center);

        position = target_center + viewVec;

        getFrustrumInfo();
    }

    template<typename T>
    auto zoom(T changeVert)
    {
        constexpr T DEFAULT_ZOOM_FRACTION = 2.5 / 100.0;

        const auto scaleFactor = std::pow(2.0, -changeVert * DEFAULT_ZOOM_FRACTION);
        position = target_center + (position - target_center) * scaleFactor;

        getFrustrumInfo();
    }

    template<typename T>
    auto translate(T changeHoriz, T changeVert, bool parallelToViewPlane)
    {
        constexpr T DEFAULT_TRANSLATE_SPEED = 0.5 / 100.0;

        const auto translateVec = parallelToViewPlane ? (m_imagePlaneHorizDir * (m_display.x * changeHoriz))
                                                            + (m_imagePlaneVertDir * (changeVert * m_display.y))
                                                      : (target_center - position) * changeVert;

        position += translateVec * DEFAULT_TRANSLATE_SPEED;
        target_center += translateVec * DEFAULT_TRANSLATE_SPEED;
        setChangedFlag<Matrix::VIEW>(true);
    }

    auto handleMouseInput(
        MouseButton::Button button,
        const glm::dvec2 &mouse_pos,
        const glm::dvec2 &mouse_pos_when_pressed,
        const std::chrono::nanoseconds dt)
    {
        const auto ms = static_cast<double>(std::chrono::duration_cast<std::chrono::milliseconds>(dt).count());
        const auto size = m_window.getSize<double>();

        switch (button) {
        case kawe::MouseButton::Button::BUTTON_LEFT: {
            const auto fractionChangeX = (mouse_pos.x - mouse_pos_when_pressed.x) / size.x;
            const auto fractionChangeY = (mouse_pos_when_pressed.y - mouse_pos.y) / size.y;
            rotate(fractionChangeX * ms, fractionChangeY * ms);
        } break;
        case kawe::MouseButton::Button::BUTTON_MIDDLE: {
            const auto fractionChangeY = (mouse_pos_when_pressed.y - mouse_pos.y) / size.y;
            zoom(fractionChangeY * ms);
        } break;
        case kawe::MouseButton::Button::BUTTON_RIGHT: {
            const auto fractionChangeX = (mouse_pos.x - mouse_pos_when_pressed.x) / size.x;
            const auto fractionChangeY = (mouse_pos_when_pressed.y - mouse_pos.y) / size.y;
            translate(-fractionChangeX * ms, -fractionChangeY * ms, true);

        } break;
        default: break;
        }
    }

    enum class Matrix { VIEW, PROJECTION };

    template<Matrix M>
    auto setChangedFlag(bool value) noexcept -> void
    {
        if constexpr (M == Matrix::VIEW) {
            m_view_changed = value;
        } else {
            m_proj_changed = value;
        }
    }

    template<Matrix M>
    [[nodiscard]] constexpr auto hasChanged() const noexcept
    {
        if constexpr (M == Matrix::VIEW) {
            return m_view_changed;
        } else {
            return m_proj_changed;
        }
    }

private:
    const Window &m_window;

    glm::dvec3 position;
    glm::dvec3 target_center{0, 0, 0};
    glm::dvec3 up;

    glm::dvec3 m_imagePlaneHorizDir;
    glm::dvec3 m_imagePlaneVertDir;
    glm::dvec2 m_display;

    ProjectionType projection_type{ProjectionType::PRESPECTIVE};

    double m_fov{45.0};
    double m_near{0.1};
    double m_far{100.0};

    bool m_view_changed{false};
    bool m_proj_changed{true};

    auto getFrustrumInfo() -> void
    {
        const auto makeOrthogonalTo = [](const glm::dvec3 &vec1, const glm::dvec3 &vec2) -> glm::dvec3 {
            if (const auto length = glm::length(vec2); length != 0) {
                const auto scale = (vec1.x * vec2.x + vec1.y * vec2.y + vec1.z * vec2.z) / (length * length);
                return {
                    vec1.x - scale * vec2.x,
                    vec1.y - scale * vec2.y,
                    vec1.z - scale * vec2.z,
                };
            } else {
                return vec1;
            }
        };

        m_view_changed = true;

        const auto viewDir = glm::normalize(target_center - position);

        m_imagePlaneVertDir = glm::normalize(makeOrthogonalTo(up, viewDir));
        m_imagePlaneHorizDir = glm::normalize(glm::cross(viewDir, m_imagePlaneVertDir));

        m_display.y = 2.0 * glm::length(target_center - position) * std::tan(0.5 * m_fov);
        m_display.x = m_display.y * m_window.getAspectRatio<double>();
    }
};

} // namespace kawe
