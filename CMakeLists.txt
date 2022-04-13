cmake_minimum_required (VERSION 3.22)


set(VCPKG_ROOT "${VCPKG_SOURCE_DIR}/vcpkg/scripts/buildsystems/vcpkg.cmake" CACHE PATH "")
set(CMAKE_TOOLCHAIN_FILE ${VCPKG_ROOT})
set(INCLUDE include)

project(LearnVulkan)

set(EXECUTABLE_SOURCES
    src/main.cpp
)


find_package(glm CONFIG REQUIRED)
find_package(glfw3 CONFIG REQUIRED)

include_directories(${INCLUDE})

LINK_DIRECTORIES(${CMAKE_SOURCE_DIR}/lib)

add_executable(LearnVulkan  ${EXECUTABLE_SOURCES})

target_link_libraries(LearnVulkan PRIVATE glfw)
target_link_libraries(LearnVulkan PRIVATE glm::glm)
target_link_libraries(LearnVulkan PRIVATE vulkan/vulkan-1)