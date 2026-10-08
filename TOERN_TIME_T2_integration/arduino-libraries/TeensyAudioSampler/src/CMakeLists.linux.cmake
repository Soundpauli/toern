cmake_minimum_required(VERSION 3.5)
set(teensypolyphony_VERSION 1.0.1)

set(INCLUDE_INSTALL_DIR ${CMAKE_INSTALL_PREFIX}/include/teensypolyphony/)
set(LIB_INSTALL_DIR ${CMAKE_INSTALL_PREFIX}/lib/teensypolyphony )

find_package(teensy_x86_stubs)
include_directories(${teensy_x86_stubs_INCLUDE_DIR})

find_package(teensy_audio_x86_stubs)
include_directories(${teensy_audio_x86_stubs_INCLUDE_DIR})

find_package(teensy_x86_sd_stubs)
include_directories(${teensy_x86_sd_stubs_INCLUDE_DIR})

find_package(teensy_variable_playback)
include_directories(${teensy_variable_playback_INCLUDE_DIR})

find_package(teensy_st7735_linux_stubs)
include_directories(${teensy_st7735_linux_stubs_INCLUDE_DIR})

add_library(teensypolyphony STATIC ${HEADER_FILES} ${SOURCE_FILES})

set_target_properties(teensypolyphony PROPERTIES PUBLIC_HEADER "${HEADER_FILES}")
set_target_properties(teensypolyphony PROPERTIES LINKER_LANGUAGE C)

include(CMakePackageConfigHelpers)
configure_package_config_file(../cmake/teensypolyphony.cmake.in
        ${CMAKE_CURRENT_BINARY_DIR}/teensypolyphonyConfig.cmake
        INSTALL_DESTINATION ${LIB_INSTALL_DIR}/teensypolyphony/cmake
        PATH_VARS)

write_basic_package_version_file(
        ${CMAKE_CURRENT_BINARY_DIR}/teensypolyphonyConfigVersion.cmake
        VERSION ${teensypolyphony_VERSION}
        COMPATIBILITY SameMajorVersion )

install(TARGETS teensypolyphony DESTINATION
        LIBRARY DESTINATION ${LIB_INSTALL_DIR}
        ARCHIVE DESTINATION ${LIB_INSTALL_DIR}
        PUBLIC_HEADER DESTINATION "include/teensypolyphony"
        )

install(FILES
        ${CMAKE_CURRENT_BINARY_DIR}/teensypolyphonyConfig.cmake
        ${CMAKE_CURRENT_BINARY_DIR}/teensypolyphonyConfigVersion.cmake
        ${CMAKE_CURRENT_SOURCE_DIR}/../cmake/uninstall.cmake
        DESTINATION "lib/cmake/teensypolyphony" )
##
# copied from: https://gist.github.com/royvandam/3033428
# Add uninstall target
# Requirements: Copy the uninstall.cmake file to the appropriate CMAKE_MODULE_PATH.
#
set(CMAKE_VERBOSE_MAKEFILE 1)
add_custom_target(installarduino
        "${CMAKE_COMMAND}" -P "${CMAKE_CURRENT_SOURCE_DIR}/../cmake/install_arduino_library.cmake")

add_custom_target(uninstall
        "${CMAKE_COMMAND}" -P "${CMAKE_CURRENT_SOURCE_DIR}/../cmake/uninstall.cmake")
