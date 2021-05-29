#pragma once

#include <cstdint>
#include <string_view>

#include <glm/gtc/type_ptr.hpp>
#include <spdlog/spdlog.h>

#include "helpers/macro.hpp"

namespace kawe {

namespace {

template<std::size_t type>
struct shader_ {
    explicit shader_(const char *source) : ID{::glCreateShader(type)}
    {
        if (ID == 0) {
            SHOW_ERROR(::glGetError());
            return;
        }

        CALL_OPEN_GL(::glShaderSource(ID, 1, &source, nullptr));
        CALL_OPEN_GL(::glCompileShader(ID));

        check(ID);
    }

    ~shader_() { CALL_OPEN_GL(::glDeleteShader(ID)); }

    static auto check(std::uint32_t id) -> void
    {
        int success;
        std::array<char, 512> log;
        std::fill(log.begin(), log.end(), '\0');
        CALL_OPEN_GL(::glGetShaderiv(id, GL_COMPILE_STATUS, &success));
        if (!success) {
            CALL_OPEN_GL(::glGetShaderInfoLog(id, log.size(), nullptr, log.data()));
            spdlog::error("Engine::Core [Shader] link failed: {}", log.data());
        }
    }

    std::uint32_t ID;
};

} // namespace

class Shader {
public:
    Shader() = default;

    Shader(const std::string_view vCode, const std::string_view fCode) : ID{::glCreateProgram()}
    {
        shader_<GL_VERTEX_SHADER> vertex{vCode.data()};
        shader_<GL_FRAGMENT_SHADER> fragment{fCode.data()};

        CALL_OPEN_GL(::glAttachShader(ID, vertex.ID));
        CALL_OPEN_GL(::glAttachShader(ID, fragment.ID));
        CALL_OPEN_GL(::glLinkProgram(ID));

        check(ID);
    }

    static auto check(std::uint32_t id) -> void
    {
        int success;
        std::array<char, 512> log;
        std::fill(log.begin(), log.end(), '\0');
        CALL_OPEN_GL(::glGetShaderiv(id, GL_LINK_STATUS, &success));
        if (!success) {
            CALL_OPEN_GL(::glGetShaderInfoLog(id, log.size(), nullptr, log.data()));
            spdlog::error("Engine::Core [Shader] link failed: {}", log.data());
        }
    }

    ~Shader() { CALL_OPEN_GL(::glDeleteProgram(ID)); }

    auto use() const noexcept -> void { CALL_OPEN_GL(::glUseProgram(ID)); }

    template<typename T>
    auto setUniform(const std::string_view, T) -> void;

private:
    std::uint32_t ID;
};

} // namespace kawe
