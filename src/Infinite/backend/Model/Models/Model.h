#ifndef VULKAN_MODEL_H
#define VULKAN_MODEL_H

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

#include "../../../frontend/Camera.h"
#include "../Image/TexturedImage.h"

namespace Infinite {

class Model {
private:
  TexturedImage texture;
  BufferAlloc vertexBuffer{};
  BufferAlloc indexBuffer{};
  uint32_t indexCount;

public:
  uint32_t getIndexCount() const;

private:
  glm::vec3 position{};
  glm::vec3 scale{};

  float xAngle{};
  float yAngle{};
  float zAngle{};

  std::vector<BufferAlloc> uniformBuffers;

  std::string name;

  static void loadModel(std::vector<Vertex> &vertices,
                        std::vector<uint32_t> &indices,
                        const std::string &model_path);

  std::vector<BufferAlloc> createUniformBuffer();

  DescriptorSet descriptorSet;

public:
  Model(std::string _name, std::string model_path, const char *texture_path,
        VkDevice device, VkPhysicalDevice physicalDevice,
        VmaAllocator allocator, unsigned int width, unsigned int height,
        VkFormat colorFormat);

  const glm::vec3 &getPosition() const;

  const glm::vec3 &getScale() const;

  float getXAngle() const;

  float getYAngle() const;

  float getZAngle() const;

  const std::string &getName() const;

  glm::mat4 getMatrix();

  void translate(glm::vec3 vector);

  void rotate(glm::vec3 radians);

  BufferAlloc getUniformBuffer(uint32_t currentImage);

  void updateUniformBuffer(uint32_t index, Camera camera,
                           VmaAllocator allocator, unsigned int width,
                           unsigned int height);

  const BufferAlloc &getVertexBuffer() const;

  const BufferAlloc &getIndexBuffer() const;

  const std::vector<BufferAlloc> &getUniformBuffers() const;

  void destroy(VkDevice device, VmaAllocator allocator);

  DescriptorSet getDescriptorSet() const;
};

void createVertexBuffer(BufferAlloc &vertexBuffer, std::vector<Vertex> vertices,
                        VmaAllocator allocator);
void createIndexBuffer(BufferAlloc &indexBuffer, std::vector<uint32_t> indices,
                       VmaAllocator allocator);

} // namespace Infinite

#endif // VULKAN_MODEL_H
