#ifndef VULKAN_CONSTANTS_H
#define VULKAN_CONSTANTS_H

#pragma once

#ifndef M_PI
#define M_PI 3.14159265358979323846
#endif

#ifndef GLFW_INCLUDE_VULKAN
#define GLFW_INCLUDE_VULKAN
#endif

#ifndef PARTICLE_COUNT
#define PARTICLE_COUNT 128
#endif

#ifndef VMA_DEBUG_LOG_FORMAT
#define VMA_LEAK_LOG_FORMAT(format, ...) do { \
        printf((format), __VA_ARGS__); \
        printf("\n"); \
    } while(false)
#endif

#ifndef GLM_FORCE_RADIANS
#define GLM_FORCE_RADIANS
#endif

#ifndef GLM_FORCE_DEPTH_ZERO_TO_ONE
#define GLM_FORCE_DEPTH_ZERO_TO_ONE
#endif

#ifndef GLM_ENABLE_EXPERIMENTAL
#define GLM_ENABLE_EXPERIMENTAL
#endif

#ifndef MAX_FRAMES_IN_FLIGHT
#define MAX_FRAMES_IN_FLIGHT 2
#endif

#include <vulkan/vulkan_core.h>

enum class QueueType { // also order of queues
  GRAPHICS = VK_QUEUE_GRAPHICS_BIT,
  PRESENT = 0x00000000,
  COMPUTE = VK_QUEUE_COMPUTE_BIT
};
enum class QueueOrder { GRAPHICS, COMPUTE, PRESENT, Count };

#endif // VULKAN_CONSTANTS_H
