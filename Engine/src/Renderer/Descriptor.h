#pragma once
#include "pch.h"
#include "Device.h"

class DescriptorSetLayout
{
public:
	class Builder
	{
	public:
		Builder(const std::shared_ptr<Device>& device) : m_Device{device} {}

		Builder& addBinding(
			uint32_t binding,
			VkDescriptorType descriptorType,
			VkShaderStageFlags stageFlags,
			uint32_t count = 1);
		std::unique_ptr<DescriptorSetLayout> build() const;

	private:
		std::shared_ptr<Device> m_Device;
		std::unordered_map<uint32_t, VkDescriptorSetLayoutBinding> m_Bindings{};
	};

	DescriptorSetLayout(Device &lveDevice, std::unordered_map<uint32_t, VkDescriptorSetLayoutBinding> bindings);
	~DescriptorSetLayout();
	
	DescriptorSetLayout(const DescriptorSetLayout &) = delete;
	DescriptorSetLayout &operator=(const DescriptorSetLayout &) = delete;

	VkDescriptorSetLayout GetDescriptorSetLayout() const { return m_DescriptorSetLayout; }

private:
	Device& m_Device;
	VkDescriptorSetLayout m_DescriptorSetLayout;
	std::unordered_map<uint32_t, VkDescriptorSetLayoutBinding> m_Bindings;

	friend class DescriptorWriter;
};

class DescriptorPool
{
public:
	class Builder
	{
	public:
		Builder(const std::shared_ptr<Device>& device) : m_Device{device} {}

		Builder& AddPoolSize(VkDescriptorType descriptorType, uint32_t count);
		Builder& SetPoolFlags(VkDescriptorPoolCreateFlags flags);
		auto SetMaxSets(uint32_t count) -> Builder&;
		std::unique_ptr<DescriptorPool> Build() const;

	private:
		std::shared_ptr<Device> m_Device;
		uint32_t m_MaxSets = 1000;
		std::vector<VkDescriptorPoolSize> m_PoolSizes{};
		VkDescriptorPoolCreateFlags m_PoolFlags = 0;
	};

	DescriptorPool(
		std::shared_ptr<Device> device,
		uint32_t maxSets,
		VkDescriptorPoolCreateFlags poolFlags,
		const std::vector<VkDescriptorPoolSize> &poolSizes);
	~DescriptorPool();
	
	DescriptorPool(const DescriptorPool &) = delete;
	DescriptorPool &operator=(const DescriptorPool &) = delete;

	bool AllocateDescriptor(const VkDescriptorSetLayout descriptorSetLayout, VkDescriptorSet &descriptor) const;
	void FreeDescriptors(const std::vector<VkDescriptorSet> &descriptors) const;
	void ResetPool() const;

private:
	std::shared_ptr<Device> m_Device;
	VkDescriptorPool m_DescriptorPool;

	friend class DescriptorWriter;
};

class DescriptorWriter {
public:
	DescriptorWriter(DescriptorSetLayout& setLayout, DescriptorPool& pool);

	DescriptorWriter &WriteBuffer(uint32_t binding, const VkDescriptorBufferInfo *bufferInfo);
	DescriptorWriter &WriteImage(uint32_t binding, const VkDescriptorImageInfo *imageInfo);

	bool Build(VkDescriptorSet &set);
	void Overwrite(const VkDescriptorSet &set);

private:
	DescriptorSetLayout& m_SetLayout;
	DescriptorPool& m_Pool;
	std::vector<VkWriteDescriptorSet> m_Writes;
};

