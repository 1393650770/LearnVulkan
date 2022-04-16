cmake_minimum_required (VERSION 3.22)


set(VCPKG_ROOT "${VCPKG_SOURCE_DIR}/vcpkg/scripts/buildsystems/vcpkg.cmake" CACHE PATH "")
set(CMAKE_TOOLCHAIN_FILE ${VCPKG_ROOT})
set(INCLUDE include)

project(LearnVulkan)

set(EXECUTABLE_SOURCES
    src/main.cpp
    src/LE2_HelloTriangle.cpp
)

set(LearnVulkan_LIBRARY_LIB_SOURCES

)

include(CheckCXXCompilerFlag)

CHECK_CXX_COMPILER_FLAG("-std=c++17" COMPILER_SUPPORTS_CXX17)

find_package(glm CONFIG REQUIRED)
find_package(glfw3 CONFIG REQUIRED)

include_directories(${INCLUDE})

LINK_DIRECTORIES(${CMAKE_SOURCE_DIR}/lib)

if(COMPILER_SUPPORTS_CXX17)
    set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -std=c++17")
else()
    set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -std=c++14")
    message(STATUS "The compiler ${CMAKE_CXX_COMPILER} has no C++17 support. Please use a different C++ compiler.")
endif()



add_executable(LearnVulkan  ${EXECUTABLE_SOURCES})


target_link_libraries(LearnVulkan PRIVATE glfw)
target_link_libraries(LearnVulkan PRIVATE glm::glm)
target_link_libraries(LearnVulkan PRIVATE vulkan/vulkan-1)