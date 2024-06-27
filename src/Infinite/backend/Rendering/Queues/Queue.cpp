#include "Queue.h"

#include "../../../Infinite.h"
#include <iostream>
#include <optional>

namespace Infinite {
void findQueueFamilies(VkPhysicalDevice device,
                       Queue queues[static_cast<int>(QueueOrder::Count)]) {

  uint32_t queueFamilyCount = 0;
  // error here
  vkGetPhysicalDeviceQueueFamilyProperties(device, &queueFamilyCount, nullptr);

  std::vector<VkQueueFamilyProperties> queueFamilies(queueFamilyCount);
  vkGetPhysicalDeviceQueueFamilyProperties(device, &queueFamilyCount,
                                           queueFamilies.data());

  int i = 0;
  for (const auto &queueFamily : queueFamilies) {
    bool allIndicesFound = true;
    for (int j = 0; j < static_cast<uint32_t>(QueueOrder::Count); j++) {
      if (queues[j].index.type == QueueType::PRESENT &&
          !queues[j].index.index.has_value()) {
        VkBool32 presentSupport = false;
        vkGetPhysicalDeviceSurfaceSupportKHR(device, i, surface,
                                             &presentSupport);
        if (presentSupport) {
          queues[j].index.index = i;
        }
      } else if ((queueFamily.queueFlags &
                  static_cast<VkQueueFlags>(queues[j].index.type)) &&
                 !queues[j].index.index.has_value()) {
        queues[j].index.index = i;
      }
    }

    for (uint32_t k = 0; k < static_cast<uint32_t>(QueueOrder::Count); k++) {
      if (!queues[k].index.index.has_value()) {
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