#include "Model.h"
#include "../../../Infinite.h"
#include "../../../util/VulkanUtils.h"
#include "../../../util/constants.h"
#include "../tiny_obj_loader.h"
#include "BaseModel.h"
#include "DescriptorSet.h"
#include <cstdint>
#include <glm/ext/matrix_clip_space.hpp>
#include <glm/gtx/hash.hpp>
#include <iostream>
#include <stdexcept>
#include <sys/types.h>
#include <unordered_map>
#include <vector>
#include <vulkan/vulkan_core.h>

namespace std {
template <> struct hash<Infinite::Vertex> {
  size_t operator()(Infinite::Vertex const &vertex) const {
    return (hash<glm::vec3>()(vertex.pos) ^
            (hash<glm::vec2>()(vertex.texCoord) << 1));
  }
};
} // namespace std

namespace Infinite {
void Model::loadModel(std::vector<Vertex> &vertices,
                      std::vector<uint32_t> &indices,
                      const std::string &model_path) {

  tinyobj::ObjReader reader;

  if (!reader.ParseFromFile(model_path)) {
    if (!reader.Error().empty()) {
      throw std::runtime_error("TinyObjReader: " + reader.Error());
    }
  }

  if (!reader.Warning().empty()) {
    std::cout << "TinyObjReader: " << reader.Warning();
  }

  auto &attrib = reader.GetAttrib();
  auto &shapes = reader.GetShapes();

  std::unordered_map<Vertex, uint32_t> uniqueVertices{};

  for (const auto &shape : shapes) {
    for (const auto &index : shape.mesh.indices) {
      Vertex vertex({attrib.vertices[3 * index.vertex_index + 0],
                     attrib.vertices[3 * index.vertex_index + 1],
                     attrib.vertices[3 * index.vertex_index + 2]},
                    {attrib.texcoords[2 * index.texcoord_index + 0],
                     1.0f - attrib.texcoords[2 * index.texcoord_index + 1]});

      if (uniqueVertices.count(vertex) == 0) {
        uniqueVertices[vertex] = static_cast<uint32_t>(vertices.size());
        vertices.push_back(vertex);
      }

      indices.push_back(uniqueVertices[vertex]);
    }
  }
}

Model::Model(std::string _name, std::string model_path,
             const char *texture_path, VkDevice device,
             VkPhysicalDevice physicalDevice, VmaAllocator allocator,
             unsigned int width, unsigned int height, VkFormat colorFormat)
    : BaseModel(_name), texture(texture_path) { // big errs

  texture.create(width, height, colorFormat, physicalDevice, allocator);

  std::vector<DescriptorSetLayout> layout = {
      {1, VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER, VK_SHADER_STAGE_VERTEX_BIT},
      {1, VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER,
       VK_SHADER_STAGE_FRAGMENT_BIT}};

  // create descriptor sets for this model
  descriptorSet = DescriptorSet(device, layout);

  std::vector<std::vector<VkBuffer>> tempBuffers;
  std::vector<std::vector<VkDeviceSize>> tempBufferSizes;
  for (size_t i = 0; i < MAX_FRAMES_IN_FLIGHT; i++) {
    tempBuffers.push_back({uniformBuffers[i].buffer});
    tempBufferSizes.push_back({sizeof(UniformBufferObject)});
  }

  descriptorSet.createDescriptorSets(device, tempBuffers, tempBufferSizes,
                                     {*texture.getImageView()},
                                     {*texture.getSampler()});

  std::vector<Vertex> vertices;
  std::vector<uint32_t> indices;

  loadModel(vertices, indices, model_path);

  indexCount = indices.size();

  createVertexBuffer(vertexBuffer, vertices, allocator,
                     queues[static_cast<uint32_t>(QueueOrder::GRAPHICS)].queue);

  createIndexBuffer(indexBuffer, indices, allocator,
                    queues[static_cast<uint32_t>(QueueOrder::GRAPHICS)].queue);
}

void Model::destroy(VkDevice device, VmaAllocator allocator) {
  for (size_t i = 0; i < MAX_FRAMES_IN_FLIGHT; i++) {
    vmaDestroyBuffer(allocator, uniformBuffers[i].buffer,
                     uniformBuffers[i].allocation);
  }
  vmaDestroyBuffer(allocator, vertexBuffer.buffer, vertexBuffer.allocation);
  vmaDestroyBuffer(allocator, indexBuffer.buffer, indexBuffer.allocation);
  texture.destroy(allocator);

  descriptorSet.destroy(device);
}

const BufferAlloc &Model::getVertexBuffer() const { return vertexBuffer; }

const BufferAlloc &Model::getIndexBuffer() const { return indexBuffer; }

uint32_t Model::getIndexCount() const { return indexCount; }

DescriptorSet Model::getDescriptorSet() const { return descriptorSet; }

} // namespace Infinite