#pragma once

#ifndef NDEBUG

#    include <stdexcept>
// #    include <source_location>
#    include <spdlog/spdlog.h>
#    include <GL/glew.h>

namespace kawe {

inline constexpr auto GetGLErrorStr(GLenum err)
{
    switch (err) {
    case GL_NO_ERROR: return "OPEN_GL: No error";
    case GL_INVALID_ENUM: return "OPEN_GL: Invalid enum";
    case GL_INVALID_VALUE: return "OPEN_GL: Invalid value";
    case GL_INVALID_OPERATION: return "OPEN_GL: Invalid operation";
    case GL_STACK_OVERFLOW: return "OPEN_GL: Stack overflow";
    case GL_STACK_UNDERFLOW: return "OPEN_GL: Stack underflow";
    case GL_OUT_OF_MEMORY: return "OPEN_GL: Out of memory";
    default: return "OPEN_GL: Unknown error";
    }
}

} // namespace kawe

#    define SHOW_ERROR(err)                                                                            \
        do {                                                                                           \
            spdlog::error("CALL_OPEN_GL: {} at {}: {}", kawe::GetGLErrorStr(err), __FILE__, __LINE__); \
        } while (0)

#    define CALL_OPEN_GL(call)                                          \
        do {                                                            \
            call;                                                       \
            if (const auto err = ::glGetError(); GL_NO_ERROR != err) {  \
                if constexpr (!noexcept(__func__)) {                    \
                    throw std::runtime_error(kawe::GetGLErrorStr(err)); \
                } else {                                                \
                    SHOW_ERROR(err);                                    \
                }                                                       \
            }                                                           \
        } while (0)

#else

#    define SHOW_ERROR(err)

#    define CALL_OPEN_GL(call) \
        do {                   \
            call;              \
        } while (0)

#endif
