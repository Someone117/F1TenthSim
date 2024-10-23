#ifndef SHADERANALYZER_H
#define SHADERANALYZER_H

#include <cstdint>
#include <vector>
#include <vulkan/vulkan_core.h>
#pragma once

namespace Infinite {
struct DescriptorSetLayout {
  uint32_t descriptorCount;
  VkDescriptorType descriptorType;
  VkShaderStageFlags stageFlags;
};

struct ShaderLayout {
  std::vector<VkPipelineShaderStageCreateInfo> shaderStages;
  std::vector<DescriptorSetLayout> highLevelLayout;
};
void generateShaderLayout(ShaderLayout &layout,
                          const std::vector<VkShaderModule> &shaderModules,
                          const std::vector<std::vector<char>> &shaderCodes);
VkFormat chooseFormat(uint32_t basetype, uint32_t vecsize);
void createVCI(VkPipelineVertexInputStateCreateInfo& v, std::vector<VkVertexInputAttributeDescription> * off, std::vector<VkVertexInputBindingDescription> * binding);
} // namespace Infinite
#endif // SHADERANALYZER_H