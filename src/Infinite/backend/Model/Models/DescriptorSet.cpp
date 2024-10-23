#include "DescriptorSet.h"
#include "../../../util/constants.h"
#include "Model.h"
#include <cstdint>
#include <iostream>
#include <ostream>
#include <stdexcept>
#include <vector>
#include <vulkan/vulkan_core.h>

namespace Infinite {

DescriptorSet::DescriptorSet(){};
DescriptorSet::DescriptorSet(VkDevice device,
                             VkDescriptorSetLayout *descriptorSetLayout,
                             ShaderLayout *_shaderLayout) {
  shaderLayout = _shaderLayout;
  std::vector<VkDescriptorSetLayout> layouts(MAX_FRAMES_IN_FLIGHT,
                                             *descriptorSetLayout);

  VkDescriptorSetAllocateInfo allocInfo{};
  allocInfo.sType = VK_STRUCTURE_TYPE_DESCRIPTOR_SET_ALLOCATE_INFO;
  allocInfo.descriptorPool = descriptorPool;
  allocInfo.descriptorSetCount = static_cast<uint32_t>(MAX_FRAMES_IN_FLIGHT);
  allocInfo.pSetLayouts = layouts.data();

  descriptorSets.resize(MAX_FRAMES_IN_FLIGHT);

  if (vkAllocateDescriptorSets(device, &allocInfo, descriptorSets.data()) !=
      VK_SUCCESS) {
    throw std::runtime_error("failed to allocate descriptor sets!");
  }
}
void DescriptorSet::createDescriptorPool(VkDevice device) {
  std::array<VkDescriptorPoolSize, 4>
      poolSizes{}; // TODO: implement a way to resize this thing
  poolSizes[0].type = VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER;
  poolSizes[0].descriptorCount =
      static_cast<uint32_t>(MAX_FRAMES_IN_FLIGHT * MAX_MODELS);
  poolSizes[1].type = VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER;
  poolSizes[1].descriptorCount =
      static_cast<uint32_t>(MAX_FRAMES_IN_FLIGHT * MAX_MODELS);

  poolSizes[2].type = VK_DESCRIPTOR_TYPE_STORAGE_BUFFER;
  poolSizes[2].descriptorCount =
      static_cast<uint32_t>(MAX_FRAMES_IN_FLIGHT) * 2;
  poolSizes[3].type = VK_DESCRIPTOR_TYPE_STORAGE_IMAGE;
  poolSizes[3].descriptorCount =
      static_cast<uint32_t>(MAX_FRAMES_IN_FLIGHT) * 2;

  VkDescriptorPoolCreateInfo poolInfo{};
  poolInfo.sType = VK_STRUCTURE_TYPE_DESCRIPTOR_POOL_CREATE_INFO;
  poolInfo.poolSizeCount = static_cast<uint32_t>(poolSizes.size());
  poolInfo.pPoolSizes = poolSizes.data();
  poolInfo.flags =
      VK_DESCRIPTOR_POOL_CREATE_FREE_DESCRIPTOR_SET_BIT; // Include the flag
  poolInfo.maxSets = static_cast<uint32_t>(MAX_FRAMES_IN_FLIGHT * MAX_MODELS);

  if (vkCreateDescriptorPool(device, &poolInfo, nullptr, &descriptorPool) !=
      VK_SUCCESS) {
    throw std::runtime_error("failed to create descriptor pool!");
  }
}

VkDescriptorSetLayout DescriptorSet::createDescriptorSetLayout(
    VkDevice device, std::vector<DescriptorSetLayout> layout) {
  VkDescriptorSetLayout descriptorSetLayout;
  std::vector<VkDescriptorSetLayoutBinding> bindings;
  for (uint32_t i = 0; i < layout.size(); i++) {
    VkDescriptorSetLayoutBinding layoutBinding{};
    layoutBinding.binding = i;
    layoutBinding.descriptorCount = layout[i].descriptorCount;
    layoutBinding.descriptorType = layout[i].descriptorType;
    layoutBinding.pImmutableSamplers = nullptr;
    layoutBinding.stageFlags = layout[i].stageFlags;
    bindings.push_back(layoutBinding);
  }

  // VkDescriptorSetLayoutBinding uboLayoutBinding{};
  // uboLayoutBinding.binding = 0;
  // uboLayoutBinding.descriptorCount = 1;
  // uboLayoutBinding.descriptorType = VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER;
  // uboLayoutBinding.pImmutableSamplers = nullptr;
  // uboLayoutBinding.stageFlags = VK_SHADER_STAGE_VERTEX_BIT; // change

  // VkDescriptorSetLayoutBinding samplerLayoutBinding{};
  // samplerLayoutBinding.binding = 1;
  // samplerLayoutBinding.descriptorCount = 1;
  // samplerLayoutBinding.descriptorType =
  //     VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER;
  // samplerLayoutBinding.pImmutableSamplers = nullptr;
  // samplerLayoutBinding.stageFlags = VK_SHADER_STAGE_FRAGMENT_BIT;

  VkDescriptorSetLayoutCreateInfo layoutInfo{};
  layoutInfo.sType = VK_STRUCTURE_TYPE_DESCRIPTOR_SET_LAYOUT_CREATE_INFO;
  layoutInfo.bindingCount = static_cast<uint32_t>(bindings.size());
  layoutInfo.pBindings = bindings.data();

  if (vkCreateDescriptorSetLayout(device, &layoutInfo, nullptr,
                                  &descriptorSetLayout) != VK_SUCCESS) {
    throw std::runtime_error("failed to create descriptor set layout!");
  }
  return descriptorSetLayout;
}

const std::vector<VkDescriptorSet> &DescriptorSet::getDescriptorSets() const {
  return descriptorSets;
}

void DescriptorSet::createDescriptorSets(
    VkDevice device,
    std::vector<std::vector<VkBuffer>>
        buffers, // need one buffer per frame in flight and one for each
                 // descriptor set
    std::vector<std::vector<VkDeviceSize>> bufferSizes,
    std::vector<VkImageView> imageViews, std::vector<VkSampler> samplers) {

  for (size_t i = 0; i < MAX_FRAMES_IN_FLIGHT; i++) {
    std::vector<VkWriteDescriptorSet> descriptorWrites{};
    size_t bufferIndex = 0;
    size_t imageIndex = 0;

    for (int j = 0; j < shaderLayout->highLevelLayout.size(); j++) {
      switch (shaderLayout->highLevelLayout[j].descriptorType) {
      case VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER: {
        VkDescriptorBufferInfo bufferInfo{};
        bufferInfo.buffer = buffers[i][bufferIndex];
        bufferInfo.offset = 0;
        bufferInfo.range = bufferSizes[i][bufferIndex++];

        VkWriteDescriptorSet descriptorWrite{};
        descriptorWrite.sType = VK_STRUCTURE_TYPE_WRITE_DESCRIPTOR_SET;
        descriptorWrite.dstSet = descriptorSets[i];
        descriptorWrite.dstBinding = j;
        descriptorWrite.dstArrayElement = 0;
        descriptorWrite.descriptorType = VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER;
        descriptorWrite.descriptorCount = 1;
        descriptorWrite.pBufferInfo = &bufferInfo;
        descriptorWrites.push_back(descriptorWrite);
        break;
      }
      case VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER: {
        VkDescriptorImageInfo imageInfo{};
        imageInfo.imageLayout = VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL;
        imageInfo.imageView = imageViews[imageIndex];
        imageInfo.sampler = samplers[imageIndex++];

        VkWriteDescriptorSet descriptorWrite{};
        descriptorWrite.sType = VK_STRUCTURE_TYPE_WRITE_DESCRIPTOR_SET;
        descriptorWrite.dstSet = descriptorSets[i];
        descriptorWrite.dstBinding = j;
        descriptorWrite.dstArrayElement = 0;
        descriptorWrite.descriptorType =
            shaderLayout->highLevelLayout[j].descriptorType;
        descriptorWrite.descriptorCount = 1;
        descriptorWrite.pImageInfo = &imageInfo;
        descriptorWrites.push_back(descriptorWrite);
        break;
      }
      case VK_DESCRIPTOR_TYPE_STORAGE_BUFFER: {
        VkDescriptorBufferInfo storageBufferInfoCurrentFrame{};
        storageBufferInfoCurrentFrame.buffer = buffers[i][bufferIndex];
        storageBufferInfoCurrentFrame.offset = 0;
        storageBufferInfoCurrentFrame.range = bufferSizes[i][bufferIndex++];

        VkWriteDescriptorSet descriptorWrite;
        descriptorWrite.sType = VK_STRUCTURE_TYPE_WRITE_DESCRIPTOR_SET;
        descriptorWrite.dstSet = descriptorSets[i];
        descriptorWrite.dstBinding = j;
        descriptorWrite.dstArrayElement = 0;
        descriptorWrite.descriptorType = VK_DESCRIPTOR_TYPE_STORAGE_BUFFER;
        descriptorWrite.descriptorCount = 1;
        descriptorWrite.pBufferInfo = &storageBufferInfoCurrentFrame;

        descriptorWrites.push_back(descriptorWrite);
        break;
      }
      case VK_DESCRIPTOR_TYPE_STORAGE_IMAGE: {
        VkDescriptorImageInfo imageInfo{};
        imageInfo.imageLayout = VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL;
        imageInfo.imageView = imageViews[imageIndex];
        imageInfo.imageLayout = VK_IMAGE_LAYOUT_GENERAL;

        VkWriteDescriptorSet descriptorWrite{};
        descriptorWrite.sType = VK_STRUCTURE_TYPE_WRITE_DESCRIPTOR_SET;
        descriptorWrite.dstSet = descriptorSets[i];
        descriptorWrite.dstBinding = j;
        descriptorWrite.dstArrayElement = 0;
        descriptorWrite.descriptorType =
            shaderLayout->highLevelLayout[j].descriptorType;
        descriptorWrite.descriptorCount = 1;
        descriptorWrite.pImageInfo = &imageInfo;
        descriptorWrites.push_back(descriptorWrite);
        break;
      }
      default:
        throw std::runtime_error(
            "Unknown descriptor type when createing descriptor sets");
        break;
      }
    }

    for (size_t j = 0; j < descriptorWrites.size(); j++) {
      std::vector<VkWriteDescriptorSet> descriptorWrite1 = {
          descriptorWrites[j]};
      vkUpdateDescriptorSets(device, 1, descriptorWrite1.data(), 0, nullptr);
    }
    // vkUpdateDescriptorSets(device,
    //                        static_cast<uint32_t>(descriptorWrites.size()),
    //                        descriptorWrites.data(), 0, nullptr);
  }
}

VkDescriptorPool &DescriptorSet::getDescriptorPool() { return descriptorPool; }

void DescriptorSet::cleanUpDescriptors(VkDevice device) {
  vkDestroyDescriptorPool(device, descriptorPool, nullptr);
}
void DescriptorSet::destroy(VkDevice device) {
  vkFreeDescriptorSets(device, descriptorPool, MAX_FRAMES_IN_FLIGHT,
                       descriptorSets.data());
}
} // namespace Infinite