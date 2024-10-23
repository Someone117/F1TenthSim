cmake_minimum_required(VERSION 3.22)
project(Vulkan)

set(CMAKE_CXX_STANDARD 17)

# Find the Vulkan SDK
# if (WINDOWS)
# set(Vulkan_LIBRARY "C:/VulkanSDK/1.3.239.0/lib/vulkan-1.lib")
# # elseif(UNIX AND NOT APPLE)
# endif()

# set(CMAKE_CXX_FLAGS_DEBUG "/MDd")
# set(CMAKE_C_FLAGS_DEBUG "/NODEFAULTLIB:vulkan-1.lib")
find_package(Vulkan REQUIRED)

#glfw3
find_package(glfw3 REQUIRED)

#glm
find_package(glm REQUIRED)
include_directories(${GLM_INCLUDE_DIRS})

include_directories("libs")


#all files unser src are sources
set(SOURCE_DIR "./")
file(GLOB_RECURSE SOURCES "${SOURCE_DIR}/src/*.cpp" "${SOURCE_DIR}/src/*.hpp" "${SOURCE_DIR}/src/*.h" "${SOURCE_DIR}/src/*.c")

file(GLOB_RECURSE LIBS "${SOURCE_DIR}/libsSrc/SPIRV-Cross/*.cpp" "${SOURCE_DIR}/libsSrc/SPIRV-Cross/*.c" "${SOURCE_DIR}/libsSrc/SPIRV-Cross/*.hpp" "${SOURCE_DIR}/libsSrc/SPIRV-Cross/*.h")


# Combine source files and additional source directories
list(APPEND SOURCES ${ADDITIONAL_SOURCES} ${LIBS})

add_executable(${PROJECT_NAME} ${SOURCES})

target_link_libraries(${PROJECT_NAME} ${Vulkan_LIBRARIES} glfw)