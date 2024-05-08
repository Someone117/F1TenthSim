#ifndef VULKAN_INCLUDES_H
#define VULKAN_INCLUDES_H
#pragma once

#include "../../../libs/vk_mem_alloc.h"
#include <GLFW/glfw3.h>
#include <algorithm>
#include <cstdint>
#include <glm/glm.hpp>
#include <iostream>
#include <stdexcept>
#include <vector>
#include <vulkan/vulkan_core.h>

#ifndef M_PI
#define M_PI 3.14159265358979323846
#endif

namespace Infinite {
enum class QueueType { // also order of queues
  GRAPHICS = VK_QUEUE_GRAPHICS_BIT,
  PRESENT = 0x00000000,
  COMPUTE = VK_QUEUE_COMPUTE_BIT
};
enum class QueueOrder { GRAPHICS, PRESENT, COMPUTE, Count };
} // namespace Infinite

#endif // VULKAN_INCLUDES_H
