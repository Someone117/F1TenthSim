#ifndef VULKAN_COMPUTE_MODEL_H
#define VULKAN_COMPUTE_MODEL_H

#include "BaseModel.h"
#include <vector>

#ifndef MAX_MODELS
#define MAX_MODELS 128
#endif

#ifndef TINYOBJLOADER_IMPLEMENTATION
#define TINYOBJLOADER_IMPLEMENTATION
#endif

#ifndef TINYOBJLOADER_USE_MAPBOX_EARCUT
#define TINYOBJLOADER_USE_MAPBOX_EARCUT
#endif

#pragma once

#include "../Image/TexturedImage.h"

#include <cstdint>

#pragma once

namespace Infinite {

class ComputeModel : public BaseModel {

protected:
  std::vector<BufferAlloc> shaderStorageBufferAlloc;

  float lastFrameTime = 1.0f;

  std::vector<BufferAlloc> uniformBuffers2;
  DescriptorSet descriptorSet2;

  TexturedImage texture;
  BufferAlloc vertexBuffer{};
  BufferAlloc indexBuffer{};
  uint32_t indexCount;

public:
  inline Image * getTexture() { return &texture; }
  ComputeModel(uint32_t WIDTH, uint32_t HEIGHT, VmaAllocator allocator);

  void createDescriptorSets(VkDevice device,
                            VkDescriptorSetLayout *descriptorSetLayout,
                            ShaderLayout *shaderLayout) override;

  void createDescriptorSets2(VkDevice device,
                             VkDescriptorSetLayout *descriptorSetLayout,
                             ShaderLayout *shaderLayout);

  void destroy(VkDevice device, VmaAllocator allocator) override;

  void updateUniformBuffer(uint32_t index, Camera camera,
                           VmaAllocator allocator, unsigned int width,
                           unsigned int height) override;

  const BufferAlloc &getVertexBuffer() const override;
  const BufferAlloc &getIndexBuffer() const override;
  inline uint32_t getIndexCount() const override { return indexCount; };
  inline DescriptorSet getDescriptorSet2() { return descriptorSet2; };
};
} // namespace Infinite

#endif // VULKAN_COMPUTE_MODEL_H
