#ifndef VULKAN_BASICRENDERPASS_H
#define VULKAN_BASICRENDERPASS_H

#pragma once

#include "RenderPass.h"

namespace Infinite {

class BasicRenderPass : public RenderPass {
public:
  void createRenderPass(VkDevice device, VkPhysicalDevice physicalDevice,
                        VkFormat swapChainImageFormat,
                        VkSampleCountFlagBits msaaSamples) override;
  void
  createPipeline(VkDescriptorSetLayout setLayout,
                                  VkDevice device,
                                  VkSampleCountFlagBits msaaSamples, std::vector<VkPipelineShaderStageCreateInfo> extraShaders = {}) override;
};

} // namespace Infinite

#endif // VULKAN_BASICRENDERPASS_H
