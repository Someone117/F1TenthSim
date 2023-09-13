#ifndef VULKAN_DEPTHIMAGE_H
#define VULKAN_DEPTHIMAGE_H

#pragma once
#include "../../util/Includes.h"
#include "../../util/VulkanUtils.h"
#include "Image.h"

namespace Infinite {

    class DepthImage: public Image {
    public:
        static VkFormat findDepthFormat();

        void create() override;
    };

} // Infinite

#endif //VULKAN_DEPTHIMAGE_H
