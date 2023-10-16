#ifndef VULKAN_DESCRIPTORSET_H
#define VULKAN_DESCRIPTORSET_H

#include "../../../util/VulkanUtils.h"
#include <vector>
#include <vulkan/vulkan_core.h>

namespace Infinite {

class DescriptorSet {

  inline static VkDescriptorPool descriptorPool;

  std::vector<VkDescriptorSet> descriptorSets;

  void createDescriptorSets();

  inline static VkDescriptorSetLayout descriptorSetLayout;

public:
  static void createDescriptorPool(VkDevice device);

  static VkDescriptorSetLayout
  createDescriptorSetLayout(VkDevice device, VkDescriptorSetLayout *setLayout);

  const std::vector<VkDescriptorSet> &getDescriptorSets() const;

  DescriptorSet(VkDevice device, std::vector<BufferAlloc> uniformBuffers,
                VkImageView textureView, VkSampler textureSampler);
  static void cleanUpDescriptors(VkDevice device);
  void destroy(VkDevice device);

  DescriptorSet();
};

} // namespace Infinite

#endif // VULKAN_DESCRIPTORSET_H
