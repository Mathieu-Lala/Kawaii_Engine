macro(run_conan)
  if(NOT EXISTS "${CMAKE_BINARY_DIR}/../download/conan.cmake")
    message(STATUS "Downloading conan.cmake from https://github.com/conan-io/cmake-conan")
    file(
      DOWNLOAD "https://raw.githubusercontent.com/conan-io/cmake-conan/v0.16.1/conan.cmake"
      "${CMAKE_BINARY_DIR}/../download/conan.cmake"
      EXPECTED_HASH SHA256=396e16d0f5eabdc6a14afddbcfff62a54a7ee75c6da23f32f7a31bc85db23484
      TLS_VERIFY ON)
  endif()

  include(${CMAKE_BINARY_DIR}/../download/conan.cmake)

  conan_add_remote(NAME bincrafters URL https://bincrafters.jfrog.io/artifactory/api/conan/public-conan)

  # cmake-format: off
  conan_cmake_run(
    BASIC_SETUP
    NO_OUTPUT_DIRS
    CMAKE_TARGETS

    CONANFILE conanfile.txt
    BUILD missing
    INSTALL_FOLDER ${CMAKE_BINARY_DIR}/conan
    BUILD_TYPE ${CMAKE_BUILD_TYPE}

    SETTINGS
    cppstd=20
  )
  # cmake-format: on

endmacro()
