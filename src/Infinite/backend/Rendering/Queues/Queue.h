#ifndef QUEUE_H
#define QUEUE_H
#pragma once

#include "../../../util/Includes.h"
#include <optional>

namespace Infinite {
struct QueueIndex {
  QueueType type;
  std::optional<uint32_t> index;
};
class Queue {
private:
  VkQueue queue;
  QueueIndex index;

public:
  VkQueue getQueue() const { return queue; }
  QueueIndex getIndex() const { return index; }

  Queue(QueueIndex index) : index(index) {}

  void createQueue(VkDevice device);

  void submit(VkSubmitInfo submitInfo);
  void waitIdle();
};
// first is graphics, second is present, third is compute
void findQueueFamilies(VkPhysicalDevice device, Queue queues[static_cast<int>(QueueOrder::Count)]);

} // namespace Infinite

#endif // QUEUE_H