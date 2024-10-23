#include "ShaderAnalyzer.h"
#include "../../../../libsSrc/SPIRV-Cross/spirv_cross.hpp"
#include <cstdint>
#include <cstring>
#include <iostream>
#include <memory>
#include <stdexcept>
#include <sys/types.h>
#include <utility>
#include <vector>
#include <vulkan/vulkan_core.h>

namespace Infinite {

void generateShaderLayout(ShaderLayout &layout,
                          const std::vector<VkShaderModule> &shaderModules,
                          const std::vector<std::vector<char>> &shaderCodes) {
  if (shaderModules.size() != shaderCodes.size()) {
    throw std::runtime_error(
        "Number of shader modules does not equal the number of shader codes!");
  }

  uint32_t vertexShaderCount = 0;

  for (uint32_t i = 0; i < shaderModules.size(); i++) {
    if (shaderCodes[i].size() % sizeof(uint32_t) != 0) {
      throw std::runtime_error("Input shader code size is not a multiple of 4");
    }

    // Create a vector of uint32_t with the appropriate size
    std::vector<uint32_t> code(shaderCodes[i].size() / sizeof(uint32_t));
    memcpy(code.data(), shaderCodes[i].data(), shaderCodes[i].size());

    spirv_cross::Compiler comp(std::move(code));
    spirv_cross::ShaderResources res = comp.get_shader_resources();
    spv::ExecutionModel execution_model = comp.get_execution_model();
    VkPipelineShaderStageCreateInfo shaderStageInfo{};
    shaderStageInfo.sType = VK_STRUCTURE_TYPE_PIPELINE_SHADER_STAGE_CREATE_INFO;
    shaderStageInfo.module = shaderModules[i];
    shaderStageInfo.pName = "main";

    switch (execution_model) {
    case spv::ExecutionModelVertex:
      shaderStageInfo.stage = VK_SHADER_STAGE_VERTEX_BIT;
      break;
    case spv::ExecutionModelFragment:
      shaderStageInfo.stage = VK_SHADER_STAGE_FRAGMENT_BIT;
      break;
    case spv::ExecutionModelGLCompute:
      shaderStageInfo.stage = VK_SHADER_STAGE_COMPUTE_BIT;
      break;
    default:
      std::cerr << "Found: " << execution_model
                << " shader type, please implement this" << std::endl;
    }

    layout.shaderStages.push_back(shaderStageInfo);

    for (auto &u : res.uniform_buffers) {
      layout.highLevelLayout.push_back(
          {1, VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER, shaderStageInfo.stage});
    }

    for (auto &u : res.sampled_images) {
      uint32_t binding = comp.get_decoration(u.id, spv::DecorationBinding);
      layout.highLevelLayout.push_back(
          {1, VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER,
           shaderStageInfo.stage});
    }

    for (auto &u : res.storage_buffers) {
      uint32_t location = comp.get_decoration(u.id, spv::DecorationLocation);
      layout.highLevelLayout.push_back(
          {1, VK_DESCRIPTOR_TYPE_STORAGE_BUFFER, shaderStageInfo.stage});
    }

    for(auto u : res.storage_images) {
      uint32_t binding = comp.get_decoration(u.id, spv::DecorationBinding);
      layout.highLevelLayout.push_back(
          {1, VK_DESCRIPTOR_TYPE_STORAGE_IMAGE,
           shaderStageInfo.stage});
    }
  }
}

void createVCI(VkPipelineVertexInputStateCreateInfo &v,
               std::vector<VkVertexInputAttributeDescription> *off,
               std::vector<VkVertexInputBindingDescription> *binding) {
  v.sType = VK_STRUCTURE_TYPE_PIPELINE_VERTEX_INPUT_STATE_CREATE_INFO;
  v.pVertexAttributeDescriptions = off->data();
  v.pVertexBindingDescriptions = binding->data();
  v.vertexBindingDescriptionCount = binding->size();
  v.vertexAttributeDescriptionCount = off->size();
}

VkFormat chooseFormat(uint32_t basetype, uint32_t vecsize) {
  switch (basetype) {
  case spirv_cross::SPIRType::Int:
    switch (vecsize) {
    case 1:
      return VK_FORMAT_R32_SINT;
      break;
    case 2:
      return VK_FORMAT_R32G32_SINT;
      break;
    case 3:
      return VK_FORMAT_R32G32B32_SINT;
      break;
    case 4:
      return VK_FORMAT_R32G32B32A32_SINT;
      break;
    default:
      return VK_FORMAT_UNDEFINED;
      break;
    }
    break;
  case spirv_cross::SPIRType::UInt:
    switch (vecsize) {
    case 1:
      return VK_FORMAT_R32_UINT;
      break;
    case 2:
      return VK_FORMAT_R32G32_UINT;
      break;
    case 3:
      return VK_FORMAT_R32G32B32_UINT;
      break;
    case 4:
      return VK_FORMAT_R32G32B32A32_UINT;
      break;
    default:
      return VK_FORMAT_UNDEFINED;
      break;
    }
    break;
  case spirv_cross::SPIRType::Int64:
    switch (vecsize) {
    case 1:
      return VK_FORMAT_R64_SINT;
      break;
    case 2:
      return VK_FORMAT_R64G64_SINT;
      break;
    case 3:
      return VK_FORMAT_R64G64B64_SINT;
      break;
    case 4:
      return VK_FORMAT_R64G64B64A64_SINT;
      break;
    default:
      return VK_FORMAT_UNDEFINED;
      break;
    }
    break;
  case spirv_cross::SPIRType::UInt64:
    switch (vecsize) {
    case 1:
      return VK_FORMAT_R64_UINT;
      break;
    case 2:
      return VK_FORMAT_R64G64_UINT;
      break;
    case 3:
      return VK_FORMAT_R64G64B64_UINT;
      break;
    case 4:
      return VK_FORMAT_R64G64B64A64_UINT;
      break;
    default:
      return VK_FORMAT_UNDEFINED;
      break;
    }
    break;
  case spirv_cross::SPIRType::Half:
    switch (vecsize) {
    case 1:
      return VK_FORMAT_R16_SFLOAT;
      break;
    case 2:
      return VK_FORMAT_R16G16_SFLOAT;
      break;
    case 3:
      return VK_FORMAT_R16G16B16_SFLOAT;
      break;
    case 4:
      return VK_FORMAT_R16G16B16A16_SFLOAT;
      break;
    default:
      return VK_FORMAT_UNDEFINED;
      break;
    }
    break;
  case spirv_cross::SPIRType::Float:
    switch (vecsize) {
    case 1:
      return VK_FORMAT_R32_SFLOAT;
      break;
    case 2:
      return VK_FORMAT_R32G32_SFLOAT;
      break;
    case 3:
      return VK_FORMAT_R32G32B32_SFLOAT;
      break;
    case 4:
      return VK_FORMAT_R32G32B32A32_SFLOAT;
      break;
    default:
      return VK_FORMAT_UNDEFINED;
      break;
    }
    break;
  case spirv_cross::SPIRType::Double:
    switch (vecsize) {
    case 1:
      return VK_FORMAT_R64_SFLOAT;
      break;
    case 2:
      return VK_FORMAT_R64G64_SFLOAT;
      break;
    case 3:
      return VK_FORMAT_R64G64B64_SFLOAT;
      break;
    case 4:
      return VK_FORMAT_R64G64B64A64_SFLOAT;
      break;
    default:
      return VK_FORMAT_UNDEFINED;
      break;
    }
    break;
  case spirv_cross::SPIRType::Char:
    switch (vecsize) {
    case 1:
      return VK_FORMAT_R8_SINT;
      break;
    case 2:
      return VK_FORMAT_R8G8_SINT;
      break;
    case 3:
      return VK_FORMAT_R8G8B8_SINT;
      break;
    case 4:
      return VK_FORMAT_R8G8B8A8_SINT;
      break;
    default:
      return VK_FORMAT_UNDEFINED;
      break;
    }
    break;
  default:
    return VK_FORMAT_UNDEFINED;
    break;
  }
}

} // namespace Infinite