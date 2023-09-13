//
// Created by Someo on 9/12/2023.
//

#ifndef VULKAN_DESCRIPTORSET_H
#define VULKAN_DESCRIPTORSET_H


#include <vulkan/vulkan_core.h>
#include <vector>

namespace Infinite {

    class DescriptorSet {

        inline static VkDescriptorPool descriptorPool;

        std::vector<VkDescriptorSet> descriptorSets;

        void createDescriptorSets();

        inline static VkDescriptorSetLayout descriptorSetLayout;

        static void createDescriptorPool(VkDevice device);

        static void createDescriptorSetLayout(VkDevice device);


    public:
        const std::vector<VkDescriptorSet> &getDescriptorSets() const;

        DescriptorSet(VkDevice device, std::vector<BufferAlloc> uniformBuffers, VkImageView textureView, VkSampler textureSampler);
    };

} // Infinite

#endif //VULKAN_DESCRIPTORSET_H
