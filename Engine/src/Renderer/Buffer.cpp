#include "Buffer.h"

Buffer::Buffer(Device* device, VkDeviceSize size, VkBufferUsageFlags usage, 
               VkMemoryPropertyFlags properties, VkCommandPool commandPool)
    : device(device), bufferSize(size), usage(usage), commandPool(commandPool)
{
    device->CreateBuffer(size, usage, properties, buffer, bufferMemory);
}

Buffer::~Buffer()
{
    Cleanup();
}

Buffer::Buffer(Buffer&& other) noexcept
    : device(other.device), buffer(other.buffer), bufferMemory(other.bufferMemory),
      bufferSize(other.bufferSize), usage(other.usage), commandPool(other.commandPool),
      mapped(other.mapped)
{
    other.buffer = VK_NULL_HANDLE;
    other.bufferMemory = VK_NULL_HANDLE;
    other.mapped = nullptr;
}

Buffer& Buffer::operator=(Buffer&& other) noexcept
{
    if (this != &other)
    {
        Cleanup();
        
        device = other.device;
        buffer = other.buffer;
        bufferMemory = other.bufferMemory;
        bufferSize = other.bufferSize;
        usage = other.usage;
        commandPool = other.commandPool;
        mapped = other.mapped;
        
        other.buffer = VK_NULL_HANDLE;
        other.bufferMemory = VK_NULL_HANDLE;
        other.mapped = nullptr;
    }
    return *this;
}

void Buffer::Map(VkDeviceSize size, VkDeviceSize offset)
{
    vkMapMemory(device->GetLogicalDevice(), bufferMemory, offset, size, 0, &mapped);
}

void Buffer::Unmap()
{
    if (mapped)
    {
        vkUnmapMemory(device->GetLogicalDevice(), bufferMemory);
        mapped = nullptr;
    }
}

void Buffer::WriteToBuffer(void* data, VkDeviceSize size, VkDeviceSize offset)
{
    if (size == VK_WHOLE_SIZE)
    {
        size = bufferSize;
    }

    if (mapped)
    {
        memcpy(static_cast<char*>(mapped) + offset, data, size);
    }
    else
    {
        Map(size, offset);
        memcpy(mapped, data, size);
        Unmap();
    }
}

void Buffer::CopyFrom(Buffer& srcBuffer, VkDeviceSize size)
{
    device->CopyBuffer(srcBuffer.GetBuffer(), buffer, size, commandPool);
}

void Buffer::Flush(VkDeviceSize size, VkDeviceSize offset)
{
    VkMappedMemoryRange mappedRange{};
    mappedRange.sType = VK_STRUCTURE_TYPE_MAPPED_MEMORY_RANGE;
    mappedRange.memory = bufferMemory;
    mappedRange.offset = offset;
    mappedRange.size = size;
    vkFlushMappedMemoryRanges(device->GetLogicalDevice(), 1, &mappedRange);
}

void Buffer::Invalidate(VkDeviceSize size, VkDeviceSize offset)
{
    VkMappedMemoryRange mappedRange{};
    mappedRange.sType = VK_STRUCTURE_TYPE_MAPPED_MEMORY_RANGE;
    mappedRange.memory = bufferMemory;
    mappedRange.offset = offset;
    mappedRange.size = size;
    vkInvalidateMappedMemoryRanges(device->GetLogicalDevice(), 1, &mappedRange);
}

void Buffer::Cleanup()
{
    if (buffer != VK_NULL_HANDLE)
    {
        Unmap();
        vkDestroyBuffer(device->GetLogicalDevice(), buffer, nullptr);
        vkFreeMemory(device->GetLogicalDevice(), bufferMemory, nullptr);
        buffer = VK_NULL_HANDLE;
        bufferMemory = VK_NULL_HANDLE;
    }
}