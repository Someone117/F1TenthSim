#pragma once

#include "Infinite.h"
#include "backend/Rendering/Engine.h"
#include "backend/Rendering/RenderPasses/RenderPass.h"
#include "backend/Model/Models/Model.h"

namespace Infinite {
    VkPhysicalDevice physicalDevice;
    VkDevice device;
//    Settings * settings;
    VkFormat swapChainImageFormat;
    VmaAllocator allocator;
    VkCommandPool imagePool;
    std::vector<RenderPass *> renderPasses; //ToDo: CleanUp

    void initInfinite() {
        Engine::getEngine().createEngine();


        RenderPass::createCommandPool(imagePool);

        for (const auto &r: renderPasses) {
            r->createRenderPass();
        }

        Model::createDescriptorSetLayout();

        for (const auto &r: renderPasses) {
            r->createGraphicsPipeline();

            r->createDepthAndColorImages();
            Engine::getEngine().createFramebuffers(*r, renderPasses.size());
        }

        Model::createDescriptorPool();

        for (const auto &r: renderPasses) {
            r->createCommandBuffers();
        }
        RenderPass::createSyncObjects();
    }

    void addRenderPass(RenderPass *r) {
        renderPasses.push_back(r);
        r->setIndex(renderPasses.size() - 1);
    }

    void recreateSwapChain() {
        Engine::getEngine().recreateSwapChain();
        for (const auto &r: renderPasses) {
            r->destroyDepthAndColorImages();
            r->createDepthAndColorImages();
            Engine::getEngine().createFramebuffers(*r, renderPasses.size());
        }
    }

    void recordCommandBuffer(VkCommandBuffer commandBuffer, uint32_t imageIndex, RenderPass &renderPass) {
        VkCommandBufferBeginInfo beginInfo{};
        beginInfo.sType = VK_STRUCTURE_TYPE_COMMAND_BUFFER_BEGIN_INFO;
        beginInfo.flags = 0; // Optional
        beginInfo.pInheritanceInfo = nullptr; // Optional

        if (vkBeginCommandBuffer(commandBuffer, &beginInfo) != VK_SUCCESS) {
            throw std::runtime_error("failed to begin recording command buffer!");
        }

        VkRenderPassBeginInfo renderPassInfo{};
        renderPassInfo.sType = VK_STRUCTURE_TYPE_RENDER_PASS_BEGIN_INFO;
        renderPassInfo.renderPass = renderPass.renderPass;
        renderPassInfo.framebuffer = Engine::getEngine().swapChainFramebuffers[renderPass.getIndex()][imageIndex]; //ToDo check
        renderPassInfo.renderArea.offset = {0, 0};
        renderPassInfo.renderArea.extent = Engine::getEngine().swapChainExtent;

        std::array<VkClearValue, 2> clearValues{};
        clearValues[0].color = {{0.0f, 0.0f, 0.0f, 1.0f}};
        clearValues[1].depthStencil = {1.0f, 0};

        renderPassInfo.clearValueCount = static_cast<uint32_t>(clearValues.size());
        renderPassInfo.pClearValues = clearValues.data();

        vkCmdBeginRenderPass(commandBuffer, &renderPassInfo, VK_SUBPASS_CONTENTS_INLINE);

        //Commands
        vkCmdBindPipeline(commandBuffer, VK_PIPELINE_BIND_POINT_GRAPHICS, renderPass.graphicsPipeline);

        VkViewport viewport{};
        viewport.x = 0.0f;
        viewport.y = 0.0f;
        viewport.width = static_cast<float>(Engine::getEngine().getWindowWidth());
        viewport.height = static_cast<float>(Engine::getEngine().getWindowHeight());
        viewport.minDepth = 0.0f;
        viewport.maxDepth = 1.0f;
        vkCmdSetViewport(commandBuffer, 0, 1, &viewport);

        VkRect2D scissor{};
        scissor.offset = {0, 0};
        scissor.extent = Engine::getEngine().swapChainExtent;
        vkCmdSetScissor(commandBuffer, 0, 1, &scissor);

        for (int i = 0; i < renderPass.getModels().size(); i++) {
            BufferAlloc vertexBuffers[] = {renderPass.getModels()[i]->getVertexBuffer()};
            VkDeviceSize offsets[] = {0};
            vkCmdBindVertexBuffers(commandBuffer, 0, 1, &vertexBuffers->buffer, offsets);

            vkCmdBindIndexBuffer(commandBuffer, renderPass.getModels()[i]->getIndexBuffer().buffer, 0,
                                 VK_INDEX_TYPE_UINT32);

            vkCmdBindDescriptorSets(commandBuffer, VK_PIPELINE_BIND_POINT_GRAPHICS, renderPass.pipelineLayout, 0, 1,
                                    &renderPass.getModels()[i]->getDescriptorSets()[Engine::getEngine().currentFrame],
                                    0, nullptr);

            vkCmdDrawIndexed(commandBuffer,
                             static_cast<uint32_t>(renderPass.getModels()[i]->getIndexCount()), 1, 0,
                             0,
                             0);
        }
        //End Commands

        vkCmdEndRenderPass(commandBuffer);

        if (vkEndCommandBuffer(commandBuffer) != VK_SUCCESS) {
            throw std::runtime_error("failed to record command buffer!");
        }
    }


    void renderFrame() {
        vkWaitForFences(device, 1,
                        &inFlightFences[Engine::getEngine().getCurrentFrame()],
                        VK_TRUE, UINT64_MAX);

        bool needToRecreateSwapChain = false;

        uint32_t imageIndex;
        VkResult result = vkAcquireNextImageKHR(device,
                                                Engine::getEngine().getSwapChain(), UINT64_MAX,
                                                imageAvailableSemaphores[Engine::getEngine().getCurrentFrame()],
                                                VK_NULL_HANDLE,
                                                &imageIndex);


        if (result == VK_ERROR_OUT_OF_DATE_KHR || result == VK_SUBOPTIMAL_KHR) {
            needToRecreateSwapChain = true;
        }
        std::vector<VkSubmitInfo> submitInformation(renderPasses.size());
        for (const auto &r: renderPasses) {
            for (Model *model: r->getModels()) {
                model->updateUniformBuffer(Engine::getEngine().getCurrentFrame(), *cameras[r->getIndex()]);
            }
        }

        vkResetFences(device, 1,
                      &inFlightFences[Engine::getEngine().getCurrentFrame()]);

        for (size_t i = 0; i < renderPasses.size(); i++) {
            vkResetCommandBuffer(
                    renderPasses[i]->commandBufferManager.commandBuffers[Engine::getEngine().getCurrentFrame()], /*VkCommandBufferResetFlagBits*/
                    0);

            Infinite::recordCommandBuffer(renderPasses[i]->commandBufferManager.commandBuffers[Engine::getEngine().getCurrentFrame()],
                                          renderPasses[i]->index, *renderPasses[i]);

            submitInformation[i].sType = VK_STRUCTURE_TYPE_SUBMIT_INFO;

            VkSemaphore waitSemaphores[] = {imageAvailableSemaphores[Engine::getEngine().getCurrentFrame()]};
            VkPipelineStageFlags waitStages[] = {VK_PIPELINE_STAGE_COLOR_ATTACHMENT_OUTPUT_BIT};
            submitInformation[i].waitSemaphoreCount = 1;
            submitInformation[i].pWaitSemaphores = waitSemaphores;
            submitInformation[i].pWaitDstStageMask = waitStages;

            submitInformation[i].commandBufferCount = 1;
            submitInformation[i].pCommandBuffers = &renderPasses[i]->commandBufferManager.commandBuffers[Engine::getEngine().getCurrentFrame()];

            VkSemaphore signalSemaphores[] = {renderFinishedSemaphores[Engine::getEngine().getCurrentFrame()]}; //Todo: is this necessary
            submitInformation[i].signalSemaphoreCount = 1;
            submitInformation[i].pSignalSemaphores = signalSemaphores;
        }

        if (vkQueueSubmit(Engine::getEngine().getGraphicsQueue(), submitInformation .size(), submitInformation.data(),
                          inFlightFences[Engine::getEngine().getCurrentFrame()]) != VK_SUCCESS) {
            throw std::runtime_error("failed to submit draw command buffer!");
        }

        VkPresentInfoKHR presentInfo{};
        presentInfo.sType = VK_STRUCTURE_TYPE_PRESENT_INFO_KHR;

        presentInfo.waitSemaphoreCount = 1;
        presentInfo.pWaitSemaphores = &renderFinishedSemaphores[Engine::getEngine().getCurrentFrame()];

        VkSwapchainKHR swapChains[] = {Engine::getEngine().getSwapChain()};
        presentInfo.swapchainCount = 1;
        presentInfo.pSwapchains = swapChains;

        presentInfo.pImageIndices = &imageIndex;

        result = vkQueuePresentKHR(presentQueue, &presentInfo);
        std::cout << "A" << std::endl;
        if (result == VK_ERROR_OUT_OF_DATE_KHR || result == VK_SUBOPTIMAL_KHR ||
            Engine::getEngine().isFramebufferResized()) {
            Engine::getEngine().setFramebufferResized(false);
            needToRecreateSwapChain = true;

        } else if (result != VK_SUCCESS) {
            throw std::runtime_error("failed to present swap chain image!");
        }

        Engine::getEngine().setCurrentFrame((Engine::getEngine().getCurrentFrame() + 1) % MAX_FRAMES_IN_FLIGHT);
        if (needToRecreateSwapChain) {
            recreateSwapChain();
        }
    }

    void waitForNextFrame() {
        vkDeviceWaitIdle(device);
    }

    VkShaderModule createShaderModule(const std::vector<char> &code) {
        VkShaderModuleCreateInfo createInfo{};
        createInfo.sType = VK_STRUCTURE_TYPE_SHADER_MODULE_CREATE_INFO;
        createInfo.codeSize = code.size();
        createInfo.pCode = reinterpret_cast<const uint32_t *>(code.data());

        VkShaderModule shaderModule;
        if (vkCreateShaderModule(device, &createInfo, nullptr, &shaderModule) !=
            VK_SUCCESS) {
            throw std::runtime_error("failed to createExtras shader module!");
        }

        return shaderModule;
    }

    void cleanUp() {
        for (const auto &r: renderPasses) {
            r->destroy();
        }
        for (const auto &c: cameras) {
            c->~Camera();
        }
        Engine::getEngine().cleanUpInfinite();
    }


}