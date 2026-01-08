cmake_minimum_required(VERSION 3.20)
project(AR1_VCPU VERSION 2.1 LANGUAGES CXX)

set(CMAKE_CXX_STANDARD 20)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
# ImGui requires no strict aliasing on some compilers, but standard C++ usually fine
set(CMAKE_EXPORT_COMPILE_COMMANDS ON)

# Warning flags
add_compile_options(-Wall -Wextra -Wpedantic)

# Include directories
include_directories(include)
include_directories(src/vendor/imgui)
include_directories(src/vendor/imgui/backends)

# Find SDL2
find_package(SDL2 REQUIRED)
include_directories(${SDL2_INCLUDE_DIRS})

# Find Capstone
find_path(CAPSTONE_INCLUDE_DIR NAMES capstone/capstone.h)
find_library(CAPSTONE_LIBRARY NAMES capstone)
include_directories(${CAPSTONE_INCLUDE_DIR})

# Source files
set(SOURCES
    src/main.cpp
    src/cpu/core.cpp
    src/cpu/registers.cpp
    src/cpu/cache.cpp
    src/cpu/predictor.cpp

    src/soc/ram.cpp
    src/soc/bus.cpp
    src/soc/loader.cpp
    src/soc/gic.cpp
    src/device/virtio_blk.cpp
    src/device/framebuffer.cpp
    src/device/display.cpp
    
    # ImGui
    src/vendor/imgui/imgui.cpp
    src/vendor/imgui/imgui_draw.cpp
    src/vendor/imgui/imgui_tables.cpp
    src/vendor/imgui/imgui_widgets.cpp
    src/vendor/imgui/backends/imgui_impl_sdl2.cpp
    src/vendor/imgui/backends/imgui_impl_sdlrenderer2.cpp
)

# Executable
add_executable(ar1_vcpu ${SOURCES})

# Link frameworks and libraries
target_link_libraries(ar1_vcpu 
    "-framework Hypervisor" 
    "-framework CoreFoundation"
    ${SDL2_LIBRARIES}
    ${CAPSTONE_LIBRARY}
)

# Sign with entitlements after build
add_custom_command(TARGET ar1_vcpu POST_BUILD
    COMMAND codesign --entitlements ${CMAKE_SOURCE_DIR}/ar1.entitlements --force -s - $<TARGET_FILE:ar1_vcpu>
    COMMENT "Signing ar1_vcpu with hypervisor entitlements..."
)
