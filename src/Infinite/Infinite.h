#ifndef VULKAN_INFINITE_H
#define VULKAN_INFINITE_H
#pragma once

#include "util/Includes.h"
#include "backend/Settings.h"

namespace Infinite {
    class RenderPass;

    extern VkPhysicalDevice physicalDevice;
    extern VkDevice device;
//    extern Settings * settings;
    extern VkFormat swapChainImageFormat;
    extern VmaAllocator allocator;
    extern VkCommandPool imagePool;
    extern std::vector<RenderPass *> renderPasses; //ToDo: CleanUp


    void initInfinite();

    void addRenderPass(RenderPass *r);

    void recordCommandBuffer(VkCommandBuffer commandBuffer, uint32_t imageIndex, RenderPass &renderPass);

    void recreateSwapChain();

    void renderFrame();

    void waitForNextFrame();

    VkShaderModule createShaderModule(const std::vector<char> &code);

    void cleanUp();
}

#endif //VULKAN_INFINITE_H
