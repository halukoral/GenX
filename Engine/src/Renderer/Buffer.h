#pragma once

#include "pch.h"
#include "Device.h"

class Buffer
{
public:
	Buffer(Device* device, VkDeviceSize size, VkBufferUsageFlags usage, 
		   VkMemoryPropertyFlags properties, VkCommandPool commandPool = VK_NULL_HANDLE);
	~Buffer();

	// Delete copy operations
	Buffer(const Buffer&) = delete;
	Buffer& operator=(const Buffer&) = delete;

	// Move operations
	Buffer(Buffer&& other) noexcept;
	Buffer& operator=(Buffer&& other) noexcept;

	void Map(VkDeviceSize size = VK_WHOLE_SIZE, VkDeviceSize offset = 0);
	void Unmap();
	void WriteToBuffer(const void* data, VkDeviceSize size = VK_WHOLE_SIZE, VkDeviceSize offset = 0);
	void CopyFrom(const Buffer& srcBuffer, VkDeviceSize size) const;
	void Flush(VkDeviceSize size = VK_WHOLE_SIZE, VkDeviceSize offset = 0) const;
	void Invalidate(VkDeviceSize size = VK_WHOLE_SIZE, VkDeviceSize offset = 0) const;

	VkBuffer			GetBuffer() const { return m_Buffer; }
	VkDeviceMemory		GetMemory() const { return m_BufferMemory; }
	void*				GetMappedMemory() const { return m_Mapped; }
	VkDeviceSize		GetSize() const { return m_BufferSize; }
	VkBufferUsageFlags	GetUsage() const { return m_Usage; }

private:
	void Cleanup();

private:
	Device* m_Device;
	VkBuffer m_Buffer = VK_NULL_HANDLE;
	VkDeviceMemory m_BufferMemory = VK_NULL_HANDLE;
	VkDeviceSize m_BufferSize;
	VkBufferUsageFlags m_Usage;
	VkCommandPool m_CommandPool;
	void* m_Mapped = nullptr;
};