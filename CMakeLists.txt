cmake_minimum_required(VERSION 3.24)
project(Vulkan)

set(CMAKE_CXX_STANDARD 17)

# Find the Vulkan SDK
#set(CMAKE_CXX_FLAGS_DEBUG "/MDd")
#set(CMAKE_C_FLAGS_DEBUG "/NODEFAULTLIB:vulkan-1.lib")


set(Vulkan_LIBRARY "C:/VulkanSDK/1.3.239.0/lib/vulkan-1.lib")
find_package(Vulkan REQUIRED)
#glfw
set(GLFW_INCLUDE_DIR "C:/glfw-3.3.8/include")
set(GLFW_LIB_DIR "C:/glfw-3.3.8/src/Release")
include_directories(${GLFW_INCLUDE_DIR})
link_directories(${GLFW_LIB_DIR})


# bullet
#set(BULLET_ROOT "C:/bullet3-3.25")
#include_directories(${BULLET_ROOT}/src)
#link_directories(${BULLET_ROOT}/src/BulletCollision
#        ${BULLET_ROOT}/src/BulletDynamics
#        ${BULLET_ROOT}/src/LinearMath)


include_directories(
        ${Vulkan_INCLUDE_DIRS}
        include # Add your additional include directory here
)

# add glm and VMA
include_directories("C:/VulkanSDK/1.3.239.0/Include"
        libs/assimp/include)


set(SOURCE_DIR "src")
file(GLOB_RECURSE SOURCES "${SOURCE_DIR}/*.cpp" "${SOURCE_DIR}/*.h")

# Combine source files and additional source directories
list(APPEND SOURCES ${ADDITIONAL_SOURCES})


add_executable(${PROJECT_NAME} ${SOURCES})

target_link_libraries(${PROJECT_NAME} ${Vulkan_LIBRARIES} glfw3)

