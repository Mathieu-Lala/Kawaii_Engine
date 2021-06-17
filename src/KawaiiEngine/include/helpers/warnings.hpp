#pragma once

/**
 * Usage:
 *
 * ```cpp
 *
 * DISABLE_WARNING_PUSH // we will change the warning for a section of code
 * DISABLE_WARNING_UNREFERENCED_FUNCTION
 *
 * // ... code where unreferenced function would cause an error
 *
 * DISABLE_WARNING_POP // reset to normal
 *
 * ```
 *
 */

// Wrapper macro

// clang-format off

#if defined(_MSC_VER)
#    define DISABLE_WARNING_PUSH  __pragma(warning(push))
#    define DISABLE_WARNING_POP   __pragma(warning(pop))
#    define DISABLE_WARNING(name) __pragma(warning(disable:name))

#elif defined(__GNUC__) || defined(__clang__)
#    define DO_PRAGMA(X)          _Pragma(#X)
#    define DISABLE_WARNING_PUSH  DO_PRAGMA(GCC diagnostic push)
#    define DISABLE_WARNING_POP   DO_PRAGMA(GCC diagnostic pop)
#    define DISABLE_WARNING(name) DO_PRAGMA(GCC diagnostic ignored #name)

#else
#    define DISABLE_WARNING_PUSH
#    define DISABLE_WARNING_POP

#endif

// Warnings macro

#if defined(_MSC_VER)
#    define DISABLE_WARNING_UNREFERENCED_FORMAL_PARAMETER DISABLE_WARNING(4100)
#    define DISABLE_WARNING_UNREFERENCED_FUNCTION         DISABLE_WARNING(4505)
#    define DISABLE_WARNING_MSVC_LEVEL_4                  DISABLE_WARNING(4296)
#    define DISABLE_WARNING_CONSTANT_CONDITIONAL          DISABLE_WARNING(4127)
#    define DISABLE_WARNING_NON_STD_EXTENSION             DISABLE_WARNING(4201)
#    define DISABLE_WARNING_OLD_CAST
#    define DISABLE_WARNING_SIGN_CONVERSION
#    define DISABLE_WARNING_USELESS_CAST

#elif defined(__GNUC__) || defined(__clang__)
#    define DISABLE_WARNING_UNREFERENCED_FORMAL_PARAMETER DISABLE_WARNING(-Wunused-parameter)
#    define DISABLE_WARNING_UNREFERENCED_FUNCTION         DISABLE_WARNING(-Wunused-function)
#    define DISABLE_WARNING_MSVC_LEVEL_4
#    define DISABLE_WARNING_CONSTANT_CONDITIONAL
#    define DISABLE_WARNING_NON_STD_EXTENSION
#    define DISABLE_WARNING_OLD_CAST        DISABLE_WARNING(-Wold-style-cast)
#    define DISABLE_WARNING_SIGN_CONVERSION DISABLE_WARNING(-Wsign-conversion)
#    define DISABLE_WARNING_USELESS_CAST    DISABLE_WARNING(-Wuseless-cast)

#else
#    define DISABLE_WARNING_UNREFERENCED_FORMAL_PARAMETER
#    define DISABLE_WARNING_UNREFERENCED_FUNCTION
#    define DISABLE_WARNING_MSVC_LEVEL_4
#    define DISABLE_WARNING_CONSTANT_CONDITIONAL
#    define DISABLE_WARNING_NON_STD_EXTENSION
#    define DISABLE_WARNING_OLD_CAST
#    define DISABLE_WARNING_SIGN_CONVERSION
#    define DISABLE_WARNING_USELESS_CAST

#endif

// clang-format on
