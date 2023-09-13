//
// Created by Someo on 5/11/2023.
//

#ifndef VULKAN_RENDERPASS_H
#define VULKAN_RENDERPASS_H

#include "../../../util/Includes.h"
#include "../CommandBuffer.h"

namespace Infinite {

    class RenderPass {
    private:
        friend class Engine;
        friend class CommandBuffer;
        CommandBuffer commandBuffers;
        VkRenderPass renderPass;

        void create();
    };

} // Infinite

#endif //VULKAN_RENDERPASS_H
