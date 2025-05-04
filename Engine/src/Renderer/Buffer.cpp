#include "Buffer.h"

/*
 * Encapsulates a vulkan buffer
 *
 * Initially based off VulkanBuffer by Sascha Willems -
 * https://github.com/SaschaWillems/Vulkan/blob/master/base/VulkanBuffer.h
 */

VkDeviceSize Buffer::GetAlignment(const VkDeviceSize instanceSize, const VkDeviceSize minOffsetAlignment)
{
	if (minOffsetAlignment > 0)
	{
		return (instanceSize + minOffsetAlignment - 1) & ~(minOffsetAlignment - 1);
	}
	return instanceSize;
}

Buffer::Buffer(
	std::shared_ptr<Device> device,
	const VkDeviceSize instanceSize,
	const uint32_t instanceCount,
	const VkBufferUsageFlags usageFlags,
	const VkMemoryPropertyFlags memoryPropertyFlags,
	const VkDeviceSize minOffsetAlignment)
	: m_Device{device},
	  m_InstanceSize{instanceSize},
	  m_InstanceCount{instanceCount},
	  m_UsageFlags{usageFlags},
	  m_MemoryPropertyFlags{memoryPropertyFlags}
{
	m_AlignmentSize = GetAlignment(instanceSize, minOffsetAlignment);
	m_BufferSize = m_AlignmentSize * instanceCount;
	m_Handle = device->CreateBuffer(m_BufferSize, usageFlags, memoryPropertyFlags);
}

Buffer::~Buffer()
{
	Unmap();
	vkDestroyBuffer(m_Device->GetLogicalDevice(), m_Handle.Buffer, nullptr);
	vkFreeMemory(m_Device->GetLogicalDevice(), m_Handle.Memory, nullptr);
}

VkResult Buffer::Map(const VkDeviceSize size, const VkDeviceSize offset)
{
	assert(m_Handle.Buffer && m_Handle.Memory && "Called map on buffer before create");
	return vkMapMemory(m_Device->GetLogicalDevice(), m_Handle.Memory, offset, size, 0, &m_Mapped);
}

void Buffer::Unmap()
{
	if (m_Mapped)
	{
		vkUnmapMemory(m_Device->GetLogicalDevice(), m_Handle.Memory);
		m_Mapped = nullptr;
	}
}

void Buffer::WriteToBuffer(const void *data, const VkDeviceSize size, const VkDeviceSize offset) const
{
	assert(m_Mapped && "Cannot copy to unmapped buffer");

	if (size == VK_WHOLE_SIZE)
	{
		memcpy(m_Mapped, data, m_BufferSize);
	}
	else
	{
		char *memOffset = (char *)m_Mapped;
		memOffset += offset;
		memcpy(memOffset, data, size);
	}
}

VkResult Buffer::Flush(const VkDeviceSize size, const VkDeviceSize offset) const
{
	VkMappedMemoryRange mappedRange = {};
	mappedRange.sType = VK_STRUCTURE_TYPE_MAPPED_MEMORY_RANGE;
	mappedRange.memory = m_Handle.Memory;
	mappedRange.offset = offset;
	mappedRange.size = size;
	return vkFlushMappedMemoryRanges(m_Device->GetLogicalDevice(), 1, &mappedRange);
}

VkResult Buffer::Invalidate(const VkDeviceSize size, const VkDeviceSize offset) const
{
	VkMappedMemoryRange mappedRange = {};
	mappedRange.sType = VK_STRUCTURE_TYPE_MAPPED_MEMORY_RANGE;
	mappedRange.memory = m_Handle.Memory;
	mappedRange.offset = offset;
	mappedRange.size = size;
	return vkInvalidateMappedMemoryRanges(m_Device->GetLogicalDevice(), 1, &mappedRange);
}

VkDescriptorBufferInfo Buffer::DescriptorInfo(const VkDeviceSize size, const VkDeviceSize offset) const
{
	return VkDescriptorBufferInfo{
		m_Handle.Buffer,
		offset,
		size,
	};
}

void Buffer::WriteToIndex(const void *data, const int index) const
{
	WriteToBuffer(data, m_InstanceSize, index * m_AlignmentSize);
}

VkResult Buffer::FlushIndex(const int index) const
{
	return Flush(m_AlignmentSize, index * m_AlignmentSize);
}

VkDescriptorBufferInfo Buffer::DescriptorInfoForIndex(const int index) const
{
	return DescriptorInfo(m_AlignmentSize, index * m_AlignmentSize);
}

VkResult Buffer::InvalidateIndex(const int index) const
{
	return Invalidate(m_AlignmentSize, index * m_AlignmentSize);
}
