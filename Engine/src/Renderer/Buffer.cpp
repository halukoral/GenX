#include "Buffer.h"

Buffer::Buffer(Device* device, VkDeviceSize size, VkBufferUsageFlags usage, 
               VkMemoryPropertyFlags properties, VkCommandPool commandPool)
    : m_Device(device), m_BufferSize(size), m_Usage(usage), m_CommandPool(commandPool)
{
    device->CreateBuffer(size, usage, properties, m_Buffer, m_BufferMemory);
}

Buffer::~Buffer()
{
    Cleanup();
}

Buffer::Buffer(Buffer&& other) noexcept
    : m_Device(other.m_Device), m_Buffer(other.m_Buffer), m_BufferMemory(other.m_BufferMemory),
      m_BufferSize(other.m_BufferSize), m_Usage(other.m_Usage), m_CommandPool(other.m_CommandPool),
      m_Mapped(other.m_Mapped)
{
    other.m_Buffer = VK_NULL_HANDLE;
    other.m_BufferMemory = VK_NULL_HANDLE;
    other.m_Mapped = nullptr;
}

Buffer& Buffer::operator=(Buffer&& other) noexcept
{
    if (this != &other)
    {
        Cleanup();
        
        m_Device = other.m_Device;
        m_Buffer = other.m_Buffer;
        m_BufferMemory = other.m_BufferMemory;
        m_BufferSize = other.m_BufferSize;
        m_Usage = other.m_Usage;
        m_CommandPool = other.m_CommandPool;
        m_Mapped = other.m_Mapped;
        
        other.m_Buffer = VK_NULL_HANDLE;
        other.m_BufferMemory = VK_NULL_HANDLE;
        other.m_Mapped = nullptr;
    }
    return *this;
}

void Buffer::Map(const VkDeviceSize size, const VkDeviceSize offset)
{
    vkMapMemory(m_Device->GetLogicalDevice(), m_BufferMemory, offset, size, 0, &m_Mapped);
}

void Buffer::Unmap()
{
    if (m_Mapped)
    {
        vkUnmapMemory(m_Device->GetLogicalDevice(), m_BufferMemory);
        m_Mapped = nullptr;
    }
}

void Buffer::WriteToBuffer(const void* data, VkDeviceSize size, const VkDeviceSize offset)
{
    if (size == VK_WHOLE_SIZE)
    {
        size = m_BufferSize;
    }

    if (m_Mapped)
    {
        memcpy(static_cast<char*>(m_Mapped) + offset, data, size);
    }
    else
    {
        Map(size, offset);
        memcpy(m_Mapped, data, size);
        Unmap();
    }
}

void Buffer::CopyFrom(const Buffer& srcBuffer, const VkDeviceSize size) const
{
    m_Device->CopyBuffer(srcBuffer.GetBuffer(), m_Buffer, size, m_CommandPool);
}

void Buffer::Flush(const VkDeviceSize size, const VkDeviceSize offset) const
{
    VkMappedMemoryRange mappedRange{};
    mappedRange.sType = VK_STRUCTURE_TYPE_MAPPED_MEMORY_RANGE;
    mappedRange.memory = m_BufferMemory;
    mappedRange.offset = offset;
    mappedRange.size = size;
    vkFlushMappedMemoryRanges(m_Device->GetLogicalDevice(), 1, &mappedRange);
}

void Buffer::Invalidate(const VkDeviceSize size, const VkDeviceSize offset) const
{
    VkMappedMemoryRange mappedRange{};
    mappedRange.sType = VK_STRUCTURE_TYPE_MAPPED_MEMORY_RANGE;
    mappedRange.memory = m_BufferMemory;
    mappedRange.offset = offset;
    mappedRange.size = size;
    vkInvalidateMappedMemoryRanges(m_Device->GetLogicalDevice(), 1, &mappedRange);
}

void Buffer::Cleanup()
{
    if (m_Buffer != VK_NULL_HANDLE)
    {
        Unmap();
        vkDestroyBuffer(m_Device->GetLogicalDevice(), m_Buffer, nullptr);
        vkFreeMemory(m_Device->GetLogicalDevice(), m_BufferMemory, nullptr);
        m_Buffer = VK_NULL_HANDLE;
        m_BufferMemory = VK_NULL_HANDLE;
    }
}