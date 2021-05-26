macro(run_conan)
  if(NOT EXISTS "${CMAKE_BINARY_DIR}/download/conan.cmake")
    message(STATUS "Downloading conan.cmake from https://github.com/conan-io/cmake-conan")
    file(DOWNLOAD "https://raw.githubusercontent.com/conan-io/cmake-conan/master/conan.cmake"
         "${CMAKE_BINARY_DIR}/download/conan.cmake")
  endif()

  include(${CMAKE_BINARY_DIR}/download/conan.cmake)

  conan_add_remote(NAME bincrafters URL https://api.bintray.com/conan/bincrafters/public-conan)

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
