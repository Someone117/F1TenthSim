#ifndef VULKAN_DESCRIPTORSET_H
#define VULKAN_DESCRIPTORSET_H

#include "../../../util/VulkanUtils.h"
#include <cstdint>
#include <vector>
#include <vulkan/vulkan_core.h>

namespace Infinite {

struct DescriptorSetLayout {
  uint32_t descriptorCount;
  VkDescriptorType descriptorType;
  VkShaderStageFlags stageFlags;
};

class DescriptorSet {

  inline static VkDescriptorPool descriptorPool;

  std::vector<VkDescriptorSet> descriptorSets;

  void createDescriptorSets();

  inline static VkDescriptorSetLayout descriptorSetLayout;

public:
  static void createDescriptorPool(VkDevice device);

  static VkDescriptorSetLayout
  createDescriptorSetLayout(VkDevice device, VkDescriptorSetLayout *setLayout,
                            std::vector<DescriptorSetLayout> layout);

  const std::vector<VkDescriptorSet> &getDescriptorSets() const;

  DescriptorSet(VkDevice device, std::vector<VkDescriptorSet> descriptorSets);
  static void cleanUpDescriptors(VkDevice device);
  void destroy(VkDevice device);

  DescriptorSet();

  static inline VkDescriptorSetLayout getDescriptorSetLayout() { return DescriptorSet::descriptorSetLayout; }
};

} // namespace Infinite

#endif // VULKAN_DESCRIPTORSET_H
