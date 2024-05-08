#include "Model.h"
#include "../../Rendering/Engine.h"
#include "../../Settings.h"
#include "../tiny_obj_loader.h"
#include "DescriptorSet.h"
#include <glm/ext/matrix_clip_space.hpp>
#include <glm/gtx/hash.hpp>
#include <stdexcept>
#include <unordered_map>
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

std::vector<BufferAlloc> Model::createUniformBuffer() {
  VkDeviceSize bufferSize = sizeof(UniformBufferObject);

  uniformBuffers.resize(MAX_FRAMES_IN_FLIGHT);

  for (size_t i = 0; i < MAX_FRAMES_IN_FLIGHT; i++) {
    createBuffer(bufferSize, VK_BUFFER_USAGE_UNIFORM_BUFFER_BIT,
                 reinterpret_cast<BufferAlloc &>(uniformBuffers[i]),
                 VK_MEMORY_PROPERTY_HOST_VISIBLE_BIT |
                     VK_MEMORY_PROPERTY_HOST_COHERENT_BIT,
                 VMA_ALLOCATION_CREATE_MAPPED_BIT |
                     VMA_ALLOCATION_CREATE_HOST_ACCESS_RANDOM_BIT);
  }
  return uniformBuffers;
}

BufferAlloc Model::getUniformBuffer(uint32_t currentImage) {
  return uniformBuffers[currentImage];
}

Model::Model(std::string _name, std::string model_path,
             const char *texture_path, VkDevice device,
             VkPhysicalDevice physicalDevice, VmaAllocator allocator,
             unsigned int width, unsigned int height, VkFormat colorFormat)
    : name(std::move(_name)), texture(texture_path) {
  texture.create(width, height, colorFormat, physicalDevice, allocator);

  std::vector<BufferAlloc> uniformBuffers = createUniformBuffer();

  std::vector<VkDescriptorSet> descriptorSets;
  for (size_t i = 0; i < MAX_FRAMES_IN_FLIGHT; i++) {
    VkDescriptorBufferInfo bufferInfo{};
    bufferInfo.buffer = uniformBuffers[i].buffer;
    bufferInfo.offset = 0;
    bufferInfo.range = sizeof(UniformBufferObject);

    VkDescriptorImageInfo imageInfo{};
    imageInfo.imageLayout = VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL;
    imageInfo.imageView = *texture.getImageView();
    imageInfo.sampler = *texture.getSampler();

    std::array<VkWriteDescriptorSet, 2> descriptorWrites{};
    descriptorWrites[0].sType = VK_STRUCTURE_TYPE_WRITE_DESCRIPTOR_SET;
    descriptorWrites[0].dstSet = descriptorSets[i];
    descriptorWrites[0].dstBinding = 0;
    descriptorWrites[0].dstArrayElement = 0;
    descriptorWrites[0].descriptorType = VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER;
    descriptorWrites[0].descriptorCount = 1;
    descriptorWrites[0].pBufferInfo = &bufferInfo;

    descriptorWrites[1].sType = VK_STRUCTURE_TYPE_WRITE_DESCRIPTOR_SET;
    descriptorWrites[1].dstSet = descriptorSets[i];
    descriptorWrites[1].dstBinding = 1;
    descriptorWrites[1].dstArrayElement = 0;
    descriptorWrites[1].descriptorType =
        VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER;
    descriptorWrites[1].descriptorCount = 1;
    descriptorWrites[1].pImageInfo = &imageInfo;

    vkUpdateDescriptorSets(device,
                           static_cast<uint32_t>(descriptorWrites.size()),
                           descriptorWrites.data(), 0, nullptr);
  }

  descriptorSet = DescriptorSet(device, descriptorSets);

  std::vector<Vertex> vertices;
  std::vector<uint32_t> indices;

  loadModel(vertices, indices, model_path);

  indexCount = indices.size();

  createVertexBuffer(vertexBuffer, vertices, allocator);

  createIndexBuffer(indexBuffer, indices, allocator);

  position = glm::vec3(0.0f);
  scale = glm::vec3(1.0f);
  xAngle = 0.0f;
  yAngle = 0.0f;
  zAngle = 0.0f;
}

void createVertexBuffer(BufferAlloc &vertexBuffer, std::vector<Vertex> vertices,
                        VmaAllocator allocator) {
  VkDeviceSize bufferSize = sizeof(vertices[0]) * vertices.size();

  BufferAlloc stagingBuffer{};
  createBuffer(bufferSize, VK_BUFFER_USAGE_TRANSFER_SRC_BIT, stagingBuffer,
               VK_MEMORY_PROPERTY_HOST_VISIBLE_BIT |
                   VK_MEMORY_PROPERTY_HOST_COHERENT_BIT,
               VMA_ALLOCATION_CREATE_MAPPED_BIT |
                   VMA_ALLOCATION_CREATE_HOST_ACCESS_SEQUENTIAL_WRITE_BIT);

  void *data;
  vmaMapMemory(allocator, stagingBuffer.allocation, &data);

  memcpy(data, vertices.data(), (size_t)bufferSize);
  vmaUnmapMemory(allocator, stagingBuffer.allocation);

  createBuffer(bufferSize,
               VK_BUFFER_USAGE_TRANSFER_DST_BIT |
                   VK_BUFFER_USAGE_VERTEX_BUFFER_BIT,
               vertexBuffer);

  copyBuffer(stagingBuffer, vertexBuffer, bufferSize);

  vmaDestroyBuffer(allocator, stagingBuffer.buffer, stagingBuffer.allocation);
}

void createIndexBuffer(BufferAlloc &indexBuffer, std::vector<uint32_t> indices,
                       VmaAllocator allocator) {
  VkDeviceSize bufferSize = sizeof(indices[0]) * indices.size();

  BufferAlloc stagingBuffer{};
  createBuffer(bufferSize, VK_BUFFER_USAGE_TRANSFER_SRC_BIT, stagingBuffer,
               VK_MEMORY_PROPERTY_HOST_VISIBLE_BIT |
                   VK_MEMORY_PROPERTY_HOST_COHERENT_BIT,
               VMA_ALLOCATION_CREATE_MAPPED_BIT |
                   VMA_ALLOCATION_CREATE_HOST_ACCESS_SEQUENTIAL_WRITE_BIT);

  void *data;
  vmaMapMemory(allocator, stagingBuffer.allocation, &data);

  memcpy(data, indices.data(), (size_t)bufferSize);
  vmaUnmapMemory(allocator, stagingBuffer.allocation);

  createBuffer(bufferSize,
               VK_BUFFER_USAGE_TRANSFER_DST_BIT |
                   VK_BUFFER_USAGE_INDEX_BUFFER_BIT,
               indexBuffer);

  copyBuffer(stagingBuffer, indexBuffer, bufferSize);

  vmaDestroyBuffer(allocator, stagingBuffer.buffer, stagingBuffer.allocation);
}

void Model::updateUniformBuffer(uint32_t index, Camera camera,
                                VmaAllocator allocator, unsigned int width,
                                unsigned int height) {

  UniformBufferObject ubo{};
  ubo.model = glm::mat4(1.0f);

  if (camera.getIsOrthographic()) {
    ubo.proj = glm::ortho(0.0f, (float)width, 0.0f, (float)height, 0.1f, 10.0f);
  } else {
    ubo.proj = glm::perspective(glm::radians(110.0f), width / (float)height,
                                0.1f, 10.0f);
  }
  ubo.view = camera.getViewMatrix();
  ubo.proj[1][1] *= -1;

  void *data;
  vmaMapMemory(allocator, uniformBuffers[index].allocation, &data);
  memcpy(data, &ubo, sizeof(ubo));
  vmaUnmapMemory(allocator, uniformBuffers[index].allocation);
}

glm::mat4 Model::getMatrix() {
  auto qY = glm::angleAxis(yAngle, glm::vec3(1, 0, 0));
  auto qX = glm::angleAxis(xAngle, glm::vec3(0, 0, 1));
  auto qZ = glm::angleAxis(zAngle, glm::vec3(0, 1, 0));
  glm::quat rotation = qY * qX * qZ;
  rotation = glm::normalize(rotation);
  return glm::translate(
      glm::scale(glm::mat4(1), scale) * glm::mat4_cast(rotation), position);
}

void Model::translate(glm::vec3 vector) { position += vector; }

void Model::rotate(glm::vec3 radians) {
  xAngle += radians.x;
  yAngle += radians.y;
  zAngle += radians.z;
}

const glm::vec3 &Model::getPosition() const { return position; }

const glm::vec3 &Model::getScale() const { return scale; }

float Model::getXAngle() const { return xAngle; }

float Model::getYAngle() const { return yAngle; }

float Model::getZAngle() const { return zAngle; }

const std::string &Model::getName() const { return name; }

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

const std::vector<BufferAlloc> &Model::getUniformBuffers() const {
  return uniformBuffers;
}

uint32_t Model::getIndexCount() const { return indexCount; }

DescriptorSet Model::getDescriptorSet() const { return descriptorSet; }

} // namespace Infinite