#include "Model.h"
#include "../../../Infinite.h"
#include "../../../util/VulkanUtils.h"
#include "../../../util/constants.h"
#include "BaseModel.h"
#include "DescriptorSet.h"
#include <cstddef>
#include <cstdint>
#include <glm/ext/matrix_clip_space.hpp>
#include <glm/gtx/hash.hpp>
#include <iostream>
#include <stdexcept>
#include <sys/types.h>
#include <vector>
#include <vulkan/vulkan_core.h>


namespace Infinite {
Model::Model(std::string _name, std::string model_path,
             const char *texture_path, VkDevice device,
             VkPhysicalDevice physicalDevice, VmaAllocator allocator,
             unsigned int width, unsigned int height, VkFormat colorFormat)
    : BaseModel(_name), texture(texture_path) { // big errs

  texture.create(width, height, colorFormat, physicalDevice, allocator);

  // std::vector<uint32_t> indices;

  loadModel(vertices, indices, model_path);

  indexCount = indices.size();

  createVertexBuffer(vertexBuffer, vertices, allocator,
                     queues[static_cast<uint32_t>(QueueOrder::GRAPHICS)].queue);

  createIndexBuffer(indexBuffer, indices, allocator,
                    queues[static_cast<uint32_t>(QueueOrder::GRAPHICS)].queue);
}

void Model::createDescriptorSets(VkDevice device,
                                 VkDescriptorSetLayout *descriptorSetLayout,
                                 ShaderLayout *shaderLayout) {

  std::vector<std::vector<VkBuffer>> tempBuffers;
  std::vector<std::vector<VkDeviceSize>> tempBufferSizes;
  for (size_t i = 0; i < MAX_FRAMES_IN_FLIGHT; i++) {
    tempBuffers.push_back({uniformBuffers[i].buffer});
    tempBufferSizes.push_back({sizeof(UniformBufferObject)});
  }

  // create descriptor sets for this model
  descriptorSet = DescriptorSet(device, descriptorSetLayout, shaderLayout);

  descriptorSet.createDescriptorSets(device, tempBuffers, tempBufferSizes,
                                     {*texture.getImageView()},
                                     {*texture.getSampler()});
  isComplete = true;
}

void Model::destroy(VkDevice device, VmaAllocator allocator) {
  BaseModel::destroy(device, allocator);

  vmaDestroyBuffer(allocator, vertexBuffer.buffer, vertexBuffer.allocation);
  vmaDestroyBuffer(allocator, indexBuffer.buffer, indexBuffer.allocation);
  texture.destroy(allocator);
}

const BufferAlloc &Model::getVertexBuffer() const { return vertexBuffer; }

const BufferAlloc &Model::getIndexBuffer() const { return indexBuffer; }

uint32_t Model::getIndexCount() const { return indexCount; }
} // namespace Infinite