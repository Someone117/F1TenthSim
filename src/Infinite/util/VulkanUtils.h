//
// Created by Someo on 4/7/2023.
//

#ifndef VULKAN_VULKANUTILS_H
#define VULKAN_VULKANUTILS_H

#pragma once
#include "Includes.h"
#include <optional>
#include <array>


namespace Infinite {

    struct BufferAlloc {
        VkBuffer buffer;
        VmaAllocation allocation;
    };
    struct ImageAlloc {
        VkImage image;
        VmaAllocation allocation;
    };

    struct Vertex {
        glm::vec3 pos;
        glm::vec2 texCoord;

        Vertex(const glm::vec3 &pos, const glm::vec2 &texCoord) : pos(pos), texCoord(texCoord) {}

        static VkVertexInputBindingDescription getBindingDescription();

        static std::array<VkVertexInputAttributeDescription, 2> getAttributeDescriptions();
    };

    struct UniformBufferObject {
        alignas(16) glm::mat4 model;
        alignas(16) glm::mat4 view;
        alignas(16) glm::mat4 proj;
    };



    VmaAllocator allocator;
    void createBuffer(VkDeviceSize size, VkBufferUsageFlags usage, BufferAlloc &bufferAllocator, VkMemoryPropertyFlags memFlags = 0, VmaAllocationCreateFlags vmaFlags = 0);
    void createImage(uint32_t width, uint32_t height, uint32_t mipLevels, VkFormat format, VkImageTiling tiling, VkImageUsageFlags usage,
                     ImageAlloc &image, VkSampleCountFlagBits numSamples=VK_SAMPLE_COUNT_1_BIT);
    bool hasStencilComponent(VkFormat format);

    std::vector<char> readFile(const std::string &filename);


    bool checkDeviceExtensionSupport(VkPhysicalDevice device);
    std::vector<const char *> getRequiredExtensions();

    VkShaderModule createShaderModule(const std::vector<char> &code);

    VkCommandBuffer beginSingleTimeCommands();
    void endSingleTimeConnands(VkCommandBuffer commandBuffer);

    void cleanup();
}


#endif //VULKAN_VULKANUTILS_H
