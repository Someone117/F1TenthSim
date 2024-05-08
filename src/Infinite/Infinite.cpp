#include "Infinite.h"
#include "backend/Model/Models/DescriptorSet.h"
#include "backend/Rendering/Engine.h"
#include "backend/Rendering/RenderPasses/RenderPass.h"
#include "backend/Settings.h"
#include "util/VulkanUtils.h"

namespace Infinite {
VkPhysicalDevice physicalDevice;
VkDevice device;
//  Settings * settings;
VkFormat swapChainImageFormat;
VmaAllocator allocator;
VkCommandPool imagePool;
std::vector<RenderPass *> renderPasses; // ToDo: CleanUp

void initInfinite() {
  Engine::getEngine().createEngine();

  QueueFamilyIndices queueFamilyIndices =
      findQueueFamilies(physicalDevice); // fix this hell

  createCommandPool(imagePool, device, queueFamilyIndices);

  for (const auto &r : renderPasses) {
    r->createRenderPass(device, physicalDevice, swapChainImageFormat,
                        msaaSamples);
  }

  VkDescriptorSetLayout setLayout;

  std::vector<DescriptorSetLayout> layout = {
      {1, VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER, VK_SHADER_STAGE_VERTEX_BIT},
      {1, VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER,
       VK_SHADER_STAGE_FRAGMENT_BIT},
      {1, VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER, VK_SHADER_STAGE_COMPUTE_BIT},
      {1, VK_DESCRIPTOR_TYPE_STORAGE_BUFFER, VK_SHADER_STAGE_COMPUTE_BIT},
      {1, VK_DESCRIPTOR_TYPE_STORAGE_BUFFER, VK_SHADER_STAGE_COMPUTE_BIT}};
  setLayout =
      DescriptorSet::createDescriptorSetLayout(device, &setLayout, layout);

  for (const auto &r : renderPasses) {
    r->createPipeline(setLayout, device, msaaSamples);

    r->createDepthAndColorImages(Engine::getEngine().getWindowWidth(),
                                 Engine::getEngine().getWindowHeight(),
                                 Engine::getEngine().getSwapChainImageFormat(),
                                 physicalDevice, allocator);
    Engine::getEngine().createFramebuffers(*r, renderPasses.size());
  }

  DescriptorSet::createDescriptorPool(device);

  for (const auto &r : renderPasses) {
    r->createCommandBuffers(imagePool, physicalDevice, device,
                            queueFamilyIndices);
  }
  RenderPass::createSyncObjects(device, MAX_FRAMES_IN_FLIGHT);
}

void addRenderPass(RenderPass *r) {
  renderPasses.push_back(r);
  r->setIndex(renderPasses.size() - 1);
}

void recreateSwapChain() {
  Engine::getEngine().recreateSwapChain();
  for (const auto &r : renderPasses) {
    r->destroyDepthAndColorImages(allocator);
    r->createDepthAndColorImages(Engine::getEngine().getWindowWidth(),
                                 Engine::getEngine().getWindowHeight(),
                                 Engine::getEngine().getSwapChainImageFormat(),
                                 physicalDevice, allocator);
    Engine::getEngine().createFramebuffers(*r, renderPasses.size());
  }
}

void recordCommandBuffer(VkCommandBuffer commandBuffer, uint32_t imageIndex,
                         RenderPass &renderPass, uint32_t currentFrame) {
  VkCommandBufferBeginInfo beginInfo{};
  beginInfo.sType = VK_STRUCTURE_TYPE_COMMAND_BUFFER_BEGIN_INFO;
  beginInfo.flags = 0;                  // Optional
  beginInfo.pInheritanceInfo = nullptr; // Optional

  if (vkBeginCommandBuffer(commandBuffer, &beginInfo) != VK_SUCCESS) {
    throw std::runtime_error("failed to begin recording command buffer!");
  }

  VkRenderPassBeginInfo renderPassInfo{};
  renderPassInfo.sType = VK_STRUCTURE_TYPE_RENDER_PASS_BEGIN_INFO;
  renderPassInfo.renderPass = renderPass.getRenderPass();
  renderPassInfo.framebuffer =
      Engine::getEngine().getSwapChainFrameBuffer(renderPass.getIndex(),
                                                  imageIndex); // ToDo check
  renderPassInfo.renderArea.offset = {0, 0};
  renderPassInfo.renderArea.extent = Engine::getEngine().getSwapChainExtent();

  std::array<VkClearValue, 2> clearValues{};
  clearValues[0].color = {{0.0f, 0.0f, 0.0f, 1.0f}};
  clearValues[1].depthStencil = {1.0f, 0};

  renderPassInfo.clearValueCount = static_cast<uint32_t>(clearValues.size());
  renderPassInfo.pClearValues = clearValues.data();

  vkCmdBeginRenderPass(commandBuffer, &renderPassInfo,
                       VK_SUBPASS_CONTENTS_INLINE);

  // Commands
  vkCmdBindPipeline(commandBuffer, VK_PIPELINE_BIND_POINT_GRAPHICS,
                    renderPass.getGraphicsPipeline());

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
  scissor.extent = Engine::getEngine().getSwapChainExtent();
  vkCmdSetScissor(commandBuffer, 0, 1, &scissor);

  for (int i = 0; i < renderPass.getModels().size(); i++) {
    BufferAlloc vertexBuffers[] = {
        renderPass.getModels()[i]->getVertexBuffer()};
    VkDeviceSize offsets[] = {0};
    vkCmdBindVertexBuffers(commandBuffer, 0, 1, &vertexBuffers->buffer,
                           offsets);

    vkCmdBindIndexBuffer(commandBuffer,
                         renderPass.getModels()[i]->getIndexBuffer().buffer, 0,
                         VK_INDEX_TYPE_UINT32);

    vkCmdBindDescriptorSets(commandBuffer, VK_PIPELINE_BIND_POINT_GRAPHICS,
                            renderPass.getPipelineLayout(), 0, 1,
                            &renderPass.getModels()[i]
                                 ->getDescriptorSet()
                                 .getDescriptorSets()[currentFrame],
                            0, nullptr);

    vkCmdDrawIndexed(
        commandBuffer,
        static_cast<uint32_t>(renderPass.getModels()[i]->getIndexCount()), 1, 0,
        0, 0);
  }
  // End Commands

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
  VkResult result = vkAcquireNextImageKHR(
      device, Engine::getEngine().getSwapChain(), UINT64_MAX,
      imageAvailableSemaphores[Engine::getEngine().getCurrentFrame()],
      VK_NULL_HANDLE, &imageIndex);

  if (result == VK_ERROR_OUT_OF_DATE_KHR || result == VK_SUBOPTIMAL_KHR) {
    needToRecreateSwapChain = true;
  }
  std::vector<VkSubmitInfo> submitInformation(renderPasses.size());
  for (const auto &r : renderPasses) {
    for (Model *model : r->getModels()) {
      model->updateUniformBuffer(Engine::getEngine().getCurrentFrame(),
                                 *cameras[r->getIndex()], allocator,
                                 Engine::getEngine().getWindowWidth(),
                                 Engine::getEngine().getWindowHeight());
    }
  }

  vkResetFences(device, 1,
                &inFlightFences[Engine::getEngine().getCurrentFrame()]);

  for (size_t i = 0; i < renderPasses.size(); i++) {
    vkResetCommandBuffer(
        renderPasses[i]->getCommandBuffer(
            Engine::getEngine()
                .getCurrentFrame()), /*VkCommandBufferResetFlagBits*/
        0);

    Infinite::recordCommandBuffer(renderPasses[i]->getCommandBuffer(
                                      Engine::getEngine().getCurrentFrame()),
                                  renderPasses[i]->getIndex(), *renderPasses[i],
                                  Engine::getEngine().getCurrentFrame());

    submitInformation[i].sType = VK_STRUCTURE_TYPE_SUBMIT_INFO;

    VkSemaphore waitSemaphores[] = {
        imageAvailableSemaphores[Engine::getEngine().getCurrentFrame()]};
    VkPipelineStageFlags waitStages[] = {
        VK_PIPELINE_STAGE_COLOR_ATTACHMENT_OUTPUT_BIT};
    submitInformation[i].waitSemaphoreCount = 1;
    submitInformation[i].pWaitSemaphores = waitSemaphores;
    submitInformation[i].pWaitDstStageMask = waitStages;

    submitInformation[i].commandBufferCount = 1;

    const VkCommandBuffer v = renderPasses[i]->getCommandBuffer(
        Engine::getEngine().getCurrentFrame());
    submitInformation[i].pCommandBuffers = &v;

    VkSemaphore signalSemaphores[] = {
        renderFinishedSemaphores
            [Engine::getEngine().getCurrentFrame()]}; // Todo: is this necessary
    submitInformation[i].signalSemaphoreCount = 1;
    submitInformation[i].pSignalSemaphores = signalSemaphores;
  }

  if (vkQueueSubmit(Engine::getEngine().getGraphicsQueue(),
                    submitInformation.size(), submitInformation.data(),
                    inFlightFences[Engine::getEngine().getCurrentFrame()]) !=
      VK_SUCCESS) {
    throw std::runtime_error("failed to submit draw command buffer!");
  }

  VkPresentInfoKHR presentInfo{};
  presentInfo.sType = VK_STRUCTURE_TYPE_PRESENT_INFO_KHR;

  presentInfo.waitSemaphoreCount = 1;
  presentInfo.pWaitSemaphores =
      &renderFinishedSemaphores[Engine::getEngine().getCurrentFrame()];

  VkSwapchainKHR swapChains[] = {Engine::getEngine().getSwapChain()};
  presentInfo.swapchainCount = 1;
  presentInfo.pSwapchains = swapChains;

  presentInfo.pImageIndices = &imageIndex;

  result = vkQueuePresentKHR(presentQueue, &presentInfo);
  if (result == VK_ERROR_OUT_OF_DATE_KHR || result == VK_SUBOPTIMAL_KHR ||
      Engine::getEngine().isFramebufferResized()) {
    Engine::getEngine().setFramebufferResized(false);
    needToRecreateSwapChain = true;

  } else if (result != VK_SUCCESS) {
    throw std::runtime_error("failed to present swap chain image!");
  }

  Engine::getEngine().setCurrentFrame(
      (Engine::getEngine().getCurrentFrame() + 1) % MAX_FRAMES_IN_FLIGHT);
  if (needToRecreateSwapChain) {
    recreateSwapChain();
  }
}

void waitForNextFrame() { vkDeviceWaitIdle(device); }

void cleanUp() {
  for (const auto &r : renderPasses) {
    r->destroy(device, allocator, MAX_FRAMES_IN_FLIGHT);
  }
  for (const auto &c : cameras) {
    c->~Camera();
  }
  Engine::getEngine().cleanUpInfinite();
}

Model createModel(std::string _name, std::string model_path,
                  const char *texture_path) {
  return Model(_name, model_path, texture_path, device, physicalDevice,
               allocator, Engine::getEngine().getWindowWidth(),
               Engine::getEngine().getWindowHeight(), swapChainImageFormat);
}

void createCommandPool(VkCommandPool &pool, VkDevice device,
                       QueueFamilyIndices queueFamilyIndices) {

  VkCommandPoolCreateInfo poolInfo{};
  poolInfo.sType = VK_STRUCTURE_TYPE_COMMAND_POOL_CREATE_INFO;
  poolInfo.flags = VK_COMMAND_POOL_CREATE_RESET_COMMAND_BUFFER_BIT;
  poolInfo.queueFamilyIndex = queueFamilyIndices.graphicsFamily.value();

  if (vkCreateCommandPool(device, &poolInfo, nullptr, &pool) != VK_SUCCESS) {
    throw std::runtime_error("failed to create command pool!");
  }
}

} // namespace Infinite