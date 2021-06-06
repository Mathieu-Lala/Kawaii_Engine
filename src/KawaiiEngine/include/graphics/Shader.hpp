#pragma once

#include <cstdint>
#include <string_view>
#include <unordered_map>

#include <glm/gtc/type_ptr.hpp>
#include <spdlog/spdlog.h>
#include <magic_enum.hpp>

#include "helpers/macro.hpp"

namespace kawe {

enum class ShaderType { vert, frag, UNKNOWN };

const std::unordered_map<ShaderType, std::uint32_t> SHADER_TYPES = {
    {ShaderType::vert, GL_VERTEX_SHADER}, {ShaderType::frag, GL_FRAGMENT_SHADER}, {ShaderType::UNKNOWN, 0}};

struct Shader {
    explicit Shader(const char *source, std::uint32_t type) : shader_id{::glCreateShader(type)}
    {
        if (!shader_id) {
            SHOW_ERROR(::glGetError());
            return;
        }

        CALL_OPEN_GL(::glShaderSource(shader_id, 1, &source, nullptr));
        CALL_OPEN_GL(::glCompileShader(shader_id));

        check_shader(shader_id);
    }

    ~Shader() { CALL_OPEN_GL(::glDeleteShader(shader_id)); }

    static auto check_shader(std::uint32_t id) -> void
    {
        int success;
        CALL_OPEN_GL(::glGetShaderiv(id, GL_COMPILE_STATUS, &success));

        if (success != GL_TRUE) {
            std::array<char, 512> log;
            std::fill(log.begin(), log.end(), '\0');

            CALL_OPEN_GL(::glGetShaderInfoLog(id, log.size(), nullptr, log.data()));
            spdlog::error("Engine::Core [Shader] link failed: {}", log.data());
        }
    }

    std::uint32_t shader_id;
};

class ShaderProgram {
public:
    ShaderProgram(std::string name, const std::vector<std::uint32_t> &shader_ids) :
        m_name{std::move(name)}, program_id{::glCreateProgram()}
    {
        if (!program_id) {
            SHOW_ERROR(::glGetError());
            return;
        }

        for (const auto shader_id : shader_ids) CALL_OPEN_GL(::glAttachShader(program_id, shader_id));

        CALL_OPEN_GL(::glLinkProgram(program_id));
        CALL_OPEN_GL(::glValidateProgram(program_id));

        check_program(program_id);
    }

    static auto check_program(std::uint32_t id) -> void
    {
        int success;
        CALL_OPEN_GL(::glGetProgramiv(id, GL_LINK_STATUS, &success));

        if (success != GL_TRUE) {
            std::array<char, 512> log;
            std::fill(log.begin(), log.end(), '\0');

            CALL_OPEN_GL(::glGetProgramInfoLog(id, log.size(), nullptr, log.data()));
            spdlog::error("Engine::Core [Shader] link failed: {}", log.data());
        }
    }

    ~ShaderProgram() { CALL_OPEN_GL(::glDeleteProgram(program_id)); }

    auto use() const noexcept -> void { CALL_OPEN_GL(::glUseProgram(program_id)); }

    template<typename T>
    auto setUniform(const std::string_view, T) -> void;

    auto getName() const noexcept -> const std::string & { return m_name; }

private:
    ShaderProgram();

    std::string m_name;

    std::uint32_t program_id;
};

} // namespace kawe
