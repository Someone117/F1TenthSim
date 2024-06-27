#ifndef VULKAN_SETTINGS_H
#define VULKAN_SETTINGS_H

#include <cstdint>
#include <vulkan/vulkan_core.h>
#pragma once



namespace Infinite {
    const uint32_t WIDTH = 800;
    const uint32_t HEIGHT = 600;
    extern VkSampleCountFlagBits msaaSamples;
    extern uint32_t mipLevels;

    static VkPresentModeKHR PRESENT_MODE = VK_PRESENT_MODE_IMMEDIATE_KHR; // VK_PRESENT_MODE_MAILBOX_KHR;
}


#endif //VULKAN_SETTINGS_H
