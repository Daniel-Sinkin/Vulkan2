cmake_minimum_required(VERSION 3.10)

project(VulkanEngine)

# Set the standard and export compile commands
set(CMAKE_EXPORT_COMPILE_COMMANDS ON)
set(CMAKE_CXX_STANDARD 20)
set(CMAKE_CXX_STANDARD_REQUIRED True)

# Check if the VULKAN_SDK environment variable is set
if(NOT DEFINED ENV{VULKAN_SDK})
    message(FATAL_ERROR "VULKAN_SDK environment variable is not set")
endif()

# Set Vulkan SDK paths
set(VULKAN_SDK "$ENV{VULKAN_SDK}")
set(VULKAN_INCLUDE_DIR "${VULKAN_SDK}/include")
set(VULKAN_LIBRARY_DIR "${VULKAN_SDK}/lib")
include_directories(${VULKAN_INCLUDE_DIR})

# Find required packages
find_package(Vulkan REQUIRED)
find_package(glfw3 3.3 REQUIRED)
find_package(glm REQUIRED)

file(GLOB SOURCES "src/*.cpp")

include_directories(include)
include_directories(external) # Suppresses warnings from external libraries
include_directories(${GLM_INCLUDE_DIRS})

set(CMAKE_RUNTIME_OUTPUT_DIRECTORY ${CMAKE_SOURCE_DIR})
add_executable(VulkanEngine ${SOURCES})

# Link necessary libraries
target_link_libraries(VulkanEngine
        glfw
        Vulkan::Vulkan
        ${VULKAN_LIBRARY_DIR}/libMoltenVK.dylib
)

# Set the default build type to Debug if not specified
if(NOT CMAKE_BUILD_TYPE)
    set(CMAKE_BUILD_TYPE Debug CACHE STRING "Build type" FORCE)
endif()

# Set flags for different build types
set(CMAKE_CXX_FLAGS_DEBUG "-g")
set(CMAKE_CXX_FLAGS_RELEASE "-O3 -DNDEBUG")  # Optimize for release and disable assertions

# Enable Interprocedural Optimization (LTO) for release builds
if(CMAKE_BUILD_TYPE STREQUAL "Release")
    set(CMAKE_INTERPROCEDURAL_OPTIMIZATION TRUE)
endif()

# Enable warnings, pedantic checks, and useful flags
if(CMAKE_CXX_COMPILER_ID STREQUAL "Clang" OR CMAKE_CXX_COMPILER_ID STREQUAL "GNU")
    target_compile_options(VulkanEngine PRIVATE
            -Wall            # Enable all common warnings
            -Wextra          # Enable extra warnings
            -Wpedantic       # Enforce strict ISO compliance
            -Wconversion     # Warn on implicit conversions
            -Wsign-conversion# Warn on sign conversions
            -Wshadow         # Warn if variables shadow others
            -Wnon-virtual-dtor # Warn about non-virtual destructors in base classes
            -Wold-style-cast # Warn if old-style C casts are used
            -Werror          # Treat all warnings as errors
    )

    if(CMAKE_BUILD_TYPE STREQUAL "Debug")
        # Add address and undefined behavior sanitizers for Clang and GCC
        target_compile_options(VulkanEngine PRIVATE -fsanitize=address -fsanitize=undefined)
        target_link_options(VulkanEngine PRIVATE -fsanitize=address -fsanitize=undefined)
    endif()
elseif(CMAKE_CXX_COMPILER_ID STREQUAL "MSVC")
    target_compile_options(VulkanEngine PRIVATE
            /W4             # Equivalent to -Wall on MSVC
            /WX             # Treat warnings as errors
            /permissive-    # Enforce standards conformance
            /EHsc           # Enable proper exception handling
    )
endif()

# Set the output directory
set(CMAKE_RUNTIME_OUTPUT_DIRECTORY ${CMAKE_SOURCE_DIR})