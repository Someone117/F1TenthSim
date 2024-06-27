#ifndef VULKAN_MODEL_H
#define VULKAN_MODEL_H

#include "BaseModel.h"
#include "DescriptorSet.h"
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

namespace Infinite {

class Model : public BaseModel {
private:
  TexturedImage texture;
  BufferAlloc vertexBuffer{};
  BufferAlloc indexBuffer{};
  uint32_t indexCount;
  static void loadModel(std::vector<Vertex> &vertices,
                        std::vector<uint32_t> &indices,
                        const std::string &model_path);

  DescriptorSet descriptorSet;

public:
  uint32_t getIndexCount() const;

  Model(std::string _name, std::string model_path, const char *texture_path,
        VkDevice device, VkPhysicalDevice physicalDevice,
        VmaAllocator allocator, unsigned int width, unsigned int height,
        VkFormat colorFormat);
  const BufferAlloc &getVertexBuffer() const;

  const BufferAlloc &getIndexBuffer() const;

  void destroy(VkDevice device, VmaAllocator allocator);

  DescriptorSet getDescriptorSet() const;
};
} // namespace Infinite

#endif // VULKAN_MODEL_H
