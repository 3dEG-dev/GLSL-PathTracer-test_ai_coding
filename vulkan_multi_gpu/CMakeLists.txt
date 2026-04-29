cmake_minimum_required(VERSION 3.16)
project(VulkanMultiGPUPathTracer VERSION 1.0 LANGUAGES CXX)

set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)

# Vulkan finden
find_package(Vulkan REQUIRED)

# GLFW für Fenstererstellung (optional, auch headless möglich)
find_package(glfw3 QUIET)

# Source Dateien
set(SOURCES
    src/main.cpp
    src/vulkan_context.cpp
    src/multi_gpu_manager.cpp
    src/path_tracer.cpp
    src/scene.cpp
)

set(HEADERS
    src/vulkan_context.h
    src/multi_gpu_manager.h
    src/path_tracer.h
    src/scene.h
    src/types.h
)

# Executable erstellen
add_executable(${PROJECT_NAME} ${SOURCES} ${HEADERS})

# Include Verzeichnisse
target_include_directories(${PROJECT_NAME} PRIVATE
    ${Vulkan_INCLUDE_DIRS}
    ${CMAKE_SOURCE_DIR}/src
)

# Libraries verlinken
target_link_libraries(${PROJECT_NAME} PRIVATE
    Vulkan::Vulkan
)

if(glfw3_FOUND)
    target_link_libraries(${PROJECT_NAME} PRIVATE glfw)
    target_compile_definitions(${PROJECT_NAME} PRIVATE USE_GLFW)
endif()

# Compiler Optionen
target_compile_options(${PROJECT_NAME} PRIVATE
    $<$<CXX_COMPILER_ID:GNU>:-Wall -Wextra -O3>
    $<$<CXX_COMPILER_ID:MSVC>:/W4 /O2>
)

# Shader Compilation (SPIR-V)
# Hier könnten GLSL zu SPIR-V Shader kompiliert werden
# add_custom_command(...)

message(STATUS "Vulkan Include: ${Vulkan_INCLUDE_DIRS}")
message(STATUS "Vulkan Library: ${Vulkan_LIBRARIES}")
