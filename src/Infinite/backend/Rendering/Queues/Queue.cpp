#include "Queue.h"
#include "../Engine.h"
#include <cstdint>

namespace Infinite {
void findQueueFamilies(VkPhysicalDevice device,
                       Queue queues[static_cast<int>(QueueOrder::Count)]) {

  uint32_t queueFamilyCount = 0;
  vkGetPhysicalDeviceQueueFamilyProperties(device, &queueFamilyCount, nullptr);

  std::vector<VkQueueFamilyProperties> queueFamilies(queueFamilyCount);
  vkGetPhysicalDeviceQueueFamilyProperties(device, &queueFamilyCount,
                                           queueFamilies.data());

  int i = 0;
  for (const auto &queueFamily : queueFamilies) {
    for (int j = 0; j < static_cast<uint32_t>(QueueOrder::Count); j++) {
      auto &queue = queues[j];
      if (queue.getIndex().type == QueueType::PRESENT) {
        VkBool32 presentSupport = false;
        vkGetPhysicalDeviceSurfaceSupportKHR(
            device, i, Engine::getEngine().getSurface(), &presentSupport);
        if (presentSupport) {
          queue.getIndex().index = i;
        }
      } else if ((queueFamily.queueFlags &
                  static_cast<VkQueueFlags>(queue.getIndex().type)) &&
                 !queue.getIndex().index.has_value()) {
        queue.getIndex().index = i;
      }
    }

    bool allIndicesFound = true;
    for (uint32_t k = 0; k < static_cast<uint32_t>(QueueOrder::Count); k++) {
      if (!queues[k].getIndex().index.has_value()) {
        allIndicesFound = false;
        break;
      }
    }

    if (allIndicesFound) {
      break;
    }

    i++;
  }
}
void Queue::createQueue(VkDevice device) {
  vkGetDeviceQueue(device, index.index.value(), 0, &queue);
}

void Queue::submit(VkSubmitInfo submitInfo) {
  vkQueueSubmit(queue, 1, &submitInfo, VK_NULL_HANDLE);
}

void Queue::waitIdle() { vkQueueWaitIdle(queue); }

} // namespace Infinite