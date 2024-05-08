#include "ComputeRenderPass.h"
#include "../Engine.h"
#include <cstring>
#include <random>
#include <sys/types.h>
#include <vulkan/vulkan_core.h>
namespace Infinite {
VkPipelineShaderStageCreateInfo createExtraShaderModule(VkDevice device, VkShaderModule &computeShaderModule) {
  auto computeShaderCode = readFile("shaders/compute.spv");

  computeShaderModule =
      createShaderModule(computeShaderCode, device);

  VkPipelineShaderStageCreateInfo computeShaderStageInfo{};
  computeShaderStageInfo.sType =
      VK_STRUCTURE_TYPE_PIPELINE_SHADER_STAGE_CREATE_INFO;
  computeShaderStageInfo.stage = VK_SHADER_STAGE_COMPUTE_BIT;
  computeShaderStageInfo.module = computeShaderModule;
  computeShaderStageInfo.pName = "main";

  return computeShaderStageInfo;
}

void ComputeRenderPass::createShaderStorageBuffers(int maxFramesInFlight,
                                                   uint32_t PARTICLE_COUNT,
                                                   uint32_t WIDTH,
                                                   uint32_t HEIGHT,
                                                   VmaAllocator allocator) {
  shaderStorageBufferAlloc.resize(maxFramesInFlight);
  // Initialize particles
  std::default_random_engine rndEngine((unsigned)time(nullptr));
  std::uniform_real_distribution<float> rndDist(0.0f, 1.0f);

  // Initial particle positions on a circle
  std::vector<Particle> particles(PARTICLE_COUNT);
  for (auto &particle : particles) {
    float r = 0.25f * sqrt(rndDist(rndEngine));
    float theta = rndDist(rndEngine) * 2 * 3.14159265358979323846;
    float x = r * cos(theta) * HEIGHT / WIDTH;
    float y = r * sin(theta);
    particle.position = glm::vec2(x, y);
    particle.velocity = glm::normalize(glm::vec2(x, y)) * 0.00025f;
    particle.color = glm::vec4(rndDist(rndEngine), rndDist(rndEngine),
                               rndDist(rndEngine), 1.0f);
  }
  VkDeviceSize bufferSize = sizeof(Particle) * PARTICLE_COUNT;

  BufferAlloc stagingBufferAlloc;
  createBuffer(bufferSize, VK_BUFFER_USAGE_TRANSFER_SRC_BIT, stagingBufferAlloc,
               VK_MEMORY_PROPERTY_HOST_VISIBLE_BIT |
                   VK_MEMORY_PROPERTY_HOST_COHERENT_BIT);

  void *data;
  vmaMapMemory(allocator, stagingBufferAlloc.allocation, &data);
  memcpy(data, particles.data(), (size_t)bufferSize);
  vmaUnmapMemory(allocator, stagingBufferAlloc.allocation);

  for (size_t i = 0; i < maxFramesInFlight; i++) {
    createBuffer(
        bufferSize,
        VK_BUFFER_USAGE_STORAGE_BUFFER_BIT | VK_BUFFER_USAGE_VERTEX_BUFFER_BIT |
            VK_BUFFER_USAGE_TRANSFER_DST_BIT,
        shaderStorageBufferAlloc[i], VK_MEMORY_PROPERTY_DEVICE_LOCAL_BIT);
    // Copy data from the staging buffer (host) to the shader storage buffer
    // (GPU)
    copyBuffer(stagingBufferAlloc, shaderStorageBufferAlloc[i], bufferSize);
  }
}

void ComputeRenderPass::createDescriptorSets(uint32_t PARTICLE_COUNT) {
  std::vector<VkDescriptorSet> computeDescriptorSets;
  for (size_t i = 0; i < MAX_FRAMES_IN_FLIGHT; i++) {
    VkDescriptorBufferInfo storageBufferInfoCurrentFrame{};
    storageBufferInfoCurrentFrame.buffer = shaderStorageBufferAlloc[i].buffer;
    storageBufferInfoCurrentFrame.offset = 0;
    storageBufferInfoCurrentFrame.range = sizeof(Particle) * PARTICLE_COUNT;

    VkWriteDescriptorSet descriptorWrite;
    descriptorWrite.sType = VK_STRUCTURE_TYPE_WRITE_DESCRIPTOR_SET;
    descriptorWrite.dstSet = computeDescriptorSets[i];
    descriptorWrite.dstBinding = 2;
    descriptorWrite.dstArrayElement = 0;
    descriptorWrite.descriptorType = VK_DESCRIPTOR_TYPE_STORAGE_BUFFER;
    descriptorWrite.descriptorCount = 1;
    descriptorWrite.pBufferInfo = &storageBufferInfoCurrentFrame;

    vkUpdateDescriptorSets(device, 1, &descriptorWrite, 0, nullptr);
  }
  descriptorSet = DescriptorSet(device, computeDescriptorSets);
}


    void ComputeRenderPass::createComputePipeline(VkDevice device, VkDescriptorSetLayout setLayout) {

        VkPipelineLayoutCreateInfo pipelineLayoutInfo{};
        pipelineLayoutInfo.sType = VK_STRUCTURE_TYPE_PIPELINE_LAYOUT_CREATE_INFO;
        pipelineLayoutInfo.setLayoutCount = 1;
        pipelineLayoutInfo.pSetLayouts = &setLayout; // is this right??

        if (vkCreatePipelineLayout(device, &pipelineLayoutInfo, nullptr, &computePipelineLayout) != VK_SUCCESS) {
            throw std::runtime_error("failed to create compute pipeline layout!");
        }

        VkShaderModule computeShaderModule;
        VkComputePipelineCreateInfo pipelineInfo{};
        pipelineInfo.sType = VK_STRUCTURE_TYPE_COMPUTE_PIPELINE_CREATE_INFO;
        pipelineInfo.layout = computePipelineLayout;
        pipelineInfo.stage = createExtraShaderModule(device, computeShaderModule);

        if (vkCreateComputePipelines(device, VK_NULL_HANDLE, 1, &pipelineInfo, nullptr, &computePipeline) != VK_SUCCESS) {
            throw std::runtime_error("failed to create compute pipeline!");
        }

        vkDestroyShaderModule(device, computeShaderModule, nullptr);
    }

}; // namespace Infinite