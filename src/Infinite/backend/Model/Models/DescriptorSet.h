#ifndef VULKAN_DESCRIPTORSET_H
#define VULKAN_DESCRIPTORSET_H

#include "../../Software/ShaderAnalyzer.h"
#include <cstdint>
#include <vector>
#include <vulkan/vulkan_core.h>

namespace Infinite {

class DescriptorSet {
private:
  inline static VkDescriptorPool descriptorPool;

  std::vector<VkDescriptorSet> descriptorSets;

  VkDescriptorSetLayout *descriptorSetLayout;

  ShaderLayout *shaderLayout;

public:
  void createDescriptorSets(VkDevice device,
                            std::vector<std::vector<VkBuffer>>
                                buffers, // need one buffer per frame in flight
                                         // and one for each descriptor set
                            std::vector<std::vector<VkDeviceSize>> bufferSizes,
                            std::vector<VkImageView> imageViews,
                            std::vector<VkSampler> samplers);

  static void createDescriptorPool(VkDevice device);

  static VkDescriptorSetLayout
  createDescriptorSetLayout(VkDevice device,
                            std::vector<DescriptorSetLayout> layout);

  static VkDescriptorPool &getDescriptorPool();

  const std::vector<VkDescriptorSet> &getDescriptorSets() const;

  DescriptorSet(VkDevice device, VkDescriptorSetLayout *descriptorSetLayout,
                ShaderLayout *_shaderLayout);
  static void cleanUpDescriptors(VkDevice device);
  void destroy(VkDevice device);

  inline void setDescriptorSetLayout(VkDescriptorSetLayout *layout) {
    descriptorSetLayout = layout;
  }

  DescriptorSet();

  inline VkDescriptorSetLayout *getDescriptorSetLayout() {
    return DescriptorSet::descriptorSetLayout;
  }
};

} // namespace Infinite

#endif // VULKAN_DESCRIPTORSET_H
