#include "BasicRenderPass.h"
#include "../../../Infinite.h"
#include "../../../util/constants.h"
#include "../../Model/Models/BaseModel.h"
#include "../../Software/ShaderAnalyzer.h"
#include "RenderPass.h"
#include <array>
#include <cstdint>
#include <cstring>
#include <iostream>
#include <optional>
#include <ostream>
#include <stdexcept>
#include <sys/types.h>
#include <vector>
#include <vulkan/vulkan_core.h>

namespace Infinite {

// TODO: change polygonMode for the rasterizer for the drawing of outlines.
// TODO: change depthClampEnable to VK_TRUE and enable the GPU feature to enable
// depth clamp
// TODO: FIX THE FREAKING FILE PATH

void BasicRenderPass::preInit(VkDevice device, VkPhysicalDevice physicalDevice,
                              VkFormat swapChainImageFormat,
                              VkExtent2D swapChainExtent,
                              VmaAllocator allocator,
                              VkSampleCountFlagBits msaaSamples,
                              std::vector<VkImageView> swapChainImageViews) {

  createPipeline(device, msaaSamples);
  createDepthAndColorImages(swapChainExtent.width, swapChainExtent.height,
                            swapChainImageFormat, physicalDevice, allocator);

  for (size_t i = 0; i < swapChainImageViews.size(); i++) {
    std::array<VkImageView, 3> attachments = {
        *getColorImageReasource()->getImageView(),
        *getDepthImageReasource()->getImageView(), swapChainImageViews[i]};

    VkFramebufferCreateInfo framebufferInfo{};
    framebufferInfo.sType = VK_STRUCTURE_TYPE_FRAMEBUFFER_CREATE_INFO;
    framebufferInfo.renderPass = renderPass;
    framebufferInfo.attachmentCount = static_cast<uint32_t>(attachments.size());
    framebufferInfo.pAttachments = attachments.data();
    framebufferInfo.width = swapChainExtent.width;
    framebufferInfo.height = swapChainExtent.height;
    framebufferInfo.layers = 1;

    if (vkCreateFramebuffer(device, &framebufferInfo, nullptr,
                            &swapChainFramebuffers[i]) != VK_SUCCESS) {
      throw std::runtime_error("failed to create framebuffer!");
    }
  }
}

void BasicRenderPass::destroy(VkDevice device, VmaAllocator allocator) {
  destroyDepthAndColorImages(allocator);

  for (BaseModel *model : models) {
    model->destroy(device, allocator);
  }
  vkDestroyPipeline(device, graphicsPipeline, nullptr);
  vkDestroyPipelineLayout(device, pipelineLayout, nullptr);
  vkDestroyRenderPass(device, renderPass, nullptr);

  for (size_t i = 0; i < MAX_FRAMES_IN_FLIGHT; i++) {
    vkDestroySemaphore(device, finishedSemaphores[i], nullptr);
    vkDestroySemaphore(device, imageAvailableSemaphores[i], nullptr);
    vkDestroyFence(device, inFlightFences[i], nullptr);
  }
}

void BasicRenderPass::createDepthAndColorImages(unsigned int width,
                                                unsigned int height,
                                                VkFormat colorFormat,
                                                VkPhysicalDevice physicalDevice,
                                                VmaAllocator allocator) {
  if (colorImageReasource == VK_NULL_HANDLE) {
    colorImageReasource = new ColorImage;
  }
  colorImageReasource->create(width, height, colorFormat, physicalDevice,
                              allocator);

  if (depthImageReasource == VK_NULL_HANDLE) {
    depthImageReasource = new DepthImage;
  }
  depthImageReasource->create(width, height, colorFormat, physicalDevice,
                              allocator);
}

void BasicRenderPass::createCommandBuffers(VkCommandPool commandPool,
                                           VkPhysicalDevice physicalDevice,
                                           VkDevice device) {
  commandBufferManager.create(commandPool, device);
}

VkPipelineBindPoint BasicRenderPass::getPipelineType() {
  return VK_PIPELINE_BIND_POINT_GRAPHICS;
}

void BasicRenderPass::recreateSwapChainWork(
    VmaAllocator allocator, VkDevice device, VkPhysicalDevice physicalDevice,
    VkFormat swapChainImageFormat, VkExtent2D swapChainExtent,
    std::vector<VkImageView> swapChainImageViews) {
  destroyDepthAndColorImages(allocator);
  createDepthAndColorImages(swapChainExtent.width, swapChainExtent.height,
                            swapChainImageFormat, physicalDevice, allocator);

  for (size_t i = 0; i < swapChainImageViews.size(); i++) {
    std::array<VkImageView, 3> attachments = {
        *getColorImageReasource()->getImageView(),
        *getDepthImageReasource()->getImageView(), swapChainImageViews[i]};

    VkFramebufferCreateInfo framebufferInfo{};
    framebufferInfo.sType = VK_STRUCTURE_TYPE_FRAMEBUFFER_CREATE_INFO;
    framebufferInfo.renderPass = renderPass;
    framebufferInfo.attachmentCount = static_cast<uint32_t>(attachments.size());
    framebufferInfo.pAttachments = attachments.data();
    framebufferInfo.width = swapChainExtent.width;
    framebufferInfo.height = swapChainExtent.height;
    framebufferInfo.layers = 1;

    if (vkCreateFramebuffer(device, &framebufferInfo, nullptr,
                            &swapChainFramebuffers[i]) != VK_SUCCESS) {
      throw std::runtime_error("failed to create framebuffer!");
    }
  }
}

void BasicRenderPass::createSyncObjects(VkDevice device) {
  imageAvailableSemaphores.resize(MAX_FRAMES_IN_FLIGHT);
  finishedSemaphores.resize(MAX_FRAMES_IN_FLIGHT);
  inFlightFences.resize(MAX_FRAMES_IN_FLIGHT);

  VkSemaphoreCreateInfo semaphoreInfo{};
  semaphoreInfo.sType = VK_STRUCTURE_TYPE_SEMAPHORE_CREATE_INFO;

  VkFenceCreateInfo fenceInfo{};
  fenceInfo.sType = VK_STRUCTURE_TYPE_FENCE_CREATE_INFO;
  fenceInfo.flags = VK_FENCE_CREATE_SIGNALED_BIT;

  for (size_t i = 0; i < MAX_FRAMES_IN_FLIGHT; i++) {
    if (vkCreateSemaphore(device, &semaphoreInfo, nullptr,
                          &imageAvailableSemaphores[i]) != VK_SUCCESS ||
        vkCreateSemaphore(device, &semaphoreInfo, nullptr,
                          &finishedSemaphores[i]) != VK_SUCCESS ||
        vkCreateFence(device, &fenceInfo, nullptr, &inFlightFences[i]) !=
            VK_SUCCESS) {

      throw std::runtime_error(
          "failed to create synchronization objects for a frame!");
    }
  }
}

void BasicRenderPass::destroyDepthAndColorImages(VmaAllocator allocator) {
  depthImageReasource->destroy(allocator);
  colorImageReasource->destroy(allocator);
  delete depthImageReasource;
  delete colorImageReasource;
}

ColorImage *BasicRenderPass::getColorImageReasource() const {
  return colorImageReasource;
}

DepthImage *BasicRenderPass::getDepthImageReasource() const {
  return depthImageReasource;
}

VkPipeline BasicRenderPass::getPipeline() { return graphicsPipeline; }

VkPipelineLayout BasicRenderPass::getPipelineLayout() const {
  return pipelineLayout;
}

VkCommandBuffer BasicRenderPass::getCommandBuffer(uint32_t index) {
  return commandBufferManager.commandBuffers[index];
}

void BasicRenderPass::createRenderPass(VkDevice device,
                                       VkPhysicalDevice physicalDevice,
                                       VkFormat swapChainImageFormat,
                                       VkSampleCountFlagBits msaaSamples) {
  VkAttachmentDescription colorAttachment{};
  colorAttachment.format = swapChainImageFormat;
  colorAttachment.loadOp = VK_ATTACHMENT_LOAD_OP_CLEAR;
  colorAttachment.storeOp = VK_ATTACHMENT_STORE_OP_STORE;
  colorAttachment.stencilLoadOp = VK_ATTACHMENT_LOAD_OP_DONT_CARE;
  colorAttachment.stencilStoreOp = VK_ATTACHMENT_STORE_OP_DONT_CARE;
  colorAttachment.initialLayout = VK_IMAGE_LAYOUT_UNDEFINED;
  colorAttachment.finalLayout = VK_IMAGE_LAYOUT_COLOR_ATTACHMENT_OPTIMAL;
  colorAttachment.samples = msaaSamples;

  VkAttachmentDescription colorAttachmentResolve{};
  colorAttachmentResolve.format = swapChainImageFormat;
  colorAttachmentResolve.samples = VK_SAMPLE_COUNT_1_BIT;
  colorAttachmentResolve.loadOp = VK_ATTACHMENT_LOAD_OP_DONT_CARE;
  colorAttachmentResolve.storeOp = VK_ATTACHMENT_STORE_OP_STORE;
  colorAttachmentResolve.stencilLoadOp = VK_ATTACHMENT_LOAD_OP_DONT_CARE;
  colorAttachmentResolve.stencilStoreOp = VK_ATTACHMENT_STORE_OP_DONT_CARE;
  colorAttachmentResolve.initialLayout = VK_IMAGE_LAYOUT_UNDEFINED;
  colorAttachmentResolve.finalLayout = VK_IMAGE_LAYOUT_PRESENT_SRC_KHR;

  VkAttachmentReference colorAttachmentRef{};
  colorAttachmentRef.attachment = 0;
  colorAttachmentRef.layout = VK_IMAGE_LAYOUT_COLOR_ATTACHMENT_OPTIMAL;

  VkAttachmentReference colorAttachmentResolveRef{};
  colorAttachmentResolveRef.attachment = 2;
  colorAttachmentResolveRef.layout = VK_IMAGE_LAYOUT_COLOR_ATTACHMENT_OPTIMAL;

  VkAttachmentDescription depthAttachment{};
  depthAttachment.format = DepthImage::findDepthFormat(physicalDevice);
  depthAttachment.loadOp = VK_ATTACHMENT_LOAD_OP_CLEAR;
  depthAttachment.storeOp = VK_ATTACHMENT_STORE_OP_DONT_CARE;
  depthAttachment.stencilLoadOp = VK_ATTACHMENT_LOAD_OP_DONT_CARE;
  depthAttachment.stencilStoreOp = VK_ATTACHMENT_STORE_OP_DONT_CARE;
  depthAttachment.initialLayout = VK_IMAGE_LAYOUT_UNDEFINED;
  depthAttachment.finalLayout =
      VK_IMAGE_LAYOUT_DEPTH_STENCIL_ATTACHMENT_OPTIMAL;
  depthAttachment.samples = msaaSamples;

  VkAttachmentReference depthAttachmentRef{};
  depthAttachmentRef.attachment = 1;
  depthAttachmentRef.layout = VK_IMAGE_LAYOUT_DEPTH_STENCIL_ATTACHMENT_OPTIMAL;

  VkSubpassDescription subpass{};
  subpass.pipelineBindPoint = VK_PIPELINE_BIND_POINT_GRAPHICS;
  subpass.colorAttachmentCount = 1;
  subpass.pColorAttachments = &colorAttachmentRef;
  subpass.pDepthStencilAttachment = &depthAttachmentRef;
  subpass.pResolveAttachments = &colorAttachmentResolveRef;

  VkSubpassDependency dependency{};
  dependency.srcSubpass = VK_SUBPASS_EXTERNAL;
  dependency.dstSubpass = 0;
  dependency.srcStageMask = VK_PIPELINE_STAGE_COLOR_ATTACHMENT_OUTPUT_BIT;
  dependency.srcAccessMask = 0;
  dependency.dstStageMask = VK_PIPELINE_STAGE_COLOR_ATTACHMENT_OUTPUT_BIT;
  dependency.dstAccessMask = VK_ACCESS_COLOR_ATTACHMENT_WRITE_BIT;

  std::array<VkAttachmentDescription, 3> attachments = {
      colorAttachment, depthAttachment, colorAttachmentResolve};
  VkRenderPassCreateInfo renderPassInfo{};
  renderPassInfo.sType = VK_STRUCTURE_TYPE_RENDER_PASS_CREATE_INFO;
  renderPassInfo.attachmentCount = static_cast<uint32_t>(attachments.size());
  renderPassInfo.pAttachments = attachments.data();
  renderPassInfo.subpassCount = 1;
  renderPassInfo.pSubpasses = &subpass;
  renderPassInfo.dependencyCount = 1;
  renderPassInfo.pDependencies = &dependency;

  if (vkCreateRenderPass(device, &renderPassInfo, nullptr, &renderPass) !=
      VK_SUCCESS) {
    throw std::runtime_error("failed to create render pass!");
  }
}

void BasicRenderPass::createPipeline(VkDevice device,
                                     VkSampleCountFlagBits msaaSamples) {
  auto vertShaderCode = readFile(R"(../assets/shaders/vert.spv)");
  auto fragShaderCode = readFile(R"(../assets/shaders/frag.spv)");

  VkShaderModule vertShaderModule = createShaderModule(vertShaderCode, device);
  VkShaderModule fragShaderModule = createShaderModule(fragShaderCode, device);

  std::array<VkVertexInputAttributeDescription, 2> attributes = Vertex::getAttributeDescriptions();
  std::vector<VkVertexInputBindingDescription> bindings = {Vertex::getBindingDescription()};

  ShaderLayout layout = {};
  generateShaderLayout(layout, {vertShaderModule, fragShaderModule},
                       {vertShaderCode, fragShaderCode});

  VkDescriptorSetLayout descriptorSetLayout =
      DescriptorSet::createDescriptorSetLayout(device, layout.highLevelLayout);


  VkPipelineInputAssemblyStateCreateInfo inputAssembly{};
  inputAssembly.sType =
      VK_STRUCTURE_TYPE_PIPELINE_INPUT_ASSEMBLY_STATE_CREATE_INFO;
  inputAssembly.topology = VK_PRIMITIVE_TOPOLOGY_TRIANGLE_LIST;
  inputAssembly.primitiveRestartEnable = VK_FALSE;

  VkPipelineViewportStateCreateInfo viewportState{};
  viewportState.sType = VK_STRUCTURE_TYPE_PIPELINE_VIEWPORT_STATE_CREATE_INFO;
  viewportState.viewportCount = 1;
  viewportState.scissorCount = 1;

  VkPipelineRasterizationStateCreateInfo rasterizer{};
  rasterizer.sType = VK_STRUCTURE_TYPE_PIPELINE_RASTERIZATION_STATE_CREATE_INFO;
  rasterizer.depthClampEnable = VK_FALSE;
  rasterizer.rasterizerDiscardEnable = VK_FALSE;
  rasterizer.polygonMode = VK_POLYGON_MODE_FILL;
  rasterizer.lineWidth = 1.0f;
  rasterizer.cullMode = VK_CULL_MODE_BACK_BIT;
  rasterizer.frontFace = VK_FRONT_FACE_COUNTER_CLOCKWISE;
  rasterizer.depthBiasEnable = VK_FALSE;

  VkPipelineMultisampleStateCreateInfo multisampling{};
  multisampling.sType =
      VK_STRUCTURE_TYPE_PIPELINE_MULTISAMPLE_STATE_CREATE_INFO;
  multisampling.rasterizationSamples = msaaSamples;
  multisampling.sampleShadingEnable =
      VK_TRUE; // enable sample shading in the pipeline
  multisampling.minSampleShading =
      1.0f; // min fraction for sample shading; closer to one is smoother

  VkPipelineColorBlendAttachmentState colorBlendAttachment{};
  colorBlendAttachment.colorWriteMask =
      VK_COLOR_COMPONENT_R_BIT | VK_COLOR_COMPONENT_G_BIT |
      VK_COLOR_COMPONENT_B_BIT | VK_COLOR_COMPONENT_A_BIT;
  colorBlendAttachment.blendEnable = VK_TRUE;
  colorBlendAttachment.srcColorBlendFactor = VK_BLEND_FACTOR_SRC_ALPHA;
  colorBlendAttachment.dstColorBlendFactor =
      VK_BLEND_FACTOR_ONE_MINUS_SRC_ALPHA;
  colorBlendAttachment.colorBlendOp = VK_BLEND_OP_ADD;
  colorBlendAttachment.srcAlphaBlendFactor = VK_BLEND_FACTOR_ONE;
  colorBlendAttachment.dstAlphaBlendFactor = VK_BLEND_FACTOR_ZERO;
  colorBlendAttachment.alphaBlendOp = VK_BLEND_OP_ADD;

  VkPipelineColorBlendStateCreateInfo colorBlending{};
  colorBlending.sType =
      VK_STRUCTURE_TYPE_PIPELINE_COLOR_BLEND_STATE_CREATE_INFO;
  colorBlending.logicOpEnable = VK_FALSE;
  colorBlending.logicOp = VK_LOGIC_OP_COPY;
  colorBlending.attachmentCount = 1;
  colorBlending.pAttachments = &colorBlendAttachment;
  colorBlending.blendConstants[0] = 0.0f;
  colorBlending.blendConstants[1] = 0.0f;
  colorBlending.blendConstants[2] = 0.0f;
  colorBlending.blendConstants[3] = 0.0f;

  std::vector<VkDynamicState> dynamicStates = {VK_DYNAMIC_STATE_VIEWPORT,
                                               VK_DYNAMIC_STATE_SCISSOR};
  VkPipelineDynamicStateCreateInfo dynamicState{};
  dynamicState.sType = VK_STRUCTURE_TYPE_PIPELINE_DYNAMIC_STATE_CREATE_INFO;
  dynamicState.dynamicStateCount = static_cast<uint32_t>(dynamicStates.size());
  dynamicState.pDynamicStates = dynamicStates.data();

  VkPipelineLayoutCreateInfo pipelineLayoutInfo{};
  pipelineLayoutInfo.sType = VK_STRUCTURE_TYPE_PIPELINE_LAYOUT_CREATE_INFO;
  pipelineLayoutInfo.setLayoutCount = 1;
  pipelineLayoutInfo.pSetLayouts = &descriptorSetLayout;

  if (vkCreatePipelineLayout(device, &pipelineLayoutInfo, nullptr,
                             &pipelineLayout) != VK_SUCCESS) {
    throw std::runtime_error("failed to create pipeline layout!");
  }

  VkPipelineDepthStencilStateCreateInfo depthStencil{};
  depthStencil.sType =
      VK_STRUCTURE_TYPE_PIPELINE_DEPTH_STENCIL_STATE_CREATE_INFO;
  depthStencil.depthTestEnable = VK_TRUE;
  depthStencil.depthWriteEnable = VK_TRUE;
  depthStencil.depthCompareOp = VK_COMPARE_OP_LESS;
  depthStencil.depthBoundsTestEnable = VK_FALSE;
  depthStencil.minDepthBounds = 0.0f; // Optional
  depthStencil.maxDepthBounds = 1.0f; // Optional
  depthStencil.stencilTestEnable = VK_FALSE;
  depthStencil.front = {}; // Optional
  depthStencil.back = {};  // Optional

  VkGraphicsPipelineCreateInfo pipelineInfo{};
  pipelineInfo.sType = VK_STRUCTURE_TYPE_GRAPHICS_PIPELINE_CREATE_INFO;
  pipelineInfo.stageCount = 2;
  pipelineInfo.pStages = layout.shaderStages.data();


  VkPipelineVertexInputStateCreateInfo infoV{};
  infoV.pVertexAttributeDescriptions = attributes.data();
  infoV.pVertexBindingDescriptions = bindings.data();
  infoV.vertexBindingDescriptionCount = bindings.size();
  infoV.vertexAttributeDescriptionCount = attributes.size();

  pipelineInfo.pVertexInputState = &infoV;
  pipelineInfo.pInputAssemblyState = &inputAssembly;
  pipelineInfo.pViewportState = &viewportState;
  pipelineInfo.pRasterizationState = &rasterizer;
  pipelineInfo.pMultisampleState = &multisampling;
  pipelineInfo.pDepthStencilState = &depthStencil;
  pipelineInfo.pColorBlendState = &colorBlending;
  pipelineInfo.pDynamicState = &dynamicState;
  pipelineInfo.layout = pipelineLayout;
  pipelineInfo.renderPass = renderPass;
  pipelineInfo.subpass = 0;
  pipelineInfo.basePipelineHandle = VK_NULL_HANDLE; // Optional
  pipelineInfo.basePipelineIndex = -1;              // Optional

  if (vkCreateGraphicsPipelines(device, VK_NULL_HANDLE, 1, &pipelineInfo,
                                nullptr, &graphicsPipeline) != VK_SUCCESS) {
    throw std::runtime_error("failed to create graphics pipeline!");
  }

  vkDestroyShaderModule(device, fragShaderModule, nullptr);
  vkDestroyShaderModule(device, vertShaderModule, nullptr);
}

void BasicRenderPass::recordCommandBuffer(VkCommandBuffer commandBuffer,
                                          uint32_t imageIndex,
                                          uint32_t currentFrame) {
  VkCommandBufferBeginInfo beginInfo{};
  beginInfo.sType = VK_STRUCTURE_TYPE_COMMAND_BUFFER_BEGIN_INFO;
  beginInfo.flags = 0;                  // Optional
  beginInfo.pInheritanceInfo = nullptr; // Optional

  if (vkBeginCommandBuffer(commandBuffer, &beginInfo) != VK_SUCCESS) {
    throw std::runtime_error("failed to begin recording command buffer!");
  }

  VkRenderPassBeginInfo renderPassInfo{};
  renderPassInfo.sType = VK_STRUCTURE_TYPE_RENDER_PASS_BEGIN_INFO;
  renderPassInfo.renderPass = renderPass;
  renderPassInfo.framebuffer = swapChainFramebuffers[imageIndex]; // ToDo check
  renderPassInfo.renderArea.offset = {0, 0};
  renderPassInfo.renderArea.extent = swapChainExtent;

  std::array<VkClearValue, 2> clearValues{};
  clearValues[0].color = {{0.0f, 0.0f, 0.0f, 1.0f}};
  clearValues[1].depthStencil = {1.0f, 0};

  renderPassInfo.clearValueCount = static_cast<uint32_t>(clearValues.size());
  renderPassInfo.pClearValues = clearValues.data();

  vkCmdBeginRenderPass(commandBuffer, &renderPassInfo,
                       VK_SUBPASS_CONTENTS_INLINE);

  // Commands
  vkCmdBindPipeline(commandBuffer, getPipelineType(), graphicsPipeline);

  VkViewport viewport{};
  viewport.x = 0.0f;
  viewport.y = 0.0f;
  viewport.width = static_cast<float>(swapChainExtent.width);
  viewport.height = static_cast<float>(swapChainExtent.height);
  viewport.minDepth = 0.0f;
  viewport.maxDepth = 1.0f;
  vkCmdSetViewport(commandBuffer, 0, 1, &viewport);

  VkRect2D scissor{};
  scissor.offset = {0, 0};
  scissor.extent = swapChainExtent;
  vkCmdSetScissor(commandBuffer, 0, 1, &scissor);

  for (int i = 0; i < getModels().size(); i++) {
    BufferAlloc vertexBuffers[] = {getModels()[i]->getVertexBuffer()};
    VkDeviceSize offsets[] = {0};
    vkCmdBindVertexBuffers(commandBuffer, 0, 1, &vertexBuffers->buffer,
                           offsets);

    vkCmdBindIndexBuffer(commandBuffer, getModels()[i]->getIndexBuffer().buffer,
                         0, VK_INDEX_TYPE_UINT32);

    vkCmdBindDescriptorSets(
        commandBuffer, VK_PIPELINE_BIND_POINT_GRAPHICS, getPipelineLayout(), 0,
        1,
        &getModels()[i]->getDescriptorSet().getDescriptorSets()[currentFrame],
        0, nullptr);

    vkCmdDrawIndexed(commandBuffer,
                     static_cast<uint32_t>(getModels()[i]->getIndexCount()), 1,
                     0, 0, 0);
  }
  // End Commands

  vkCmdEndRenderPass(commandBuffer);

  if (vkEndCommandBuffer(commandBuffer) != VK_SUCCESS) {
    throw std::runtime_error("failed to record command buffer!");
  }
}

VkSubmitInfo BasicRenderPass::renderFrame(uint32_t currentFrame,
                                          uint32_t imageIndex) {
  VkSubmitInfo submitInformation;
  vkResetCommandBuffer(commandBufferManager.commandBuffers[currentFrame], 0);

  recordCommandBuffer(commandBufferManager.commandBuffers[currentFrame],
                      imageIndex, currentFrame);

  // submitInformation.sType = VK_STRUCTURE_TYPE_SUBMIT_INFO;

  // VkSemaphore waitSemaphores[] = {imageAvailableSemaphores[currentFrame]};

  // VkPipelineStageFlags waitStages[] = {
  //     VK_PIPELINE_STAGE_COLOR_ATTACHMENT_OUTPUT_BIT};
  // submitInformation.waitSemaphoreCount = 1;
  // submitInformation.pWaitSemaphores = waitSemaphores;
  // submitInformation.pWaitDstStageMask = waitStages;

  // submitInformation.commandBufferCount = 1;
  // submitInformation.pNext = VK_NULL_HANDLE;

  // const VkCommandBuffer v =
  // commandBufferManager.commandBuffers[currentFrame];
  // submitInformation.pCommandBuffers = &v;

  // VkSemaphore signalSemaphores[] = {
  //     finishedSemaphores[currentFrame]}; // Todo: is this necessary
  // submitInformation.signalSemaphoreCount = 1;
  // submitInformation.pSignalSemaphores = signalSemaphores;
  return submitInformation;
}

void BasicRenderPass::waitForFences() {
  vkWaitForFences(device, 1, &inFlightFences[currentFrame], VK_TRUE,
                  UINT64_MAX);
}
void BasicRenderPass::resetFences() {
  vkResetFences(device, 1, &inFlightFences[currentFrame]);
}
} // namespace Infinite