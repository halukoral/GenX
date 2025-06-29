#pragma once

#include "pch.h"
#include "Device.h"
#include "UniformBuffer.h"

template<typename UniformType>
class Descriptor {
public:
	Descriptor(Device* device, uint32_t maxFramesInFlight);
	~Descriptor();

	// Delete copy operations
	Descriptor(const Descriptor&) = delete;
	Descriptor& operator=(const Descriptor&) = delete;

	void CreateDescriptorSetLayout();
	void CreateDescriptorPool();
	void CreateDescriptorSets(UniformBuffer<UniformType>* uniformBuffers);
    
	VkDescriptorSetLayout GetDescriptorSetLayout() const { return descriptorSetLayout; }
	const std::vector<VkDescriptorSet>& GetDescriptorSets() const { return descriptorSets; }
	VkDescriptorSet GetDescriptorSet(uint32_t index) const { return descriptorSets[index]; }

private:
	Device* device;
	uint32_t maxFramesInFlight;
    
	VkDescriptorSetLayout descriptorSetLayout = VK_NULL_HANDLE;
	VkDescriptorPool descriptorPool = VK_NULL_HANDLE;
	std::vector<VkDescriptorSet> descriptorSets;
};