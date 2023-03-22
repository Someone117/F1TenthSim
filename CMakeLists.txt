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


include_directories(
        ${Vulkan_INCLUDE_DIRS}
        include # Add your additional include directory here
)
# add glm and VMA
include_directories("C:/VulkanSDK/1.3.239.0/Include")

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