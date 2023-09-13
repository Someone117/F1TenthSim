#pragma once

#include "Model.h"
#include <unordered_map>
#include "tiny_obj_loader.h"
#include "../Settings.h"
#include "../Rendering/Engine.h"
#include <glm/gtx/hash.hpp>

namespace std {
    template<>
    struct hash<Infinite::Vertex> {
        size_t operator()(Infinite::Vertex const &vertex) const {
            return (hash<glm::vec3>()(vertex.pos) ^ (hash<glm::vec2>()(vertex.texCoord) << 1));
        }
    };
}

namespace Infinite {
    void
    Model::loadModel(std::vector<Vertex> &vertices, std::vector<uint32_t> &indices, const std::string &model_path) {
        tinyobj::attrib_t attrib;
        std::vector<tinyobj::shape_t> shapes;
        std::vector<tinyobj::material_t> materials;
        std::string warn, err;

        if (!tinyobj::LoadObj(&attrib, &shapes, &materials, &warn, &err, model_path.c_str())) {
            throw std::runtime_error(warn + err);
        }

        std::unordered_map<Vertex, uint32_t> uniqueVertices{};

        for (const auto &shape: shapes) {
            for (const auto &index: shape.mesh.indices) {
                Vertex vertex({
                                      attrib.vertices[3 * index.vertex_index + 0],
                                      attrib.vertices[3 * index.vertex_index + 1],
                                      attrib.vertices[3 * index.vertex_index + 2]},
                              {
                                      attrib.texcoords[2 * index.texcoord_index + 0],
                                      1.0f - attrib.texcoords[2 * index.texcoord_index + 1]});


                if (uniqueVertices.count(vertex) == 0) {
                    uniqueVertices[vertex] = static_cast<uint32_t>(vertices.size());
                    vertices.push_back(vertex);
                }

                indices.push_back(uniqueVertices[vertex]);
            }
        }
    }

    void Model::createDescriptorSetLayout() {
        VkDescriptorSetLayoutBinding uboLayoutBinding{};
        uboLayoutBinding.binding = 0;
        uboLayoutBinding.descriptorCount = 1;
        uboLayoutBinding.descriptorType = VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER;
        uboLayoutBinding.pImmutableSamplers = nullptr;
        uboLayoutBinding.stageFlags = VK_SHADER_STAGE_VERTEX_BIT;

        VkDescriptorSetLayoutBinding samplerLayoutBinding{};
        samplerLayoutBinding.binding = 1;
        samplerLayoutBinding.descriptorCount = 1;
        samplerLayoutBinding.descriptorType = VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER;
        samplerLayoutBinding.pImmutableSamplers = nullptr;
        samplerLayoutBinding.stageFlags = VK_SHADER_STAGE_FRAGMENT_BIT;

        std::array<VkDescriptorSetLayoutBinding, 2> bindings = {uboLayoutBinding, samplerLayoutBinding};
        VkDescriptorSetLayoutCreateInfo layoutInfo{};
        layoutInfo.sType = VK_STRUCTURE_TYPE_DESCRIPTOR_SET_LAYOUT_CREATE_INFO;
        layoutInfo.bindingCount = static_cast<uint32_t>(bindings.size());
        layoutInfo.pBindings = bindings.data();

        if (vkCreateDescriptorSetLayout(device, &layoutInfo, nullptr,
                                        &descriptorSetLayout) != VK_SUCCESS) {
            throw std::runtime_error("failed to create descriptor set layout!");
        }
    }

    void Model::createDescriptorPool() {
        std::array<VkDescriptorPoolSize, 2> poolSizes{};
        poolSizes[0].type = VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER;
        poolSizes[0].descriptorCount = static_cast<uint32_t>(MAX_FRAMES_IN_FLIGHT * MAX_MODELS);
        poolSizes[1].type = VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER;
        poolSizes[1].descriptorCount = static_cast<uint32_t>(MAX_FRAMES_IN_FLIGHT * MAX_MODELS);

        VkDescriptorPoolCreateInfo poolInfo{};
        poolInfo.sType = VK_STRUCTURE_TYPE_DESCRIPTOR_POOL_CREATE_INFO;
        poolInfo.poolSizeCount = static_cast<uint32_t>(poolSizes.size());
        poolInfo.pPoolSizes = poolSizes.data();
        poolInfo.maxSets = static_cast<uint32_t>(MAX_FRAMES_IN_FLIGHT * MAX_MODELS);

        if (vkCreateDescriptorPool(device, &poolInfo, nullptr, &descriptorPool) !=
            VK_SUCCESS) {
            throw std::runtime_error("failed to createExtras descriptor pool!");
        }
    }

    void Model::createUniformBuffer() {
        VkDeviceSize bufferSize = sizeof(UniformBufferObject);

        uniformBuffers.resize(MAX_FRAMES_IN_FLIGHT);

        for (size_t i = 0; i < MAX_FRAMES_IN_FLIGHT; i++) {
            createBuffer(bufferSize, VK_BUFFER_USAGE_UNIFORM_BUFFER_BIT,
                         reinterpret_cast<BufferAlloc &>(uniformBuffers[i]),
                         VK_MEMORY_PROPERTY_HOST_VISIBLE_BIT | VK_MEMORY_PROPERTY_HOST_COHERENT_BIT,
                         VMA_ALLOCATION_CREATE_MAPPED_BIT | VMA_ALLOCATION_CREATE_HOST_ACCESS_RANDOM_BIT);
        }
    }

    void Model::createDescriptorSets() {
        std::vector<VkDescriptorSetLayout> layouts(MAX_FRAMES_IN_FLIGHT, descriptorSetLayout);
        VkDescriptorSetAllocateInfo allocInfo{};
        allocInfo.sType = VK_STRUCTURE_TYPE_DESCRIPTOR_SET_ALLOCATE_INFO;
        allocInfo.descriptorPool = descriptorPool;
        allocInfo.descriptorSetCount = static_cast<uint32_t>(MAX_FRAMES_IN_FLIGHT);
        allocInfo.pSetLayouts = layouts.data();

        descriptorSets.resize(MAX_FRAMES_IN_FLIGHT);
        if (vkAllocateDescriptorSets(device, &allocInfo, descriptorSets.data()) !=
            VK_SUCCESS) {
            throw std::runtime_error("failed to allocate descriptor sets!");
        }

        for (size_t i = 0; i < MAX_FRAMES_IN_FLIGHT; i++) {
            VkDescriptorBufferInfo bufferInfo{};
            bufferInfo.buffer = uniformBuffers[i].buffer;
            bufferInfo.offset = 0;
            bufferInfo.range = sizeof(UniformBufferObject);

            VkDescriptorImageInfo imageInfo{};
            imageInfo.imageLayout = VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL;
            imageInfo.imageView = *texture.getImageView();
            imageInfo.sampler = *texture.getSampler();

            std::array<VkWriteDescriptorSet, 2> descriptorWrites{};
            descriptorWrites[0].sType = VK_STRUCTURE_TYPE_WRITE_DESCRIPTOR_SET;
            descriptorWrites[0].dstSet = descriptorSets[i];
            descriptorWrites[0].dstBinding = 0;
            descriptorWrites[0].dstArrayElement = 0;
            descriptorWrites[0].descriptorType = VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER;
            descriptorWrites[0].descriptorCount = 1;
            descriptorWrites[0].pBufferInfo = &bufferInfo;

            descriptorWrites[1].sType = VK_STRUCTURE_TYPE_WRITE_DESCRIPTOR_SET;
            descriptorWrites[1].dstSet = descriptorSets[i];
            descriptorWrites[1].dstBinding = 1;
            descriptorWrites[1].dstArrayElement = 0;
            descriptorWrites[1].descriptorType = VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER;
            descriptorWrites[1].descriptorCount = 1;
            descriptorWrites[1].pImageInfo = &imageInfo;

            vkUpdateDescriptorSets(device, static_cast<uint32_t>(descriptorWrites.size()),
                                   descriptorWrites.data(),
                                   0, nullptr);
        }
    }

    BufferAlloc Model::getUniformBuffer(uint32_t currentImage) {
        return uniformBuffers[currentImage];
    }

    Model::Model(std::string _name, std::string model_path, const char *texture_path) : name(std::move(_name)),
                                                                                        texture(texture_path) {
        texture.create();

        std::vector<Vertex> vertices;
        std::vector<uint32_t> indices;

        loadModel(vertices, indices, model_path);

        indexCount = indices.size();

        createVertexBuffer(vertexBuffer, vertices);

        createIndexBuffer(indexBuffer, indices);

        position = glm::vec3(0.0f);
        scale = glm::vec3(1.0f);
        xAngle = 0.0f;
        yAngle = 0.0f;
        zAngle = 0.0f;

        createUniformBuffer();

        createDescriptorSets();
    }

    void Model::updateUniformBuffer(uint32_t index, Camera camera) {

        UniformBufferObject ubo{};
        ubo.model = glm::mat4(1.0f);

        ubo.proj = glm::perspective(glm::radians(110.0f), Engine::getEngine().getWindowWidth() /
                                                          (float) Engine::getEngine().getWindowHeight(),
                                    0.1f, 10.0f);
        ubo.view = camera.getViewMatrix();
        ubo.proj[1][1] *= -1;

        void *data;
        vmaMapMemory(allocator, uniformBuffers[index].allocation, &data);
        memcpy(data, &ubo, sizeof(ubo));
        vmaUnmapMemory(allocator, uniformBuffers[index].allocation);
    }


    glm::mat4 Model::getMatrix() {
        auto qY = glm::angleAxis(yAngle, glm::vec3(1, 0, 0));
        auto qX = glm::angleAxis(xAngle, glm::vec3(0, 0, 1));
        auto qZ = glm::angleAxis(zAngle, glm::vec3(0, 1, 0));
        glm::quat rotation = qY * qX * qZ;
        rotation = glm::normalize(rotation);
        return glm::translate(glm::scale(glm::mat4(1), scale) * glm::mat4_cast(rotation), position);
    }

    void Model::translate(glm::vec3 vector) {
        position += vector;
    }

    void Model::rotate(glm::vec3 radians) {
        xAngle += radians.x;
        yAngle += radians.y;
        zAngle += radians.z;
    }

    const glm::vec3 &Model::getPosition() const {
        return position;
    }

    const glm::vec3 &Model::getScale() const {
        return scale;
    }

    float Model::getXAngle() const {
        return xAngle;
    }

    float Model::getYAngle() const {
        return yAngle;
    }

    float Model::getZAngle() const {
        return zAngle;
    }

    const std::string &Model::getName() const {
        return name;
    }

    void Model::destroy() {
        for (size_t i = 0; i < MAX_FRAMES_IN_FLIGHT; i++) {
            vmaDestroyBuffer(allocator, uniformBuffers[i].buffer, uniformBuffers[i].allocation);
        }
        vmaDestroyBuffer(allocator, vertexBuffer.buffer, vertexBuffer.allocation);
        vmaDestroyBuffer(allocator, indexBuffer.buffer, indexBuffer.allocation);
        texture.destroy(allocator);

        vkFreeDescriptorSets(device, descriptorPool, MAX_FRAMES_IN_FLIGHT,
                             descriptorSets.data());

    }

    void Model::cleanUpDescriptors() {
        vkDestroyDescriptorPool(device, descriptorPool, nullptr);

        vkDestroyDescriptorSetLayout(device, descriptorSetLayout, nullptr);

    }

    const BufferAlloc &Model::getVertexBuffer() const {
        return vertexBuffer;
    }

    const BufferAlloc &Model::getIndexBuffer() const {
        return indexBuffer;
    }

    const std::vector<BufferAlloc> &Model::getUniformBuffers() const {
        return uniformBuffers;
    }

    const std::vector<VkDescriptorSet> &Model::getDescriptorSets() const {
        return descriptorSets;
    }

    uint32_t Model::getIndexCount() const {
        return indexCount;
    }


} // Infinite