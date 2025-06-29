#pragma once

#include "pch.h"
#include "Buffer.h"

template<typename T>
class UniformBuffer
{
public:
    UniformBuffer(Device* device, uint32_t count = 1);
    ~UniformBuffer();

    // Delete copy operations
    UniformBuffer(const UniformBuffer&) = delete;
    UniformBuffer& operator=(const UniformBuffer&) = delete;

    void UpdateUniform(const T& data, uint32_t index = 0);
    
    VkBuffer GetBuffer(uint32_t index = 0) const { return buffers[index]->GetBuffer(); }
    VkDescriptorBufferInfo GetDescriptorInfo(uint32_t index = 0) const;
    uint32_t GetCount() const { return static_cast<uint32_t>(buffers.size()); }

private:
    Device* device;
    std::vector<std::unique_ptr<Buffer>> buffers;
    VkDeviceSize bufferSize;
};

// Template implementation
template<typename T>
UniformBuffer<T>::UniformBuffer(Device* device, uint32_t count)
    : device(device), bufferSize(sizeof(T))
{
    buffers.reserve(count);
    
    for (uint32_t i = 0; i < count; i++)
    {
        auto buffer = std::make_unique<Buffer>(
            device,
            bufferSize,
            VK_BUFFER_USAGE_UNIFORM_BUFFER_BIT,
            VK_MEMORY_PROPERTY_HOST_VISIBLE_BIT | VK_MEMORY_PROPERTY_HOST_COHERENT_BIT
        );
        buffer->Map();
        buffers.push_back(std::move(buffer));
    }
}

template<typename T>
UniformBuffer<T>::~UniformBuffer() = default;

template<typename T>
void UniformBuffer<T>::UpdateUniform(const T& data, uint32_t index)
{
    if (index >= buffers.size())
    {
        throw std::runtime_error("Uniform buffer index out of range!");
    }
    
    buffers[index]->WriteToBuffer((void*)&data, bufferSize);
}

template<typename T>
VkDescriptorBufferInfo UniformBuffer<T>::GetDescriptorInfo(uint32_t index) const
{
    if (index >= buffers.size())
    {
        throw std::runtime_error("Uniform buffer index out of range!");
    }
    
    VkDescriptorBufferInfo bufferInfo{};
    bufferInfo.buffer = buffers[index]->GetBuffer();
    bufferInfo.offset = 0;
    bufferInfo.range = bufferSize;
    
    return bufferInfo;
}