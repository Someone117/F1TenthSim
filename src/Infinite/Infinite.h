#ifndef VULKAN_INFINITE_H
#define VULKAN_INFINITE_H

#include "../../util/Includes.h"

namespace Infinite {
    class RenderPass;

    std::vector<RenderPass *> renderPasses; //ToDo: CleanUp

    void startInitInfinite();
    void finishInitInfinite();

    // Calls everything from startInitInfinite and finishInitInfinite
    void initInfinite();
    void addRenderPass(RenderPass *r);
    static void recordCommandBuffer(VkCommandBuffer commandBuffer, uint32_t imageIndex, RenderPass renderPass);
    void recreateSwapChain();
    void renderFrame();
    void waitForNextFrame();
}

#endif //VULKAN_INFINITE_H
