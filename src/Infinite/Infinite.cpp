#include "Infinite.h"
#include "../../Engine.h"
#include "RenderPasses/RenderPass.h"
#include "../Model/Model.h"

namespace Infinite {
    void startInitInfinite() {
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
    }
    void finishInitInfinite() {
            for (const auto &r: renderPasses) {
                r->createCommandBuffers();
                r->createSyncObjects();
            }
    }

    void initInfinite() {
        startInitInfinite();
        finishInitInfinite();
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

    void recordCommandBuffer(VkCommandBuffer commandBuffer, uint32_t imageIndex, RenderPass renderPass) {
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
        bool needToRecreateSwapChain = false;

        vkWaitForFences(device, 1,
                        &renderPasses.back()->getSyncObjects().inFlightFences[Engine::getEngine().getCurrentFrame()],
                        VK_TRUE, UINT64_MAX); // ToDo: move

        uint32_t imageIndex;
        VkResult result = vkAcquireNextImageKHR(device,
                                                Engine::getEngine().getSwapChain(), UINT64_MAX,
                                                renderPasses.back()->getSyncObjects().imageAvailableSemaphores[Engine::getEngine().getCurrentFrame()],
                                                VK_NULL_HANDLE,
                                                &imageIndex);


        if (result == VK_ERROR_OUT_OF_DATE_KHR || result == VK_SUBOPTIMAL_KHR) {
            needToRecreateSwapChain = true;
        }

        vkResetFences(device, 1,
                      &renderPasses.back()->getSyncObjects().inFlightFences[Engine::getEngine().getCurrentFrame()]);

        for (const auto &r: renderPasses) {
            for (Model *model: r->getModels()) {
                model->updateUniformBuffer(Engine::getEngine().getCurrentFrame(), *Camera::cameras[r->getIndex()]);
            }
            r->run(imageIndex);
            vkResetFences(device, 1,
                          &r->getSyncObjects().inFlightFences[Engine::getEngine().getCurrentFrame()]);
        }

        VkPresentInfoKHR presentInfo{};
        presentInfo.sType = VK_STRUCTURE_TYPE_PRESENT_INFO_KHR;

        presentInfo.waitSemaphoreCount = 1;
        presentInfo.pWaitSemaphores = {
                &renderPasses.back()->getSyncObjects().renderFinishedSemaphores[Engine::getEngine().getCurrentFrame()]};

        VkSwapchainKHR swapChains[] = {Engine::getEngine().getSwapChain()};
        presentInfo.swapchainCount = 1;
        presentInfo.pSwapchains = swapChains;

        presentInfo.pImageIndices = &imageIndex;

        result = vkQueuePresentKHR(RenderPass::presentQueue, &presentInfo);

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

}