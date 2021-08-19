cmake_minimum_required(VERSION 3.11)

project(sleepy_boi LANGUAGES CXX C)

set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)

# raylib
find_package(raylib 3.0 QUIET)
if (NOT raylib_FOUND)
  include(FetchContent)
  FetchContent_Declare(
    raylib
    URL https://github.com/raysan5/raylib/archive/master.tar.gz
  )
  FetchContent_GetProperties(raylib)
  if (NOT raylib_POPULATED)
    set(FETCHCONTENT_QUIET NO)
    FetchContent_Populate(raylib)
    set(BUILD_EXAMPLES OFF CACHE BOOL "" FORCE)
    add_subdirectory(${raylib_SOURCE_DIR} ${raylib_BINARY_DIR})
  endif()
endif()

# raygui
add_library(raygui INTERFACE)
file(GLOB sources third_party/raygui/src/*.h)
set(RAYGUI_HEADERS ${sources})
install(FILES ${RAYGUI_HEADERS} DESTINATION include)
target_include_directories(raygui INTERFACE third_party/raygui/src)

add_executable(sleepy_boi src/mmu.cpp src/cpu/cpu.cpp src/gameboy.cpp src/debugger.cpp src/utility.cpp src/timer.cpp src/cpu/interrupt_controller.cpp src/video/video.cpp src/video/framebuffer.cpp src/cartridge.cpp src/main.cpp)
target_link_libraries(sleepy_boi raylib raygui)

# OSX Support
if (APPLE)
    target_link_libraries(sleepy_boi "-framework IOKit")
    target_link_libraries(sleepy_boi "-framework Cocoa")
    target_link_libraries(sleepy_boi "-framework OpenGL")
endif()
