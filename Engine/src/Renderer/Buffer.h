#pragma once
#include "pch.h"
#include "Device.h"

class Buffer
{
public:
	Buffer(
		Device& device,
		VkDeviceSize instanceSize,
		uint32_t instanceCount,
		VkBufferUsageFlags usageFlags,
		VkMemoryPropertyFlags memoryPropertyFlags,
		VkDeviceSize minOffsetAlignment = 1);
	~Buffer();

	Buffer(const Buffer&) = delete;
	Buffer& operator=(const Buffer&) = delete;

	VkResult Map(VkDeviceSize size = VK_WHOLE_SIZE, VkDeviceSize offset = 0);
	void Unmap();

	void WriteToBuffer(const void* data, VkDeviceSize size = VK_WHOLE_SIZE, VkDeviceSize offset = 0) const;
	VkResult Flush(VkDeviceSize size = VK_WHOLE_SIZE, VkDeviceSize offset = 0) const;
	VkDescriptorBufferInfo DescriptorInfo(VkDeviceSize size = VK_WHOLE_SIZE, VkDeviceSize offset = 0) const;
	VkResult Invalidate(VkDeviceSize size = VK_WHOLE_SIZE, VkDeviceSize offset = 0) const;

	void WriteToIndex(const void* data, int index) const;
	VkResult FlushIndex(int index) const;
	VkDescriptorBufferInfo DescriptorInfoForIndex(int index) const;
	VkResult InvalidateIndex(int index) const;

	VkBuffer				GetBuffer() const			{ return m_Handle.Buffer; }
	void*					GetMappedMemory() const		{ return m_Mapped; }
	uint32_t				GetInstanceCount() const	{ return m_InstanceCount; }
	VkDeviceSize			GetInstanceSize() const		{ return m_InstanceSize; }
	VkDeviceSize			GetAlignmentSize() const	{ return m_InstanceSize; }
	VkBufferUsageFlags		GetUsageFlags() const		{ return m_UsageFlags; }
	VkDeviceSize			GetBufferSize() const		{ return m_BufferSize; }
	VkMemoryPropertyFlags	GetMemoryPropertyFlags() const { return m_MemoryPropertyFlags; }

private:
	static VkDeviceSize GetAlignment(VkDeviceSize instanceSize, VkDeviceSize minOffsetAlignment);

	Device& m_Device;
	BufferHandle m_Handle;
	void* m_Mapped = nullptr;

	VkDeviceSize m_BufferSize;
	uint32_t m_InstanceCount;
	VkDeviceSize m_InstanceSize;
	VkDeviceSize m_AlignmentSize;
	VkBufferUsageFlags m_UsageFlags;
	VkMemoryPropertyFlags m_MemoryPropertyFlags;
};
