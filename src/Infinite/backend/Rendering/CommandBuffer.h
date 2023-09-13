//
// Created by Someo on 5/11/2023.
//

#ifndef VULKAN_COMMANDBUFFER_H
#define VULKAN_COMMANDBUFFER_H
#include "../../util/Includes.h"
#include "RenderPasses/RenderPass.h"

namespace Infinite {

    class CommandBuffer {
    private:
        std::vector<VkCommandBuffer> commandBuffers;
        static void recordCommandBuffer(VkCommandBuffer commandBuffer, uint32_t imageIndex, RenderPass renderPass);
    public:
        void create();
    };

} // Infinite

#endif //VULKAN_COMMANDBUFFER_H
