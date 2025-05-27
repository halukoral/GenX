#pragma once

#include "pch.h"
#include "Device.h"
#include "Model.h"

class Texture;

// for uniform buffer
class Descriptor
{
public:
    Descriptor(Device* dev, int maxFrames);
    ~Descriptor();

    VkDescriptorSetLayout& GetDescriptorSetLayout() { return descriptorSetLayout; }
    VkDescriptorSet& GetDescriptorSet(const int frameIndex) { return descriptorSets[frameIndex]; }

    void UpdateUniformBuffer(uint32_t currentFrame, const UniformBufferObject& ubo) const;
	void UpdateTextureDescriptor(Texture* texture);
	
private:
    void CreateDescriptorSetLayout();
    void CreateUniformBuffers();
    void CreateDescriptorPool();
    void CreateDescriptorSets();
    void CreateBuffer(VkDeviceSize size, VkBufferUsageFlags usage, VkMemoryPropertyFlags properties, VkBuffer& buffer, VkDeviceMemory& bufferMemory) const;

	void Cleanup() const;

private:
	Device* device;
	VkDescriptorSetLayout descriptorSetLayout = VK_NULL_HANDLE;
	VkDescriptorPool descriptorPool = VK_NULL_HANDLE;
	std::vector<VkDescriptorSet> descriptorSets;
	std::vector<VkBuffer> uniformBuffers;
	std::vector<VkDeviceMemory> uniformBuffersMemory;
	std::vector<void*> uniformBuffersMapped;
	int maxFramesInFlight;
};
