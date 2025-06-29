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
	void WriteToBuffer(void* data, VkDeviceSize size = VK_WHOLE_SIZE, VkDeviceSize offset = 0);
	void CopyFrom(Buffer& srcBuffer, VkDeviceSize size);
	void Flush(VkDeviceSize size = VK_WHOLE_SIZE, VkDeviceSize offset = 0);
	void Invalidate(VkDeviceSize size = VK_WHOLE_SIZE, VkDeviceSize offset = 0);

	VkBuffer GetBuffer() const { return buffer; }
	VkDeviceMemory GetMemory() const { return bufferMemory; }
	void* GetMappedMemory() const { return mapped; }
	VkDeviceSize GetSize() const { return bufferSize; }
	VkBufferUsageFlags GetUsage() const { return usage; }

private:
	void Cleanup();

private:
	Device* device;
	VkBuffer buffer = VK_NULL_HANDLE;
	VkDeviceMemory bufferMemory = VK_NULL_HANDLE;
	VkDeviceSize bufferSize;
	VkBufferUsageFlags usage;
	VkCommandPool commandPool;
	void* mapped = nullptr;
};