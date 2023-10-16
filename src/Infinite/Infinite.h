#ifndef VULKAN_INFINITE_H
#define VULKAN_INFINITE_H

#include "backend/Model/Models/Model.h"
#include "backend/Settings.h"
#include "util/Includes.h"

namespace Infinite {
class RenderPass;

extern VkPhysicalDevice physicalDevice;
extern VkDevice device;
//    extern Settings * settings;
extern VkFormat swapChainImageFormat;
extern VmaAllocator allocator;
extern VkCommandPool imagePool;
extern std::vector<RenderPass *> renderPasses; // ToDo: CleanUp

void initInfinite();

void addRenderPass(RenderPass *r);

void recreateSwapChain();

void recordCommandBuffer(VkCommandBuffer commandBuffer, uint32_t imageIndex,
                         RenderPass &renderPass, uint32_t currentFrame);

void renderFrame();

void waitForNextFrame();

void cleanUp();

static void createCommandPool(VkCommandPool &pool, VkDevice device,
                              QueueFamilyIndices queueFamilyIndices);

Model createModel(std::string _name, std::string model_path,
                  const char *texture_path);
} // namespace Infinite

#endif // VULKAN_INFINITE_H
