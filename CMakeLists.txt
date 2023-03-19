cmake_minimum_required(VERSION 3.24)
project(Vulkan)

set(CMAKE_CXX_STANDARD 17)

# Find the Vulkan SDK
set(Vulkan_LIBRARY "C:/VulkanSDK/1.3.239.0/lib/vulkan-1.lib")
find_package(Vulkan REQUIRED)
set(GLFW_INCLUDE_DIR "C:/glfw-3.3.8/include")
set(GLFW_LIB_DIR "C:/glfw-3.3.8/src/Release")
include_directories(${GLFW_INCLUDE_DIR})
link_directories(${GLFW_LIB_DIR})


# Add GLFW library
#set(GLFW_LIB_DIR "C:/glfw-3.3.8.bin.WIN64/lib-vc2022")
#set(GLFW_LIBRARIES glfw3 glfw3.dll)
#include_directories(${GLFW_INCLUDE_DIR})
#
#
#set(GLFW_BUILD_DOCS OFF CACHE BOOL "" FORCE)
#set(GLFW_BUILD_TESTS OFF CACHE BOOL "" FORCE)
#set(GLFW_BUILD_EXAMPLES OFF CACHE BOOL "" FORCE)

#add_subdirectory(${GLFW_SOURCE_DIR} build/glfw)


include_directories(
        ${Vulkan_INCLUDE_DIRS}
        include # Add your additional include directory here
)
# add glm and VMA
include_directories("C:/VulkanSDK/1.3.239.0/Include")
#if(CMAKE_BUILD_TYPE STREQUAL "Debug")
    #file(COPY ${CMAKE_SOURCE_DIR}/assets DESTINATION ${CMAKE_BINARY_DIR}/Debug/ FILES_MATCHING PATTERN "*")
#else ()
#    file(COPY ${CMAKE_SOURCE_DIR}/assets DESTINATION ${CMAKE_BINARY_DIR}/Release/assets FILES_MATCHING PATTERN "*")
#endif ()

# Source files
set(SOURCES
        src/main.cpp # Add your source files here
        src/sbt_image.h src/Camera.cpp src/Camera.h)


# Combine source files and additional source directories
list(APPEND SOURCES ${ADDITIONAL_SOURCES})


# Link against Vulkan, GLFW, GLM, and VMA
add_executable(${PROJECT_NAME} ${SOURCES})
#link_directories(Vulkan_LIBRARY)
target_link_libraries(${PROJECT_NAME} ${Vulkan_LIBRARIES} glfw3)
