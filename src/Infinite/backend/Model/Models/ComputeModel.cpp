#include "ComputeModel.h"
#include "../../../Infinite.h"
#include "../../../util/constants.h"
#include "BaseModel.h"
#include <cstring>
#include <ctime>
#include <iostream>
#include <ostream>
#include <random>

namespace Infinite {

ComputeModel::ComputeModel(uint32_t WIDTH, uint32_t HEIGHT,
                           VmaAllocator allocator)
    : BaseModel("Compute Model"), texture(NULL) {

  texture.create(WIDTH, HEIGHT, swapChainImageFormat, physicalDevice,
                 allocator);

  uniformBuffers2.resize(MAX_FRAMES_IN_FLIGHT);

  for (size_t i = 0; i < MAX_FRAMES_IN_FLIGHT; i++) {
    createBuffer(sizeof(ComputeUniformBufferObject),
                 VK_BUFFER_USAGE_UNIFORM_BUFFER_BIT,
                 reinterpret_cast<BufferAlloc &>(uniformBuffers2[i]),
                 VK_MEMORY_PROPERTY_HOST_VISIBLE_BIT |
                     VK_MEMORY_PROPERTY_HOST_COHERENT_BIT,
                 VMA_ALLOCATION_CREATE_MAPPED_BIT |
                     VMA_ALLOCATION_CREATE_HOST_ACCESS_RANDOM_BIT);
  }

  std::vector<Vertex> vertices;
  std::vector<uint32_t> indices;

  loadModel(vertices, indices, R"(../assets/untitled.obj)");

  indexCount = indices.size();

  createVertexBuffer(vertexBuffer, vertices, allocator,
                     queues[static_cast<uint32_t>(QueueOrder::GRAPHICS)].queue);

  createIndexBuffer(indexBuffer, indices, allocator,
                    queues[static_cast<uint32_t>(QueueOrder::GRAPHICS)].queue);

  shaderStorageBufferAlloc.resize(MAX_FRAMES_IN_FLIGHT);
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
                   VK_MEMORY_PROPERTY_HOST_COHERENT_BIT,
               VMA_ALLOCATION_CREATE_HOST_ACCESS_SEQUENTIAL_WRITE_BIT);

  void *data;
  vmaMapMemory(allocator, stagingBufferAlloc.allocation, &data);
  memcpy(data, particles.data(), (size_t)bufferSize);
  vmaUnmapMemory(allocator, stagingBufferAlloc.allocation);

  for (size_t i = 0; i < MAX_FRAMES_IN_FLIGHT; i++) {
    createBuffer(
        bufferSize,
        VK_BUFFER_USAGE_STORAGE_BUFFER_BIT | VK_BUFFER_USAGE_VERTEX_BUFFER_BIT |
            VK_BUFFER_USAGE_TRANSFER_DST_BIT,
        shaderStorageBufferAlloc[i], VK_MEMORY_PROPERTY_DEVICE_LOCAL_BIT);
    // Copy data from the staging buffer (host) to the shader storage buffer
    // (GPU)
    copyBuffer(stagingBufferAlloc, shaderStorageBufferAlloc[i], bufferSize,
               queues[static_cast<uint32_t>(QueueOrder::COMPUTE)].queue);
  }
  vmaDestroyBuffer(allocator, stagingBufferAlloc.buffer,
                   stagingBufferAlloc.allocation);
}

void ComputeModel::createDescriptorSets2(
    VkDevice device, VkDescriptorSetLayout *descriptorSetLayout,
    ShaderLayout *shaderLayout) {
  descriptorSet2 = DescriptorSet(device, descriptorSetLayout, shaderLayout);
  std::vector<std::vector<VkBuffer>> tempBuffers;
  std::vector<std::vector<VkDeviceSize>> tempBufferSizes;
  for (size_t i = 0; i < MAX_FRAMES_IN_FLIGHT; i++) {
    std::vector<VkBuffer> temp1;
    std::vector<VkDeviceSize> temp2;

    temp1.push_back({uniformBuffers2[i].buffer});
    temp2.push_back({sizeof(ComputeUniformBufferObject)});
    temp1.push_back(
        {shaderStorageBufferAlloc[(i - 1) % MAX_FRAMES_IN_FLIGHT].buffer});
    temp2.push_back({sizeof(Particle) * PARTICLE_COUNT});
    temp1.push_back({shaderStorageBufferAlloc[i].buffer});
    temp2.push_back({sizeof(Particle) * PARTICLE_COUNT});
    tempBuffers.push_back(temp1);
    tempBufferSizes.push_back(temp2);
  }

  descriptorSet2.createDescriptorSets(device, tempBuffers, tempBufferSizes,
                                      {*texture.getImageView()}, {});
}

void ComputeModel::createDescriptorSets(
    VkDevice device, VkDescriptorSetLayout *descriptorSetLayout,
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

void ComputeModel::destroy(VkDevice device, VmaAllocator allocator) {
  BaseModel::destroy(device, allocator);
  for (uint32_t i = 0; i < shaderStorageBufferAlloc.size(); i++) {
    vmaDestroyBuffer(allocator, shaderStorageBufferAlloc[i].buffer,
                     shaderStorageBufferAlloc[i].allocation);
  }
  for (uint32_t i = 0; i < uniformBuffers2.size(); i++) {
    vmaDestroyBuffer(allocator, uniformBuffers2[i].buffer,
                     uniformBuffers2[i].allocation);
  }

  vmaDestroyBuffer(allocator, vertexBuffer.buffer, vertexBuffer.allocation);
  vmaDestroyBuffer(allocator, indexBuffer.buffer, indexBuffer.allocation);
  texture.destroy(allocator);
  descriptorSet2.destroy(device);
}

const BufferAlloc &ComputeModel::getVertexBuffer() const {
  return vertexBuffer;
}

const BufferAlloc &ComputeModel::getIndexBuffer() const { return indexBuffer; }

void ComputeModel::updateUniformBuffer(uint32_t index, Camera camera,
                                       VmaAllocator allocator,
                                       unsigned int width,
                                       unsigned int height) {
  ComputeUniformBufferObject ubo{};
  ubo.deltaTime = lastFrameTime * 2.0f;

  void *data;
  vmaMapMemory(allocator, uniformBuffers2[index].allocation, &data);
  memcpy(data, &ubo, sizeof(ubo));
  vmaUnmapMemory(allocator, uniformBuffers2[index].allocation);

  // the ubo2 corresponds with the UBO 1 obejct
  UniformBufferObject ubo2{};
  ubo2.model = glm::mat4(1.0f);

  if (camera.getIsOrthographic()) {
    ubo2.proj =
        glm::ortho(0.0f, (float)width, 0.0f, (float)height, 0.1f, 10.0f);
  } else {
    ubo2.proj = glm::perspective(glm::radians(110.0f), width / (float)height,
                                 0.01f, 10.0f);
  }
  ubo2.view = camera.getViewMatrix();
  ubo2.proj[1][1] *= -1;

  void *data2;
  vmaMapMemory(allocator, uniformBuffers[index].allocation, &data2);
  memcpy(data2, &ubo2, sizeof(ubo2));
  vmaUnmapMemory(allocator, uniformBuffers[index].allocation);
}
} // namespace Infinite