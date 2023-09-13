#ifndef VULKAN_RENDERPASS_H
#define VULKAN_RENDERPASS_H
#pragma once


#include "../CommandBuffer.h"
#include "../../Model/Image/DepthImage.h"
#include "../../Model/Image/ColorImage.h"

namespace Infinite {

    class Model;

    class ColorImage;

    class DepthImage;

    extern std::vector<VkSemaphore> imageAvailableSemaphores;
    extern std::vector<VkSemaphore> renderFinishedSemaphores;
    extern std::vector<VkFence> inFlightFences;
    extern VkQueue presentQueue;


    class RenderPass {
        friend void recordCommandBuffer(VkCommandBuffer commandBuffer, uint32_t imageIndex, RenderPass &renderPass);

        friend void renderFrame();

    public:


        void destroyDepthAndColorImages();

        const std::vector<Model *> &getModels() const;

        virtual void createRenderPass();

        void createCommandBuffers();

        static void createSyncObjects();

        uint32_t getIndex() const;

        void setIndex(uint32_t index);

        void addModel(Model *model);

        static void createCommandPool(VkCommandPool &pool);

        void createGraphicsPipeline();

        void createDepthAndColorImages();

        ColorImage *getColorImageReasource() const;

        DepthImage *getDepthImageReasource() const;

        void destroy();

        VkRenderPass_T *getRenderPass();


    protected:


        VkRenderPass renderPass;

        ColorImage *colorImageReasource;
        DepthImage *depthImageReasource;

        VkPipelineLayout pipelineLayout;
        VkPipeline graphicsPipeline;

        VkCommandPool commandPool;

        std::vector<Model *> models;

        uint32_t index;

        friend struct CommandBuffer;

        friend class Image;

        CommandBuffer commandBufferManager;
    };

} // Infinite

#endif //VULKAN_RENDERPASS_H