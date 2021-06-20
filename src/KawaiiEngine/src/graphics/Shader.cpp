#include <GL/glew.h>

#include "graphics/Shader.hpp"
#include "helpers/macro.hpp"

template<>
auto kawe::ShaderProgram::setUniform(const std::string_view name, bool v) -> void
{
    if (const auto location = ::glGetUniformLocation(program_id, name.data()); location != -1)
        CALL_OPEN_GL(::glUniform1ui(location, v));
    else
        spdlog::warn("'{}' uniform not found.", name);
}

template<>
auto kawe::ShaderProgram::setUniform(const std::string_view name, float v) -> void
{
    if (const auto location = ::glGetUniformLocation(program_id, name.data()); location != -1)
        CALL_OPEN_GL(::glUniform1f(location, v));
    else
        spdlog::warn("'{}' uniform not found.", name);
}

template<>
auto kawe::ShaderProgram::setUniform(const std::string_view name, glm::dvec4 vec) -> void
{
    if (const auto location = ::glGetUniformLocation(program_id, name.data()); location != -1)
        CALL_OPEN_GL(::glUniform4f(
            location,
            static_cast<float>(vec.x),
            static_cast<float>(vec.y),
            static_cast<float>(vec.z),
            static_cast<float>(vec.w)));
    else
        spdlog::warn("'{}' uniform not found.", name);
}

template<>
auto kawe::ShaderProgram::setUniform(const std::string_view name, glm::dvec3 vec) -> void
{
    if (const auto location = ::glGetUniformLocation(program_id, name.data()); location != -1)
        CALL_OPEN_GL(::glUniform3f(
            location, static_cast<float>(vec.x), static_cast<float>(vec.y), static_cast<float>(vec.z)));
    else
        spdlog::warn("'{}' uniform not found.", name);
}

template<>
auto kawe::ShaderProgram::setUniform(const std::string_view name, glm::mat4 mat) -> void
{
    if (const auto location = ::glGetUniformLocation(program_id, name.data()); location != -1)
        CALL_OPEN_GL(::glUniformMatrix4fv(location, 1, GL_FALSE, glm::value_ptr(mat)));
    else
        spdlog::warn("'{}' uniform not found.", name);
}

template<>
auto kawe::ShaderProgram::setUniform(const std::string_view name, glm::dmat4 mat) -> void
{
    setUniform(name, glm::mat4{mat});
}

template<>
auto kawe::ShaderProgram::setUniform(const std::string_view name, unsigned int v) -> void
{
    if (const auto location = ::glGetUniformLocation(program_id, name.data()); location != -1)
        CALL_OPEN_GL(::glUniform1ui(location, v));
    else
        spdlog::warn("'{}' uniform not found.", name);
}
