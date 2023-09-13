#ifndef VULKAN_IMAGE_H
#define VULKAN_IMAGE_H

#pragma once
#include "../../util/Includes.h"
#include "../../util/VulkanUtils.h"
#include "../Rendering/RenderPasses/RenderPass.h"


namespace Infinite {

    class Image {
    protected:
        VkImageView _image_view;

        static void copyBufferToImage(BufferAlloc buffer, Image* image, uint32_t width, uint32_t height);

        static void transitionImageLayout(Image* image, VkFormat format, VkImageLayout oldLayout, VkImageLayout newLayout, uint32_t mipLevels = 1);

        static void createImage(uint32_t width, uint32_t height, uint32_t mipLevels, VkFormat format, VkImageTiling tiling, VkImageUsageFlags usage,
                                ImageAlloc &image, VkSampleCountFlagBits numSamples=VK_SAMPLE_COUNT_1_BIT);

        static VkImageView createImageView(VkImage image, VkFormat format, uint32_t mipLevels, VkImageAspectFlags aspectFlags = VK_IMAGE_ASPECT_COLOR_BIT);

    public:
        virtual void create();

        friend class SwapChain;

        virtual void destroy(VmaAllocator allocator);

        ImageAlloc _image;
    };

} // Infinite

#endif //VULKAN_IMAGE_H
