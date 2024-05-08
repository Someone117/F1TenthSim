#ifndef COMPUTE_RENDER_PASS_H
#define COMPUTE_RENDER_PASS_H

#pragma once

#include "../../Model/Models/DescriptorSet.h"
#include "RenderPass.h"

namespace Infinite {
struct Particle {
  glm::vec2 position;
  glm::vec2 velocity;
  glm::vec4 color;
};

class ComputeRenderPass {
private:
  std::vector<BufferAlloc> shaderStorageBufferAlloc;
  DescriptorSet descriptorSet;
  VkPipelineLayout computePipelineLayout;

public:
  VkPipelineShaderStageCreateInfo
  createExtraShaderModule(VkDevice device, VkShaderModule &computeShaderModule);
  // on Compute space
  // https://vulkan-tutorial.com/Compute_Shader

  void createShaderStorageBuffers(int maxFramesInFlight,
                                  uint32_t PARTICLE_COUNT, uint32_t WIDTH,
                                  uint32_t HEIGHT, VmaAllocator allocator);

  void createDescriptorSets(uint32_t PARTICLE_COUNT);

  void createComputePipeline(VkDevice device, VkDescriptorSetLayout setLayout);
};

} // namespace Infinite

#endif // COMPUTE_RENDER_PASS_H