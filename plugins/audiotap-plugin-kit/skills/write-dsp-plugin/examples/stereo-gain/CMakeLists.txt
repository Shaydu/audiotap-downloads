cmake_minimum_required(VERSION 3.19)
project(StereoGain VERSION 1.0.0 LANGUAGES C)

set(CMAKE_C_STANDARD 11)
set(CMAKE_C_STANDARD_REQUIRED ON)

# Universal macOS binary (build.sh also passes this on the command line).
if(APPLE AND NOT CMAKE_OSX_ARCHITECTURES)
  set(CMAKE_OSX_ARCHITECTURES "arm64;x86_64" CACHE STRING "" FORCE)
endif()
if(APPLE AND NOT CMAKE_OSX_DEPLOYMENT_TARGET)
  set(CMAKE_OSX_DEPLOYMENT_TARGET "12.0" CACHE STRING "" FORCE)
endif()

# The CLAP headers, fetched at a pinned tag. To use a newer CLAP, change the tag and re-run
# the validator.
set(CLAP_TAG "1.2.10" CACHE STRING "free-audio/clap tag the headers come from")
include(FetchContent)
FetchContent_Declare(clap
  GIT_REPOSITORY https://github.com/free-audio/clap.git
  GIT_TAG ${CLAP_TAG}
  GIT_SHALLOW TRUE)
FetchContent_MakeAvailable(clap)

add_library(stereo-gain MODULE src/stereo_gain.c)
target_link_libraries(stereo-gain PRIVATE clap) # the headers
target_compile_options(stereo-gain PRIVATE -Wall -Wextra)
if(NOT APPLE)
  target_link_libraries(stereo-gain PRIVATE m)
  target_compile_options(stereo-gain PRIVATE -fvisibility=hidden)
endif()

if(APPLE)
  # A bundle named StereoGain.clap with a CFBundle Info.plist.
  set_target_properties(stereo-gain PROPERTIES
    BUNDLE TRUE
    BUNDLE_EXTENSION clap
    OUTPUT_NAME StereoGain
    MACOSX_BUNDLE_INFO_PLIST ${CMAKE_CURRENT_SOURCE_DIR}/Info.plist.in
    MACOSX_BUNDLE_BUNDLE_NAME "Stereo Gain"
    MACOSX_BUNDLE_GUI_IDENTIFIER "org.audiotap.example.stereo-gain"
    MACOSX_BUNDLE_SHORT_VERSION_STRING "${PROJECT_VERSION}"
    MACOSX_BUNDLE_BUNDLE_VERSION "${PROJECT_VERSION}")
else()
  # A plain shared object named StereoGain.clap.
  set_target_properties(stereo-gain PROPERTIES
    PREFIX ""
    OUTPUT_NAME StereoGain
    SUFFIX ".clap")
endif()
