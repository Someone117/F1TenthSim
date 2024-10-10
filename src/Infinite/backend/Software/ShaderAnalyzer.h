#ifndef SHADERANALYZER_H
#define SHADERANALYZER_H

#include "../Model/Models/DescriptorSet.h"
#include <cstdint>
#include <optional>
#include <vector>
#include <vulkan/vulkan_core.h>
#pragma once

namespace Infinite {

struct ShaderLayout {
  std::vector<VkPipelineShaderStageCreateInfo> shaderStages;
  std::vector<DescriptorSetLayout> highLevelLayout;
};
void generateShaderLayout(ShaderLayout &layout,
                          const std::vector<VkShaderModule> &shaderModules,
                          const std::vector<std::vector<char>> &shaderCodes);
VkFormat chooseFormat(uint32_t basetype, uint32_t vecsize);

} // namespace Infinite
#endif // SHADERANALYZER_H