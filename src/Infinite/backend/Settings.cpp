#pragma once

#include "Settings.h"

namespace Infinite {
    VkSampleCountFlagBits msaaSamples = VK_SAMPLE_COUNT_1_BIT;
    uint32_t mipLevels;
//
//    Settings::Settings() = default;
//
//    void Settings::setMsaaSamples(VkSampleCountFlagBits msaaSamples) {
//        Settings::msaaSamples = msaaSamples;
//    }
}