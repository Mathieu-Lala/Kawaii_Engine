#include <GL/glew.h>

#include "graphics/Shader.hpp"
#include "helpers/macro.hpp"

template<>
auto kawe::Shader::setUniform(const std::string_view name, bool v) -> void
{
    if (const auto location = ::glGetUniformLocation(ID, name.data()); location != -1)
        CALL_OPEN_GL(::glUniform1ui(location, v));
}

template<>
auto kawe::Shader::setUniform(const std::string_view name, float v) -> void
{
    if (const auto location = ::glGetUniformLocation(ID, name.data()); location != -1)
        CALL_OPEN_GL(::glUniform1f(location, v));
}


template<>
auto kawe::Shader::setUniform(const std::string_view name, glm::mat4 mat) -> void
{
    if (const auto location = ::glGetUniformLocation(ID, name.data()); location != -1)
        CALL_OPEN_GL(::glUniformMatrix4fv(location, 1, GL_FALSE, glm::value_ptr(mat)));
}

template<>
auto kawe::Shader::setUniform(const std::string_view name, glm::dmat4 mat) -> void
{
    setUniform(name, glm::mat4{mat});
}
