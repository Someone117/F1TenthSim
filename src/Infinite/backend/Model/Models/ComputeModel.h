#ifndef VULKAN_COMPUTE_MODEL_H
#define VULKAN_COMPUTE_MODEL_H

#include "BaseModel.h"
#include "DescriptorSet.h"
#include <cstdint>

#pragma once

namespace Infinite {

class ComputeModel : public BaseModel {
protected:
public:
  ComputeModel() : BaseModel("ComputeModel"){};


  inline const BufferAlloc &getVertexBuffer() const { throw "Not implemented"; }
  inline const BufferAlloc &getIndexBuffer() const { throw "Not implemented"; }
  inline DescriptorSet getDescriptorSet() const { throw "Not implemented"; }
  inline uint32_t getIndexCount() const { return 0; };
};
} // namespace Infinite

#endif // VULKAN_COMPUTE_MODEL_H
