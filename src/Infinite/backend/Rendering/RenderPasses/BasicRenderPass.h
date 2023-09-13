#ifndef VULKAN_BASICRENDERPASS_H
#define VULKAN_BASICRENDERPASS_H

#pragma once

#include "RenderPass.h"

namespace Infinite {

    class BasicRenderPass : public RenderPass {
    public:
        void createRenderPass() override;

    };

} // Infinite

#endif //VULKAN_BASICRENDERPASS_H
