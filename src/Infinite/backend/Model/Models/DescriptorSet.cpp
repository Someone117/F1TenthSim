#include "DescriptorSet.h"
#include "../../../util/VulkanUtils.h"
#include "../../Settings.h"
#include "Model.h"
#include <cstdint>
#include <vulkan/vulkan_core.h>

namespace Infinite {

DescriptorSet::DescriptorSet(){};
DescriptorSet::DescriptorSet(VkDevice device, std::vector<VkDescriptorSet> descriptorSets) {
  std::vector<VkDescriptorSetLayout> layouts(MAX_FRAMES_IN_FLIGHT,
                                             descriptorSetLayout);
  VkDescriptorSetAllocateInfo allocInfo{};
  allocInfo.sType = VK_STRUCTURE_TYPE_DESCRIPTOR_SET_ALLOCATE_INFO;
  allocInfo.descriptorPool = descriptorPool;
  allocInfo.descriptorSetCount = static_cast<uint32_t>(MAX_FRAMES_IN_FLIGHT);
  allocInfo.pSetLayouts = layouts.data();

  descriptorSets.resize(MAX_FRAMES_IN_FLIGHT);
  if (vkAllocateDescriptorSets(device, &allocInfo, descriptorSets.data()) !=
      VK_SUCCESS) {
    throw std::runtime_error("failed to allocate descriptor sets!");
  }
}
void DescriptorSet::createDescriptorPool(VkDevice device) {
  std::array<VkDescriptorPoolSize, 3> poolSizes{}; // TODO: implement a way to resize this thing
  poolSizes[0].type = VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER;
  poolSizes[0].descriptorCount =
      static_cast<uint32_t>(MAX_FRAMES_IN_FLIGHT * MAX_MODELS);
  poolSizes[1].type = VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER;
  poolSizes[1].descriptorCount =
      static_cast<uint32_t>(MAX_FRAMES_IN_FLIGHT * MAX_MODELS);
      
  poolSizes[2].type = VK_DESCRIPTOR_TYPE_STORAGE_BUFFER;
  poolSizes[2].descriptorCount = static_cast<uint32_t>(MAX_FRAMES_IN_FLIGHT) * 2;

  VkDescriptorPoolCreateInfo poolInfo{};
  poolInfo.sType = VK_STRUCTURE_TYPE_DESCRIPTOR_POOL_CREATE_INFO;
  poolInfo.poolSizeCount = static_cast<uint32_t>(poolSizes.size());
  poolInfo.pPoolSizes = poolSizes.data();
  poolInfo.flags =
      VK_DESCRIPTOR_POOL_CREATE_FREE_DESCRIPTOR_SET_BIT; // Include the flag
  poolInfo.maxSets = static_cast<uint32_t>(MAX_FRAMES_IN_FLIGHT * MAX_MODELS);

  if (vkCreateDescriptorPool(device, &poolInfo, nullptr, &descriptorPool) !=
      VK_SUCCESS) {
    throw std::runtime_error("failed to create descriptor pool!");
  }
}
VkDescriptorSetLayout DescriptorSet::createDescriptorSetLayout(
    VkDevice device, VkDescriptorSetLayout *setLayout,
    std::vector<DescriptorSetLayout> layout) {
  std::vector<VkDescriptorSetLayoutBinding> bindings;
  for (uint32_t i = 0; i < layout.size(); i++) {
    VkDescriptorSetLayoutBinding layoutBinding{};
    layoutBinding.binding = i;
    layoutBinding.descriptorCount = layout[i].descriptorCount;
    layoutBinding.descriptorType = layout[i].descriptorType;
    layoutBinding.pImmutableSamplers = nullptr;
    layoutBinding.stageFlags = layout[i].stageFlags;
    bindings.push_back(layoutBinding);
  }

  VkDescriptorSetLayoutBinding uboLayoutBinding{};
  uboLayoutBinding.binding = 0;
  uboLayoutBinding.descriptorCount = 1;
  uboLayoutBinding.descriptorType = VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER;
  uboLayoutBinding.pImmutableSamplers = nullptr;
  uboLayoutBinding.stageFlags = VK_SHADER_STAGE_VERTEX_BIT; // change

  VkDescriptorSetLayoutBinding samplerLayoutBinding{};
  samplerLayoutBinding.binding = 1;
  samplerLayoutBinding.descriptorCount = 1;
  samplerLayoutBinding.descriptorType =
      VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER;
  samplerLayoutBinding.pImmutableSamplers = nullptr;
  samplerLayoutBinding.stageFlags = VK_SHADER_STAGE_FRAGMENT_BIT;

  VkDescriptorSetLayoutCreateInfo layoutInfo{};
  layoutInfo.sType = VK_STRUCTURE_TYPE_DESCRIPTOR_SET_LAYOUT_CREATE_INFO;
  layoutInfo.bindingCount = static_cast<uint32_t>(bindings.size());
  layoutInfo.pBindings = bindings.data();

  if (vkCreateDescriptorSetLayout(device, &layoutInfo, nullptr,
                                  &descriptorSetLayout) != VK_SUCCESS) {
    throw std::runtime_error("failed to create descriptor set layout!");
  }
  return descriptorSetLayout;
}

const std::vector<VkDescriptorSet> &DescriptorSet::getDescriptorSets() const {
  return descriptorSets;
}

void DescriptorSet::cleanUpDescriptors(VkDevice device) {
  vkDestroyDescriptorPool(device, descriptorPool, nullptr);

  vkDestroyDescriptorSetLayout(device, descriptorSetLayout, nullptr);
}
void DescriptorSet::destroy(VkDevice device) {
  vkFreeDescriptorSets(device, descriptorPool, MAX_FRAMES_IN_FLIGHT,
                       descriptorSets.data());
}
} // namespace Infinite